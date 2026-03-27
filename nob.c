#define NOB_IMPLEMENTATION
#include "nob.h"

#define OPTIMISATION 0

#define incl "./PDCurses"
#define lib "./PDCurses/x11"


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
nob_cmd_append(&cmd,
    "gcc", "-D_POSIX_C_SOURCE=200809L", "-m64", "-std=c99", "-g",
    "-I", incl,
    "-L", lib,
    "-Wl,-rpath=./PDCurses/x11",
    "-o", "main", "main.c",
#if OPTIMISATION == 1
    "-O3", "-march=native",
#endif

    "-lXCurses",
    "-lX11", "-lXext",
    "-Wall", "-Wextra", "-Wno-sign-compare"
);
    #if (OPTIMISATION == 1)
        nob_cmd_append(&cmd, "-O3", "-march=native");
    #endif
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    nob_cmd_append(&cmd, "./main", "main.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}