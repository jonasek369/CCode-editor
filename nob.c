#define NOB_IMPLEMENTATION
#include "nob.h"

// Usage: ./nob [-release] [-windows] [-valgrind | -callgrind | -autostart]
//   -release  optimised build without the profiler
//   -windows  build main.exe with mingw (on Linux this is a cross-compile,
//             -autostart then runs the result through wine)

#define mingw_compiler "x86_64-w64-mingw32-gcc"

#ifdef _WIN32
    #define HOST_WINDOWS true
    #define host_compiler mingw_compiler
    #define grammar_file_name "build_grammar.exe"
#else
    #define HOST_WINDOWS false
    #define host_compiler "cc"
    #define grammar_file_name "build_grammar"
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

    bool release = false;
    bool target_windows = HOST_WINDOWS;
    bool valgrind = false, callgrind = false, autostart = false;
    for(int i = 1; i < argc; i++){
        if     (strcmp(argv[i], "-release") == 0)            release = true;
        else if(strcmp(argv[i], "-windows") == 0)            target_windows = true;
        else if(strncmp(argv[i], "-valgrind", 9) == 0)       valgrind = true;
        else if(strncmp(argv[i], "-callgrind", 10) == 0)     callgrind = true;
        else if(strncmp(argv[i], "-autostart", 10) == 0)     autostart = true;
    }

    // Building a windows exe on a non windows host (run it with wine)
    bool cross = target_windows && !HOST_WINDOWS;

    const char *compiler    = target_windows ? mingw_compiler : host_compiler;
    const char *win_dir     = "./win-build";
    const char *output_file = cross ? "./win-build/main.exe" : target_windows ? "./main.exe" : "./main";
    const char *grammar_lib = cross ? "./win-build/libtslangs.a" : "./build/libtslangs.a";
    if(cross) nob_mkdir_if_not_exists(win_dir);

    // Build the NOB script for build_grammar if it isnt already build.
    // It always runs on the host, so it is always built with the host compiler.
    if(!nob_file_exists("./"grammar_file_name)){
        nob_cmd_append(&cmd, host_compiler, "build_grammar.c", "-o", grammar_file_name);
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    // Runs and compiles all TS grammar if neede
    nob_cmd_append(&cmd, "./"grammar_file_name);
    if(target_windows) nob_cmd_append(&cmd, "-windows");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Build Ccode-editor
    nob_cmd_append(&cmd,
        compiler, "-D_DEFAULT_SOURCE", "-m64", "-std=c11", "-g",
        "-I", "./tree-sitter/lib/include",
        "-L", "./tree-sitter");

    if(!target_windows){
        nob_cmd_append(&cmd, "-Wl,-rpath=./tree-sitter");
    }

    // Cross build: windows ncurses (curses.h, libncursesw.a) can't come from the
    // linux system, so it is looked up in ./win-deps (see README note in nob.c)
    if(cross){
        nob_cmd_append(&cmd,
            "-I", "./win-deps/include",
            "-I", "./win-deps/include/ncursesw",
            "-L", "./win-deps/lib");
    }

    nob_cmd_append(&cmd, "-o", output_file, "main.c", "./tiny_queue/tiny_queue.c");

    if(release){
        nob_cmd_append(&cmd, "-O3", "-march=native", "-flto=auto");
    }else{
        nob_cmd_append(&cmd, "-DCOMPILE_PROFILING=1");
    }

    // When cross compiling, libtree-sitter is built by build_grammar into grammar_lib
    if(!cross) nob_cmd_append(&cmd, "./tree-sitter/libtree-sitter.a");
    nob_cmd_append(&cmd, grammar_lib);

    if(target_windows){
        nob_cmd_append(&cmd, "-lncursesw", "-lwinmm");
    }else{
        nob_cmd_append(&cmd, "-lncursesw");
    }

    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-Wno-sign-compare");

    if(!cross) generate_compile_flags(&cmd);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    if(cross){
        const char *dlls[] = {"libncursesw6.dll", "libwinpthread-1.dll", "libssp-0.dll"};
        const char *dirs[] = {"./win-deps/bin", "/usr/x86_64-w64-mingw32/bin"};
        for(size_t d = 0; d < sizeof(dlls)/sizeof(dlls[0]); d++){
            const char *dst = nob_temp_sprintf("%s/%s", win_dir, dlls[d]);
            if(nob_file_exists(dst)) continue;
            bool found = false;
            for(size_t k = 0; k < sizeof(dirs)/sizeof(dirs[0]) && !found; k++){
                const char *src = nob_temp_sprintf("%s/%s", dirs[k], dlls[d]);
                if(nob_file_exists(src)) found = nob_copy_file(src, dst);
            }
            if(!found) nob_log(NOB_WARNING, "Could not find %s, wine will fail to start main.exe", dlls[d]);
        }
    }

    if(valgrind || callgrind){
        if(cross){
            nob_log(NOB_ERROR, "-valgrind/-callgrind can't be used with a -windows cross build");
            return 1;
        }
        if(valgrind){
            // Runs valgrind to check for memory leaks
            nob_cmd_append(&cmd, "valgrind", "--leak-check=full", output_file);
        }else{
            nob_cmd_append(&cmd, "valgrind", "--tool=callgrind", "--dump-instr=yes", output_file);
        }
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }
    else if(autostart){
        // Runs Ccode-editor (through wine when it is a windows exe on linux)
        if(cross) nob_cmd_append(&cmd, "wine");
        nob_cmd_append(&cmd, output_file, "main.c");
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    return 0;
}