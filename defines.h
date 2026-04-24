#ifndef _H_DEFINE
#define _H_DEFINE
// Some keys are not define in curses.h or arent right
#define CUSTOM_KEY_BACKSPACE 8
#define CUSTOM_KEY_ENTER 13
#define CUSTOM_KEY_ESCAPE 27
#define CUSTOM_CTL_F 6
#define CUSTOM_CTL_S 19

#define FPS 240
#define FRAME_NS (1000000000L / FPS)

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <tree_sitter/api.h>

#include "curses.h"
#include "curspriv.h"


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define CJSON_NO_STB_DS
#include "parser.h"
#include "./tiny_queue/tiny_queue.h"


#define LSP_NO_STB_DS
#define LSP_NO_CJSON
#define LSP_NO_TINY_QUEUE
#include "LSP.h"
    
#define NOB_IMPLEMENTATION
#include "nob.h"

#include "config.h"
#include "utils.h"


#ifdef __linux__
    #ifndef MAX_PATH
        #ifdef PATH_MAX
            #define MAX_PATH PATH_MAX
        #else
            // fallback
            #define MAX_PATH 4096
        #endif
    #endif
#endif

#ifndef MAX_PATH
    #define MAX_PATH 4096
#endif

typedef enum {LAYER_CODE=0, LAYER_CONSOLE, LAYER_DIR_WALK, LAYER_THEME_SELECTOR} LayerType;
typedef enum {TOKEN_INVALID = 0, TOKEN_COMMAND, TOKEN_STRING, TOKEN_INTEGER, TOKEN_UNKNOWN} ConsoleTokenType;
typedef enum {
    COMMAND_INVALID = 0,
    COMMAND_OPEN,
    COMMAND_QUIT,
    COMMAND_GOTO,
    COMMAND_SAVE,
    COMMAND_WRITE,
    COMMAND_CHANGE_NAME,
    COMMAND_SYS,
    COMMAND_FIND,
    COMMAND_CLOSE,
    COMMAND_FORCE_CLOSE,
    COMMAND_TREE,
    COMMAND_SET_TAB_SIZE,
    COMMAND_TREE_CHANGE_DIR,
    COMMAND_PROFILING,
    COMMAND_THEME,
    COMMAND_WRITE_CONFIG,
    COMMAND_MAKE_DIRECTORY,
    COMMAND_MAKE_FILE
} CommandType;

typedef enum {
    LANG_UNKNOWN=0,
    LANG_C,
    LANG_JSON,
    LANG_PYTHON,
    LANG_C_SHARP,
    LANG_RUST
} SyntaxLanguage;

static const bool is_whitespace[256] = {
  [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1
};

typedef struct {
    ConsoleTokenType type;
    CommandType command_type;
    union {
        struct {
            char* start;
            int32_t size;
        } string;
        int32_t integer;
    };
} Token;

typedef struct {
    Token* tokens;
    char* string_storage;
} TokenizationOutput;

typedef struct {
    char* source;
    size_t index;
} TokenizerState;

typedef struct {
    int x;
    int y;
    int xoff;
    int yoff;
} Cursor;

typedef struct {
    int start_character;
    int start_line;

    int end_character;
    int end_line;
} LSPRange;

typedef struct {
    char* substr;
    size_t at_nth_occurence;
} FindingSubstr;

typedef struct {
    JsonValue* completion;
    size_t selected;
    size_t items_count;
} CompletionWindow;

typedef struct {
    bool saved;
    char* filename;
    char** code_buffer;
    Cursor* cursor;
    FindingSubstr* finding_substr;

    // tree sitter
    SyntaxLanguage lang;
    TSParser *parser;
    TSTree *tree;

    // LSP
    char* uri;
    int version;
    int id;
    JsonValue* diagnostics; // object
    LSPRange** ranges;
    CompletionWindow* completion_window;
} LayerCodeData;


typedef struct {
    char* console_buffer;
    int console_buffer_x;
    char** history;
    int history_index;
} LayerConsoleData;

typedef struct {
    char* current_dir_path;
    char** current_dir_files;
    int selected;
    int offset;
} LayerDirWalkData;


typedef struct {
    char** current_dir_files;
    int selected;
    int offset;
}LayerThemeSelectorData;

typedef struct {
    LayerType type;
    bool consume_input;
    bool draws_fullscreen;
    void* layer_data;
} Layer;


typedef struct {
    Layer** layers;
    LSPContext** lsp_ctxs;
    CCodeConfig* config;
} CCode;

#define LABEL(x) x:

#include "profiling.h"
#include "syntax_highlighting.h"
#include "lsp_handler.h"


#endif