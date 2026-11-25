#define NOB_IMPLEMENTATION
#include "nob.h"
 
// Usage: ./build_grammar [-windows]
//   -windows  build for windows with mingw. On a non windows host this is a
//             cross-compile and everything goes to ./win-build, including
//             tree-sitter itself (it is bundled into libtslangs.a).
 
#define mingw_compiler "x86_64-w64-mingw32-gcc"
 
#ifdef _WIN32
    #define HOST_WINDOWS true
    #define host_compiler mingw_compiler
#else
    #define HOST_WINDOWS false
    #define host_compiler "cc"
#endif
 
typedef struct {
    const char *src;
    const char *obj; // file name inside the build dir
} File;
 
int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
 
    bool target_windows = HOST_WINDOWS;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-windows") == 0) target_windows = true;
    }
    bool cross = target_windows && !HOST_WINDOWS;
 
    const char *compiler  = target_windows ? mingw_compiler : host_compiler;

    const char *archiver  = cross ? "x86_64-w64-mingw32-gcc-ar" : "ar";
    const char *build_dir = cross ? "./win-build" : "./build";
    const char *lib_name  = nob_temp_sprintf("%s/libtslangs.a", build_dir);
 
    if (!cross) nob_mkdir_if_not_exists("./build");
    nob_mkdir_if_not_exists(build_dir);
 
    File files[] = {
        {"./tree-sitter-grammar/tree-sitter-c/src/parser.c",        "c_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-json/src/parser.c",     "json_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-python/src/parser.c",   "py_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-python/src/scanner.c",  "py_scanner.o"},
        {"./tree-sitter-grammar/tree-sitter-c-sharp/src/parser.c",  "cs_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-c-sharp/src/scanner.c", "cs_scanner.o"},
        {"./tree-sitter-grammar/tree-sitter-rust/src/parser.c",     "rs_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-rust/src/scanner.c",    "rs_scanner.o"},
        // Only for the cross build, the native one uses the prebuilt tree-sitter/libtree-sitter.a
        {"./tree-sitter/lib/src/lib.c",                             "ts_lib.o"},
    };
 
    size_t count = sizeof(files)/sizeof(files[0]);
    if (!cross) count -= 1;
 
    Nob_Cmd cmd = {0};
    Procs procs = {0};
    for (size_t i = 0; i < count; i++) {
        const char *obj = nob_temp_sprintf("%s/%s", build_dir, files[i].obj);
        if (nob_needs_rebuild1(obj, files[i].src)) {
            nob_cmd_append(&cmd,
                compiler,
                "-c",
                "-O3",
                "-march=native",
                "-flto=auto");
 
            if (!target_windows) {
                nob_cmd_append(&cmd, "-fPIC");
            }
 
            nob_cmd_append(&cmd,
                "-D_DEFAULT_SOURCE",
                "-I", "./tree-sitter/lib/include",
                "-I", "./tree-sitter/lib/src",
                files[i].src,
                "-o", obj);
 
            if (!cmd_run(&cmd, .async = &procs)) return 1;
        }
    }
 
    if (!procs_flush(&procs)) return 1;
 
    bool rebuild_lib = false;
    for (size_t i = 0; i < count; i++) {
        const char *obj = nob_temp_sprintf("%s/%s", build_dir, files[i].obj);
        if (nob_needs_rebuild1(lib_name, obj)) {
            rebuild_lib = true;
            break;
        }
    }
 
    if (rebuild_lib) {
        nob_cmd_append(&cmd, archiver, "rcs", lib_name);
 
        for (size_t i = 0; i < count; i++) {
            nob_cmd_append(&cmd, nob_temp_sprintf("%s/%s", build_dir, files[i].obj));
        }
 
        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }
 
    return 0;
}
 