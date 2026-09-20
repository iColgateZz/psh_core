#!/bin/sh
set -eu

tests_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/psh-core-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

cat > "$test_dir/header.c" <<'EOF'
#include "psh_core.h"

b32 public_header_consumer(Psh_Cmd *command) {
    return psh_cmd_run(command);
}
EOF

cat > "$test_dir/implementation.c" <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define PSH_CORE_IMPL
#include "psh_core.h"
EOF

${CC:-cc} ${CFLAGS:-} -std=c99 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -Wall -Wextra -Werror \
    -Wno-override-init -I "$tests_dir/.." -c "$test_dir/header.c" -o "$test_dir/header.o"
${CC:-cc} ${CFLAGS:-} -std=c99 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -Wall -Wextra -Werror \
    -Wno-override-init -I "$tests_dir/.." -c "$test_dir/implementation.c" -o "$test_dir/implementation.o"
${CC:-cc} ${CFLAGS:-} -std=c99 -pthread -Wall -Wextra -Werror \
    -Wno-override-init "$tests_dir/platform.c" -o "$test_dir/platform"

cd "$test_dir"
./platform

cp "$tests_dir/../psh_core.h" psh_core.h
cat > rebuild.c <<'EOF'
#define PSH_CORE_IMPL
#define PSH_NO_ECHO
#include "psh_core.h"

int main(int argc, char **argv) {
    PSH_REBUILD_UNITY(argc, argv, "psh_core.h");
#ifdef INITIAL_BUILD
    return EXIT_FAILURE;
#else
    if (argc != 2 || strcmp(argv[1], "preserved") != 0) return EXIT_FAILURE;
    puts("psh_core self-rebuild test passed");
    return EXIT_SUCCESS;
#endif
}
EOF

${CC:-cc} ${CFLAGS:-} -std=c99 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -DINITIAL_BUILD \
    -Wall -Wextra -Werror -Wno-override-init rebuild.c -o rebuild
touch -t 200001010000 rebuild
./rebuild preserved
