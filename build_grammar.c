#define NOB_IMPLEMENTATION
#include "../thirdparty/nob.h"

#define BUILD_DIR "./build"
#define LIB_NAME  BUILD_DIR"/libtslangs.a"

typedef struct {
    const char *src;
    const char *obj;
} File;

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    nob_mkdir_if_not_exists(BUILD_DIR);

    File files[] = {
        {"./tree-sitter-grammar/tree-sitter-c/src/parser.c",       BUILD_DIR"/c_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-json/src/parser.c",    BUILD_DIR"/json_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-python/src/parser.c",  BUILD_DIR"/py_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-python/src/scanner.c", BUILD_DIR"/py_scanner.o"},
        {"./tree-sitter-grammar/tree-sitter-c-sharp/src/parser.c", BUILD_DIR"/cs_parser.o"},
        {"./tree-sitter-grammar/tree-sitter-c-sharp/src/scanner.c",BUILD_DIR"/cs_scanner.o"},
    };

    size_t count = sizeof(files)/sizeof(files[0]);

    Nob_Cmd cmd = {0};

    for (size_t i = 0; i < count; i++) {
        if (nob_needs_rebuild1(files[i].obj, files[i].src)) {
            nob_cmd_append(&cmd,
                "gcc",
                "-c",
                "-O3",
                "-fPIC",
                "-I", "./tree-sitter/lib/include",
                files[i].src,
                "-o", files[i].obj
            );

            if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
        }
    }

    bool rebuild_lib = false;
    for (size_t i = 0; i < count; i++) {
        if (nob_needs_rebuild1(LIB_NAME, files[i].obj)) {
            rebuild_lib = true;
            break;
        }
    }

    if (rebuild_lib) {
        nob_cmd_append(&cmd, "ar", "rcs", LIB_NAME);

        for (size_t i = 0; i < count; i++) {
            nob_cmd_append(&cmd, files[i].obj);
        }

        if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    }

    return 0;
}