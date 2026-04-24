#define NOB_IMPLEMENTATION
#include "nob.h"

#define OPTIMISATION 0
#define COMPILE_PROFILING  1


#define incl "./PDCurses"
#ifdef _WIN32
    #define lib "./PDCurses/wincon"
    #define grammar_file_name "build_grammar.exe"
    #define output_file_name "main.exe"
#else
    #define lib "./PDCurses/x11"
    #define grammar_file_name "build_grammar"
    #define output_file_name "main"
#endif



void generate_compile_flags(Nob_Cmd* cmd){
    if(nob_file_exists("compile_flags.txt")){
        nob_delete_file("compile_flags.txt");
    }
    FILE* f = fopen("compile_flags.txt", "w");
    if(!f){
        exit(1);
    }
    for(size_t i = 1; i < cmd->count; i++){
        fprintf(f, "%s\n", cmd->items[i]);
    }
    fclose(f);
}


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    // Build the NOB script for build_grammar if it isnt already build
    if(!nob_file_exists("./"grammar_file_name)){
        nob_cmd_append(&cmd, "gcc", "build_grammar.c", "-o", grammar_file_name);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    // Runs and compiles all TS grammar if neede
    nob_cmd_append(&cmd, "./"grammar_file_name);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Build Ccode-editor
    nob_cmd_append(&cmd,
        "gcc", "-D_POSIX_C_SOURCE=200809L", "-m64", "-std=c11", "-g",
        "-I", incl,
        "-L", lib,
        "-I", "./tree-sitter/lib/include",
        "-L", "./tree-sitter",
        #ifndef _WIN32
        "-Wl,-rpath=./PDCurses/x11",
        "-Wl,-rpath=./tree-sitter",
        #endif
        "-o", output_file_name, "main.c", "./tiny_queue/tiny_queue.c",
    #if(OPTIMISATION == 1)
        "-O3", "-march=native",
    #endif
    #if(COMPILE_PROFILING == 1)
        "-DCOMPILE_PROFILING=1",
    #endif
        "./tree-sitter/libtree-sitter.a",
        "./build/libtslangs.a",
    #ifdef _WIN32 
        "-lpdcurses", "-lwinmm",
    #else
        "-lXCurses",
        "-lX11", "-lXext",
    #endif
        "-Wall", "-Wextra", "-Wno-sign-compare"
    );
    #if (OPTIMISATION == 1)
        nob_cmd_append(&cmd, "-O3", "-march=native");
    #endif
    generate_compile_flags(&cmd);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Runs Ccode-editor
    nob_cmd_append(&cmd, "./"output_file_name, "main.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}