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
    "gcc", "-D_POSIX_C_SOURCE=200809L", "-m64", "-std=c11", "-g",
    "-I", incl,
    "-L", lib,
    "-I", "./tree-sitter/lib/include",
    "-L", "./tree-sitter",
    "-Wl,-rpath=./PDCurses/x11",
    "-Wl,-rpath=./tree-sitter",
    "-o", "main", "main.c",
    "./tree-sitter-grammar/tree-sitter-c/src/parser.c",
    "./tree-sitter-grammar/tree-sitter-json/src/parser.c",
    "./tree-sitter-grammar/tree-sitter-python/src/parser.c",
    "./tree-sitter-grammar/tree-sitter-python/src/scanner.c",
#if(OPTIMISATION == 1)
    "-O3", "-march=native",
#endif
    "./tree-sitter/libtree-sitter.a",
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