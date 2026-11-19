#define NOB_IMPLEMENTATION
#include "nob.h"


#ifdef _WIN32
    #define grammar_file_name "build_grammar.exe"
    #define output_file_name "main.exe"
#else
    #define grammar_file_name "build_grammar"
    #define output_file_name "main"
#endif


void generate_compile_flags(Nob_Cmd* cmd){
    if(nob_file_exists("compile_flags.txt")){
        nob_delete_file("compile_flags.txt");
    }
    FILE* f = fopen("compile_flags.txt", "w");
    if(!f){
        fprintf(stderr, "Could not open file to generate 'compile_flags.txt'\n");
        return;
    }
    for(size_t i = 1; i < cmd->count; i++){
        fprintf(f, "%s\n", cmd->items[i]);
    }
    fclose(f);
}


int main(int argc, char **argv){
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};

    // -release builds with optimisations and without the profiler
    bool release = false;
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-release") == 0) release = true;
    }
    // Build the NOB script for build_grammar if it isnt already build
    if(!nob_file_exists("./"grammar_file_name)){
        nob_cmd_append(&cmd, "cc", "build_grammar.c", "-o", grammar_file_name);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    // Runs and compiles all TS grammar if neede
    nob_cmd_append(&cmd, "./"grammar_file_name);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Build Ccode-editor
    nob_cmd_append(&cmd,
        "cc", "-D_DEFAULT_SOURCE", "-m64", "-std=c11", "-g",
        "-I", "./tree-sitter/lib/include",
        "-L", "./tree-sitter");

    #ifndef _WIN32
        nob_cmd_append(&cmd, "-Wl,-rpath=./tree-sitter");
    #endif
    
    nob_cmd_append(&cmd, "-o", output_file_name, "main.c", "./tiny_queue/tiny_queue.c");
    
    if(release){
        nob_cmd_append(&cmd, "-O3", "-march=native", "-flto=auto");
    }else{
        nob_cmd_append(&cmd, "-DCOMPILE_PROFILING=1");
    }
    
    nob_cmd_append(&cmd, "./tree-sitter/libtree-sitter.a", "./build/libtslangs.a");
    
    #ifdef _WIN32
        nob_cmd_append(&cmd, "-lncursesw", "-lwinmm");
    #else
        nob_cmd_append(&cmd, "-lncursesw");
    #endif

    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-Wno-sign-compare");
    
    generate_compile_flags(&cmd);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    if(argc >= 2 && strncmp(argv[1], "-valgrind", 9) == 0){
        // Runs valgrind to check for memory leaks
        nob_cmd_append(&cmd, "valgrind", "--leak-check=full", "./"output_file_name);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }
    else if(argc >= 2 && strncmp(argv[1], "-callgrind", 10) == 0){
        nob_cmd_append(&cmd, "valgrind", "--tool=callgrind", "--dump-instr=yes", "./"output_file_name);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }
    else if(argc >= 2 && strncmp(argv[1], "-autostart", 10) == 0){
        // Runs Ccode-editor
        nob_cmd_append(&cmd, "./"output_file_name, "main.c");
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    return 0;
}