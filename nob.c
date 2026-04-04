#define NOB_IMPLEMENTATION
#include "../thirdparty/nob.h"

#define OPTIMISATION 0

#define incl "./PDCurses"
#define lib "./PDCurses/x11"


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    // Build the NOB script for build_grammar if it isnt already build
    if(!nob_file_exists("./build_grammar")){
        nob_cmd_append(&cmd, "gcc", "build_grammar.c", "-o", "build_grammar");
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    // Runs and compiles all TS grammar if neede
    nob_cmd_append(&cmd, "./build_grammar");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Build Ccode-editor
    nob_cmd_append(&cmd,
        "gcc", "-D_POSIX_C_SOURCE=200809L", "-m64", "-std=c11", "-g",
        "-I", incl,
        "-L", lib,
        "-I", "./tree-sitter/lib/include",
        "-L", "./tree-sitter",
        "-Wl,-rpath=./PDCurses/x11",
        "-Wl,-rpath=./tree-sitter",
        "-o", "main", "main.c", "../tiny_queue/tiny_queue.c",
    #if(OPTIMISATION == 1)
        "-O3", "-march=native",
    #endif
        "./tree-sitter/libtree-sitter.a",
        "./build/libtslangs.a",
        "-lXCurses",
        "-lX11", "-lXext",
        "-Wall", "-Wextra", "-Wno-sign-compare"
    );
    #if (OPTIMISATION == 1)
        nob_cmd_append(&cmd, "-O3", "-march=native");
    #endif
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Runs Ccode-editor
    nob_cmd_append(&cmd, "./main", "main.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}