#ifndef _H_TOKENIZER
#define _H_TOKENIZER


#include "defines.h"

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

	if(command_length == 2 && strncmp(start, ":q", 2) == 0){
		out->command_type = COMMAND_QUIT;
		return;
	}else if(command_length == 2 && strncmp(start, ":w", 2) == 0){
		out->command_type = COMMAND_WRITE;
		return;
	}else if(command_length == 4 && strncmp(start, ":chn", 4) == 0){
		out->command_type = COMMAND_CHANGE_NAME;
		return;
	}else if(command_length == 4 && strncmp(start, ":sav", 4) == 0){
		out->command_type = COMMAND_SAVE;
		return;
	}else if(command_length == 3 && strncmp(start, ":gt", 3) == 0){
		out->command_type = COMMAND_GOTO;
		return;
	}else if(command_length == 4 && strncmp(start, ":sys", 4) == 0){
		out->command_type = COMMAND_SYS;
		return;
	}else if(command_length == 2 && strncmp(start, ":o", 2) == 0){
		out->command_type = COMMAND_OPEN;
		return;
	}else if(command_length == 2 && strncmp(start, ":f", 2) == 0){
		out->command_type = COMMAND_FIND;
		return;
	}

	out->command_type = COMMAND_INVALID;
	return;
}

int32_t atoin(const char *str, int n) {
    int32_t result = 0;
    int32_t sign = 1;
    int i = 0;

    if(str[0] != '\0' && str[0] == '-'){
    	sign = -1;
    	i++;
    }

    while (i < n && str[i] != '\0') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
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
		ts->source[ts->index] != '\0' 									   &&
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
			ts.index++;
		}
	}

	return 0;
}


#endif