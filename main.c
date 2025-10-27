#include "fun.h"
#include "types.h"


i32 main(void) {
    byte *argv[] = {"./lol.sh", NULL};

    return _fun(argv);
}
