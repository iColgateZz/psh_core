#define PSH_CORE_IMPL
#define PSH_NO_ECHO
#include "../psh_core.h"

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    Arena *arenas[2];
    usize ready_count;
    b32 release;
} ScratchThreads;

static void *run_scratch_thread(void *argument) {
    ScratchThreads *threads = argument;
    CHECK(pthread_mutex_lock(&threads->mutex) == 0);
    Scratch scratch = scratch_get();
    CHECK(arena_push(scratch.arena, u64) != NULL);
    threads->arenas[threads->ready_count++] = scratch.arena;
    CHECK(pthread_cond_broadcast(&threads->condition) == 0);
    while (!threads->release) CHECK(pthread_cond_wait(&threads->condition, &threads->mutex) == 0);
    CHECK(pthread_mutex_unlock(&threads->mutex) == 0);
    scratch_end(scratch);
    arena_destroy(*scratch.arena);
    return NULL;
}

static void test_scratch_threads(void) {
    ScratchThreads threads = {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .condition = PTHREAD_COND_INITIALIZER,
    };
    pthread_t workers[2];
    for (usize i = 0; i < psh_countof(workers); ++i) {
        CHECK(pthread_create(&workers[i], NULL, run_scratch_thread, &threads) == 0);
    }
    CHECK(pthread_mutex_lock(&threads.mutex) == 0);
    while (threads.ready_count < psh_countof(workers)) {
        CHECK(pthread_cond_wait(&threads.condition, &threads.mutex) == 0);
    }
    CHECK(threads.arenas[0] != threads.arenas[1]);
    CHECK(threads.arenas[0]->base_ptr != threads.arenas[1]->base_ptr);
    threads.release = true;
    CHECK(pthread_cond_broadcast(&threads.condition) == 0);
    CHECK(pthread_mutex_unlock(&threads.mutex) == 0);
    for (usize i = 0; i < psh_countof(workers); ++i) CHECK(pthread_join(workers[i], NULL) == 0);
    CHECK(pthread_cond_destroy(&threads.condition) == 0);
    CHECK(pthread_mutex_destroy(&threads.mutex) == 0);
}

static void test_time_and_arena(void) {
    u64 previous = psh_time_now_ns();
    CHECK(previous > 0);
    for (usize i = 0; i < 10000; ++i) {
        u64 current = psh_time_now_ns();
        CHECK(current >= previous);
        previous = current;
    }

    Arena arena = arena_init(MB(1) + 1);
    CHECK(arena.base_ptr != NULL);
    CHECK(arena.page_size > 0);
    CHECK((uptr)arena.base_ptr % arena.page_size == 0);
    CHECK(arena.reserved_size >= MB(1) + 1);
    CHECK(arena.reserved_size % arena.page_size == 0);
    CHECK(arena.committed_size == 0);

    byte *first = arena_push(&arena, byte, arena.page_size - 1);
    CHECK(first != NULL);
    memset(first, 0x5a, arena.page_size - 1);
    CHECK(arena.committed_size == arena.page_size);

    u64 *value = arena_push(&arena, u64);
    CHECK(value != NULL);
    CHECK((uptr)value % alignof_type(u64) == 0);
    *value = UINT64_C(0x123456789abcdef0);
    CHECK(arena.committed_size == arena.page_size * 2);
    for (usize i = 0; i < arena.page_size - 1; ++i) CHECK(first[i] == 0x5a);

    ArenaSP savepoint = arena_savepoint(&arena);
    byte *more = arena_push(&arena, byte, arena.page_size * 2);
    CHECK(more != NULL);
    memset(more, 0x33, arena.page_size * 2);
    CHECK(*value == UINT64_C(0x123456789abcdef0));
    arena_restore(&arena, savepoint);
    CHECK(arena.current_offset == savepoint);
    CHECK(arena_push(&arena, byte, arena.reserved_size) == NULL);
    CHECK(arena.current_offset == savepoint);
    arena_clear(&arena);
    CHECK(arena.current_offset == 0);
    arena_destroy(arena);

    Arena lazy_arena = {0};
    CHECK(arena_push(&lazy_arena, u64) != NULL);
    arena_destroy(lazy_arena);
}

static void test_files(void) {
    Psh_Cmd command = {0};
    Psh_Fd fd = psh_fd_openw("contents.txt");
    CHECK(fd != PSH_INVALID_FD);
    CHECK(psh_fd_not_default(fd));
    psh_cmd_append(&command, "sh", "-c", "printf first");
    CHECK(psh_cmd_run(&command, .fdout = fd));

    fd = psh_fd_opena("contents.txt");
    CHECK(fd != PSH_INVALID_FD);
    psh_cmd_append(&command, "sh", "-c", "printf second");
    CHECK(psh_cmd_run(&command, .fdout = fd));

    Psh_Fd_Reader reader = {.fd = psh_fd_openr("contents.txt")};
    CHECK(reader.fd != PSH_INVALID_FD);
    CHECK(psh_fd_read(&reader));
    CHECK(reader.ready);
    CHECK(reader.store.count == strlen("firstsecond"));
    CHECK(memcmp(reader.store.items, "firstsecond", reader.store.count) == 0);
    psh_list_free(reader.store);

    fd = psh_fd_openw("contents.txt");
    CHECK(fd != PSH_INVALID_FD);
    psh_fd_close(fd);
    reader = (Psh_Fd_Reader){.fd = psh_fd_open("contents.txt", O_RDONLY, 0)};
    CHECK(reader.fd != PSH_INVALID_FD);
    CHECK(psh_fd_read(&reader));
    CHECK(reader.ready && reader.store.count == 0);
    psh_list_free(reader.store);
    psh_list_free(command);
}

static void test_commands(void) {
    Psh_Cmd command = {0};
    psh_cmd_append(&command, "sh", "-c", "exit 0");
    CHECK(psh_cmd_run(&command));
    CHECK(command.count == 0);
    psh_cmd_append(&command, "sh", "-c", "exit 7");
    CHECK(!psh_cmd_run(&command));
    CHECK(command.count == 0);

    Psh_Procs processes = {0};
    u64 started = psh_time_now_ns();
    psh_cmd_append(&command, "sh", "-c", "sleep 1; printf first > order.txt");
    CHECK(psh_cmd_run(&command, .async = &processes, .max_procs = 1));
    CHECK(processes.count == 1);
    psh_cmd_append(&command, "sh", "-c", "test \"$(cat order.txt)\" = first && printf second >> order.txt");
    CHECK(psh_cmd_run(&command, .async = &processes, .max_procs = 1));
    CHECK(processes.count == 1);
    CHECK(psh_procs_block(&processes));
    CHECK(processes.count == 0);
    CHECK(psh_time_now_ns() - started >= UINT64_C(900000000));

    Psh_Fd_Reader reader = {.fd = psh_fd_openr("order.txt")};
    CHECK(reader.fd != PSH_INVALID_FD);
    CHECK(psh_fd_read(&reader));
    CHECK(reader.store.count == strlen("firstsecond"));
    CHECK(memcmp(reader.store.items, "firstsecond", reader.store.count) == 0);
    psh_list_free(reader.store);

    psh_cmd_append(&command, "sh", "-c", "exit 9");
    CHECK(psh_cmd_run(&command, .async = &processes, .max_procs = 2));
    psh_cmd_append(&command, "sh", "-c", "exit 0");
    CHECK(psh_cmd_run(&command, .async = &processes, .max_procs = 2));
    CHECK(processes.count == 2);
    CHECK(!psh_procs_block(&processes));
    CHECK(processes.count == 0);
    psh_list_free(processes);
    psh_list_free(command);
}

static void test_readers(void) {
    Psh_Unix_Pipe output = {0};
    Psh_Unix_Pipe errors = {0};
    CHECK(psh_pipe_open(&output));
    CHECK(psh_pipe_open(&errors));
    Psh_Fd_Reader readers[] = {
        {.fd = PSH_INVALID_FD, .ready = true},
        {.fd = output.read_fd},
        {.fd = PSH_INVALID_FD, .ready = true},
        {.fd = errors.read_fd},
    };
    CHECK(psh_fd_read(&readers[1], .nonblocking = true));
    CHECK(!readers[1].ready && readers[1].store.count == 0);

    Psh_Cmd command = {0};
    Psh_Procs processes = {0};
    psh_cmd_append(&command, "sh", "-c", "printf output; printf error >&2");
    CHECK(psh_cmd_run(&command, .async = &processes, .fdout = output.write_fd, .fderr = errors.write_fd));
    CHECK(psh_fd_readers_join(readers, psh_countof(readers)));
    CHECK(psh_procs_block(&processes));
    CHECK(readers[1].ready && readers[3].ready);
    CHECK(readers[1].store.count == strlen("output"));
    CHECK(memcmp(readers[1].store.items, "output", readers[1].store.count) == 0);
    CHECK(readers[3].store.count == strlen("error"));
    CHECK(memcmp(readers[3].store.items, "error", readers[3].store.count) == 0);
    CHECK(psh_fd_readers_join(readers, psh_countof(readers)));
    CHECK(psh_fd_readers_join(NULL, 0));
    psh_list_free(readers[1].store);
    psh_list_free(readers[3].store);
    psh_list_free(processes);
    psh_list_free(command);
}

static void test_pipeline(void) {
    Psh_Cmd command = {0};
    Psh_Pipeline pipeline = {0};
    psh_cmd_append(&command, "sh", "-c", "printf 'hello platform\\n'");
    CHECK(psh_pipeline_chain(&pipeline, &command));
    psh_cmd_append(&command, "tr", "a-z", "A-Z");
    CHECK(psh_pipeline_chain(&pipeline, &command, .fdout = psh_fd_openw("pipeline.txt")));
    CHECK(psh_pipeline_end(&pipeline));
    CHECK(!pipeline.error);

    Psh_Fd_Reader reader = {.fd = psh_fd_openr("pipeline.txt")};
    CHECK(reader.fd != PSH_INVALID_FD);
    CHECK(psh_fd_read(&reader));
    CHECK(reader.store.count == strlen("HELLO PLATFORM\n"));
    CHECK(memcmp(reader.store.items, "HELLO PLATFORM\n", reader.store.count) == 0);
    CHECK(fcntl(STDIN_FILENO, F_GETFD) >= 0);
    CHECK(fcntl(STDOUT_FILENO, F_GETFD) >= 0);
    psh_list_free(reader.store);
    psh_list_free(command);
}

int main(void) {
    alarm(20);
    test_time_and_arena();
    test_scratch_threads();
    test_files();
    test_commands();
    test_readers();
    test_pipeline();
    alarm(0);
    puts("psh_core platform tests passed");
    return EXIT_SUCCESS;
}
