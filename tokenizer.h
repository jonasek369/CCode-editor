#ifndef _H_TOKENIZER
#define _H_TOKENIZER


#include "defines.h"

typedef struct {
    char* key;
    CommandType value;
} CommandMap;

// used for the welcome screen
#define LONGEST_COMMAND 7

static CommandMap* commands = NULL;
// Description of each command, indexed by CommandType - 1
static char* commands_info[COMMAND_COUNT] = {0};

typedef struct {
    const char* key;
    CommandType type;
    const char* info;
} CommandDef;

static const CommandDef command_defs[] = {
    {":o",      COMMAND_OPEN,            "<path>             | Opens a file (creates it when saved if it doesn't exist)"},
    {":new",    COMMAND_NEW,             "                   | Opens a new empty buffer"},
    {":w",      COMMAND_WRITE,           "                   | Saves the current file"},
    {":s",      COMMAND_SAVE,            "<path>             | Saves the current file under a new path"},
    {":wa",     COMMAND_WRITE_ALL,       "                   | Saves all modified files"},
    {":wq",     COMMAND_WRITE_QUIT,      "                   | Saves the current file and quits"},
    {":q",      COMMAND_QUIT,            "                   | Quits (asks first when files are unsaved)"},
    {":q!",     COMMAND_FORCE_QUIT,      "                   | Quits and discards unsaved changes"},
    {":c",      COMMAND_CLOSE,           "                   | Closes the current file"},
    {":c!",     COMMAND_FORCE_CLOSE,     "                   | Closes the current file, discarding changes"},
    {":reload", COMMAND_RELOAD,          "                   | Rereads the current file from disk"},
    {":gt",     COMMAND_GOTO,            "<line> <column>    | Jumps to a line (and column)"},
    {":f",      COMMAND_FIND,            "<text>             | Finds text, arrows jump between matches"},
    {":r",      COMMAND_REPLACE,         "<find> <replace>   | Replaces every occurrence in the file"},
    {":chn",    COMMAND_CHANGE_NAME,     "<name>             | Changes the current file name"},
    {":set",    COMMAND_SET,             "<option> [value]   | Changes an editor option (:set alone lists them)"},
    {":sys",    COMMAND_SYS,             "<command>          | Executes a system command"},
    {":tree",   COMMAND_TREE,            "[path]             | Opens the file tree"},
    {":cd",     COMMAND_TREE_CHANGE_DIR, "<path>             | Changes the file tree directory"},
    {":ft",     COMMAND_FLOATING_WINDOW, "                   | Opens the fuzzy file finder"},
    {":mkfile", COMMAND_MAKE_FILE,       "<filename>         | Creates a new file in tree"},
    {":mkdir",  COMMAND_MAKE_DIRECTORY,  "<dirname>          | Creates a new directory in tree"},
    {":rm",     COMMAND_DELETE_FILE,     "<path>             | Deletes a file in tree"},
    {":sw",     COMMAND_SPLIT_VIEW,      "[a] [b]            | Splits two open files side by side"},
    {":mv",     COMMAND_MOVE_MODE,       "                   | Toggles resizing of the split view"},
    {":theme",  COMMAND_THEME,           "[path]             | Opens the theme picker"},
    {":conf",   COMMAND_CONFIG,          "                   | Opens the configuration file"},
    {":diag",   COMMAND_DIAGNOSTICS,     "                   | Toggles diagnostics in the focused file"},
    {":prof",   COMMAND_PROFILING,       "                   | Toggles the profiler overlay"},
    {":help",   COMMAND_HELP,            "                   | Shows keyboard shortcuts and commands"},
};

void add_command(const char* command, CommandType type){
    char* key = strdup(command);
    shput(commands, key, type);
}

void init_commands(){
    for(size_t i = 0; i < sizeof(command_defs)/sizeof(command_defs[0]); i++){
        add_command(command_defs[i].key, command_defs[i].type);
        commands_info[command_defs[i].type - 1] = (char*)command_defs[i].info;
    }
}

void destroy_commands(){
    for(size_t i = 0; i < shlenu(commands); i++){
        CommandMap cm = commands[i];
        free(cm.key);
    }

    shfree(commands);
}

void parse_command(TokenizerState* ts, Token* out){
    out->type = TOKEN_COMMAND;
    char* start = ts->source + ts->index;
    char* end = start;
    while((*end != '\0' && !is_whitespace[(uint8_t)*end]) || *end == ':'){
        end++;
    }
    size_t command_length = end-start; // including :
    if(command_length <= 1){
        out->command_type = COMMAND_INVALID;
    }
    out->string.start = start;
    out->string.size = command_length;

    ts->index += command_length; // move index

    char* cmd_str = stringview_to_str(start, command_length);

    ptrdiff_t cmd_index = shgeti(commands, cmd_str);
    if(cmd_index == -1){
        out->command_type = COMMAND_INVALID;
        free(cmd_str);
        return;
    }
    CommandMap type = commands[cmd_index];
    out->command_type = type.value;
    free(cmd_str);
}


void parse_string(TokenizerState* ts, Token* out) {
    size_t start = ++ts->index; // skip "

    while (ts->source[ts->index] != '"' && ts->source[ts->index] != '\0') {
        ts->index++;
    }

    out->type = TOKEN_STRING;
    out->string.start = ts->source + start;
    out->string.size = ts->index-start;

    if(ts->source[ts->index] == '"'){
        ts->index++;
    }
}


void parse_unquoted_string(TokenizerState* ts, Token* out){
    size_t start = ts->index;

    while(
        ts->source[ts->index] != '\0'                                      &&
        !is_whitespace[(uint8_t)ts->source[ts->index]]                     &&
        (start != ts->index || !(isdigit(ts->source[ts->index]) || ts->source[ts->index] == '-' )) &&
        ts->source[ts->index] != '"'                                       
        ){
        ts->index++;
    }
    int32_t size = ts->index-start;
    if(size == 0){
        out->type = TOKEN_INVALID;
        return;
    }

    out->type = TOKEN_STRING;
    out->string.start = ts->source + start;
    out->string.size = ts->index-start;
}

void parse_number(TokenizerState* ts, Token* out){
    size_t start = ts->index;
    ts->index++; // In case the value had - we skip first characters
    while(isdigit(ts->source[ts->index]) && ts->source[ts->index] != '\0'){
        ts->index++;
    }
    out->type = TOKEN_INTEGER;
    out->integer = atoint64(ts->source+start, ts->index-start);
}

int next_token(TokenizerState* ts, Token* out){
    while (is_whitespace[(uint8_t)ts->source[ts->index]]) {
        ts->index++;
    }
    out->type = TOKEN_INVALID;

    char c = ts->source[ts->index];

    if(isdigit(c) || c == '-'){
        parse_number(ts, out);
        return 0;
    }else if(c == '"'){
        parse_string(ts, out);
        return 0;
    }else if(c == ':'){
        size_t pre_parse = ts->index;
        parse_command(ts, out);
        if(out->command_type != COMMAND_INVALID){
            return 0;                
        }
        ts->index = pre_parse; // Rollback buffer iteration
        return 1; // Invalid command or separator
    }else {
        parse_unquoted_string(ts, out);
        return out->type == TOKEN_INVALID;
    }

    return out->type == TOKEN_INVALID;
}


int tokenize_console_command(const char* buffer, TokenizationOutput* out){
    out->tokens = NULL;
    size_t buffer_size = strlen(buffer);
    out->string_storage = malloc(buffer_size+1);
    if(!out->string_storage){
        return 1;
    }
    strcpy(out->string_storage, buffer);
    TokenizerState ts = {0};
    ts.source = out->string_storage;
    ts.index = 0;

    while(ts.source[ts.index] != '\0'){
        Token tok = {0};
        if(!next_token(&ts, &tok)){
            arrput(out->tokens, tok);
        }else{
            if(ts.source[ts.index] != '\0'){
                ts.index++;                
            }
        }
    }

    return 0;
}


#endif