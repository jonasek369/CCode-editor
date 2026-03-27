#ifndef _H_TOKENIZER
#define _H_TOKENIZER


#include "defines.h"

typedef struct {
    char* key;
    CommandType value;
} CommandMap;

static CommandMap* commands = NULL;


void add_command(const char* command, CommandType type){
    char* key = strdup(command);
    shput(commands, key, type);
}

void init_commands(){
    add_command(":o",    COMMAND_OPEN);
    add_command(":q",    COMMAND_QUIT);
    add_command(":gt",   COMMAND_GOTO);
    add_command(":s",    COMMAND_SAVE);
    add_command(":w",    COMMAND_WRITE);
    add_command(":chn",  COMMAND_CHANGE_NAME);
    add_command(":sys",  COMMAND_SYS);
    add_command(":f",    COMMAND_FIND);
    add_command(":c",    COMMAND_CLOSE);
    add_command(":c!",   COMMAND_FORCE_CLOSE);
    add_command(":tree", COMMAND_TREE);
    add_command(":ts",   COMMAND_SET_TAB_SIZE);
    add_command(":cd",   COMMAND_TREE_CHANGE_DIR);
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
        !(isdigit(ts->source[ts->index]) || ts->source[ts->index] == '-' ) &&
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
    out->integer = atoin(ts->source+start, ts->index-start);
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