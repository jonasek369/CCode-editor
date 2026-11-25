#ifndef _H_DEFINE
#define _H_DEFINE
// Some keys are not define in curses.h or arent right
#define CUSTOM_KEY_BACKSPACE 8
#define CUSTOM_KEY_ENTER 13
#define CUSTOM_KEY_ESCAPE 27
#define CUSTOM_CTL_F 6
#define CUSTOM_CTL_S 19
#define CUSTOM_KEY_TAB 9

#define LIMIT_FPS 0 
#define FPS 240
#define FRAME_NS (1000000000L / FPS)
// How long getch waits for a key before the main loop checks LSP messages again
#define INPUT_TIMEOUT_MS 16

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <errno.h>
#include <strings.h>
#include <sys/stat.h>
#include <tree_sitter/api.h>

#include <curses.h>

// Keys PDCurses defined but ncurses does not. ncurses reports these either
// as extended key codes (> KEY_MAX) or raw bytes, read_key() in ccode_keys.h
// translates them into the values below.
// Base is well past KEY_MAX and the extended codes ncurses hands out
#define CCODE_KEY_BASE 0x1000
#define CTL_TAB    (CCODE_KEY_BASE + 0)
#define CTL_BKSP   (CCODE_KEY_BASE + 1)
#define CTL_UP     (CCODE_KEY_BASE + 2)
#define CTL_DOWN   (CCODE_KEY_BASE + 3)
#define CTL_LEFT   (CCODE_KEY_BASE + 4)
#define CTL_RIGHT  (CCODE_KEY_BASE + 5)
#define CTL_PGUP   (CCODE_KEY_BASE + 6)
#define CTL_PGDN   (CCODE_KEY_BASE + 7)
#define CTL_HOME   (CCODE_KEY_BASE + 9)
#define CTL_END    (CCODE_KEY_BASE + 10)
#define CTL_SHOME  (CCODE_KEY_BASE + 11)
#define CTL_SEND   (CCODE_KEY_BASE + 12)
#define CTL_SLEFT  (CCODE_KEY_BASE + 13)
#define CTL_SRIGHT (CCODE_KEY_BASE + 14)
#define CTL_DEL    (CCODE_KEY_BASE + 15)
#define ALT_UP     (CCODE_KEY_BASE + 16)
#define ALT_DOWN   (CCODE_KEY_BASE + 17)
#define ALT_LEFT   (CCODE_KEY_BASE + 18)
#define ALT_RIGHT  (CCODE_KEY_BASE + 19)
#define KEY_PASTE_BEGIN (CCODE_KEY_BASE + 20)
#define KEY_PASTE_END   (CCODE_KEY_BASE + 21)
// A bracketed paste was read, the text is in pasted_text
#define KEY_PASTE       (CCODE_KEY_BASE + 22)
// Alt+<key> arrives as ESC followed by the key
#define ALT_KEY(c) (CCODE_KEY_BASE + 0x100 + (c))
#define ALT_I      ALT_KEY('i')
#define CTRL_KEY(c) ((c) & 0x1f)
// ncurses calls shift+up/down "scroll reverse"/"scroll forward"
#define KEY_SUP    KEY_SR
#define KEY_SDOWN  KEY_SF


// Needed for hmput to work using stb_ds
#if defined(__GNUC__) || defined(__clang__)
    #define typeof __typeof__
#endif

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

#include "utils.h"
#include "config.h"
#include "clipboard.h"

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

typedef enum {
    LAYER_CODE = 0,
    LAYER_CONSOLE,
    LAYER_DIR_WALK,
    LAYER_THEME_SELECTOR,
    LAYER_SPLIT_VIEW,
    LAYER_FLOATING_TREE,
    LAYER_FLOATING_DIALOG
} LayerType;


typedef enum {
    TOKEN_INVALID = 0,
    TOKEN_COMMAND,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_UNKNOWN
} ConsoleTokenType;


typedef enum {
    COMMAND_INVALID = 0,
    COMMAND_OPEN,
    COMMAND_QUIT,
    COMMAND_GOTO,
    COMMAND_SAVE,
    COMMAND_WRITE,
    COMMAND_WRITE_QUIT,
    COMMAND_CHANGE_NAME,
    COMMAND_SYS,
    COMMAND_FIND,
    COMMAND_CLOSE,
    COMMAND_FORCE_CLOSE,
    COMMAND_TREE,
    COMMAND_TREE_CHANGE_DIR,
    COMMAND_PROFILING,
    COMMAND_THEME,
    COMMAND_MAKE_DIRECTORY,
    COMMAND_MAKE_FILE,
    COMMAND_DELETE_FILE,
    COMMAND_SPLIT_VIEW,
    COMMAND_FLOATING_WINDOW,
    COMMAND_CONFIG,
    COMMAND_DIAGNOSTICS,
    COMMAND_MOVE_MODE,
    COMMAND_FORCE_QUIT,
    COMMAND_NEW,
    COMMAND_REPLACE,
    COMMAND_HELP,
    COMMAND_SET,
    COMMAND_RELOAD,
    COMMAND_WRITE_ALL,
    COMMAND_COUNT
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
#define default_filename_length 8
static char* default_filename = "untitled";

typedef struct {
    ConsoleTokenType type;
    CommandType command_type;
    union {
        struct {
            char* start;
            int64_t size;
        } string;
        int64_t integer;
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
    size_t* index_char_positions;
    size_t* indecies_size;
    int64_t chars_offseted;
    size_t at_index;
    size_t total_len;
} CompletionFunctionParams;

typedef struct {
    JsonValue* completion;
    size_t selected;     // index into visible
    size_t items_count;  // number of visible items
    size_t* visible;     // stb array, indices of the items matching the typed word
    int row;             // line the completion was requested on
} CompletionWindow;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} VirtualWindow;

static VirtualWindow default_virtual_window = {.x = 0, .y = 1, .width = 0, .height = 0};

bool is_inside_virtual_window(int x, int y, VirtualWindow* w){
    return (
        x >= w->x &&
        x <  w->x + w->width &&
        y >= w->y &&
        y <  w->y + w->height
    );
}


// One reversible buffer edit. Edits made by one key press share a group and are
// undone together
typedef struct {
    bool   insert;    // true: text was inserted at row/col, false: text was deleted from there
    int    row;
    int    col;
    char*  text;      // malloc'd, len bytes, may contain '\n'
    size_t len;
    int    cursor_x;  // cursor before the edit, restored on undo
    int    cursor_y;
    int    group;
} UndoEntry;

typedef struct {
    bool allow_diagnostics;
    bool saved;
    // buffer changed since the last reparse and LSP didChange
    bool pending_edit;
    char* filename;
    char** code_buffer;
    Cursor* cursor;
    Cursor* select_end;
    FindingSubstr* finding_substr;
    CompletionFunctionParams* completion_function_params;
    VirtualWindow* virtual_window;

    // tree sitter
    SyntaxLanguage lang;
    TSParser* parser;
    TSTree* tree;
    TSQuery* query;

    // LSP
    char* uri;
    int version;
    int id;
    JsonValue* diagnostics; // object
    LSPRange** ranges;
    CompletionWindow* completion_window;

    // Undo / redo
    UndoEntry* undo_stack;     // stb array
    UndoEntry* redo_stack;     // stb array
    int  undo_group;           // group new edits are recorded into
    int  saved_undo_len;       // undo_stack length at the last save, -1 when unreachable
    bool undo_replaying;       // set while undo/redo applies edits so they are not recorded
    bool typing_run;           // last edit was plain typing, consecutive typing undoes as one

    // File format, restored on save
    bool crlf;                 // file used \r\n line endings
    bool use_tabs;             // indentation unit is '\t'
    bool read_only;            // could not be written last time

    int  preferred_col;        // display column kept while moving up/down, -1 when unset
    long long mtime;           // modification time of the file when loaded/saved

    int  tab_order;            // creation order, the tab bar and F1-F9 are sorted by it
    bool is_welcome;           // welcome / help text, not a file the user has to save
    int  last_cx, last_cy;     // cursor at the last render, the view follows it when it moves
    bool final_newline;        // the file ended with a newline when it was loaded
} LayerCodeData;

typedef struct {
    char* console_buffer;
    int console_buffer_x;
} LayerConsoleData;

typedef struct {
    char** current_dir_files;
    int selected;
    int offset;
} LayerDirWalkData;

typedef struct {
    char** current_dir_files;
    char* theme_dir_path; // stb_ds array
    int selected;
    int offset;
} LayerThemeSelectorData;

typedef struct {
    VirtualWindow* virtual_window;
    int selected;
    int offset;
    char* search_buffer;
    int search_buffer_x;
    char* cwd;
    char** files;
    char** filtered_files;
} LayerFloatingTreeData;

typedef struct Layer Layer;
typedef struct CCode CCode;

struct Layer {
    LayerType type;
    bool consume_input;
    bool draws_fullscreen;
    void* layer_data;
    void (*handle_keypress_function)(CCode*, Layer*, int, bool);
    bool (*update_function)(CCode*, Layer*, int);
};

typedef struct {
    bool move_mode;
    Layer** splitten_layers;         // Array of split layers 
    VirtualWindow** virtual_windows;
    int focused;                     // index into splitten_layers
} LayerSplitViewData;

struct CCode{
    Layer** layers;
    LSPContext** lsp_ctxs;
    CCodeConfig* config;
};

// Short message shown in the status bar (errors, "saved", ...)
static char   status_message[512] = {0};
static time_t status_message_time = 0;
static bool   status_message_error = false;

#if defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
void set_status(bool error, const char* fmt, ...);
void set_status(bool error, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vsnprintf(status_message, sizeof(status_message), fmt, args);
    va_end(args);
    status_message_time = time(NULL);
    status_message_error = error;
}

typedef struct {
    VirtualWindow* virtual_window;
    char*          message;     // stb_ds array string
    int            selected;    // 0 = Yes, 1 = No
    void         (*on_yes)(CCode*, void*);
    void         (*on_no )(CCode*, void*);
    void*          userdata;
} LayerFloatingDialogData;

// Lines are stb arrays ending with '\0', their length is known without strlen
static inline size_t line_length(const char* line){
    size_t len = arrlenu(line);
    return len > 0 ? len - 1 : 0;
}

// Line length without the trailing '\n'
static inline size_t line_content_length(const char* line){
    size_t len = line_length(line);
    if(len > 0 && line[len - 1] == '\n') len--;
    return len;
}

char *flatten_buffer(LayerCodeData *code){
    size_t total = 0;

    for(int i = 0; i < arrlen(code->code_buffer); i++) {
        total += line_length(code->code_buffer[i]);
    }

    char *result = malloc(total + 1);
    size_t pos = 0;

    for (int i = 0; i < arrlen(code->code_buffer); i++) {
        size_t len = line_length(code->code_buffer[i]);
        memcpy(result + pos, code->code_buffer[i], len);
        pos += len;
    }

    result[pos] = '\0';
    return result;
}


#define LABEL(x) x:

#include "profiling.h"
#include "syntax_highlighting.h"

// used on definition find
void push_layer_to_top(CCode* ccode, Layer* to_top);
Layer* read_file_to_code_layer(CCode* ccode, const char* filepath_start, size_t size, bool add_to_ccode);

// declaring callbacks that needs to be defined for layers.h
void file_remove_callback(CCode* ccode, void* data);
void file_not_remove_callback(CCode* ccode, void* data);
void file_on_save_callback(CCode* ccode, void* data);

// Used for LSP
void file_on_open_callback(CCode* ccode, void* data);
void file_on_close_callback(CCode* ccode, void* data);
void file_on_change_callback(CCode* ccode, void* data);
void file_on_type_callback(CCode* ccode, void* data);

bool file_before_save_callback(CCode* ccode, void* data);
void file_find_definition_callback(CCode* ccode, void* data);

#include "layers.h"
#include "lsp_handler.h"
#include "callbacks.h"

void free_ccode(CCode* ccode){
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        free_layer(ccode->layers[i]);
    }
    arrfree(ccode->layers);
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            destroy_lsp(ccode->lsp_ctxs[i]);
        }
        arrfree(ccode->lsp_ctxs);
    }
    if(ccode->config){
        free_config(ccode->config);
    }
}


#endif