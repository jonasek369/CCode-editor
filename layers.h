#ifndef _H_LAYERS
#define _H_LAYERS

#include "defines.h"

#include "tokenizer.h"

#define NOB_IMPLEMENTATION
#include "nob.h"


#define default_filename_length 8
static char* default_filename = "untitled";

/*

    Layer constructors

*/


Layer* new_layer_code(){
    Layer* code = malloc(sizeof(Layer));
    if(!code){
        return NULL;
    }
    code->type = LAYER_CODE;
    code->consume_input = true;
    code->draws_fullscreen = true;

    LayerCodeData* lcd = malloc(sizeof(LayerCodeData));
    if(!lcd){
        free(code);
        return NULL;
    }
    lcd->cursor = malloc(sizeof(Cursor));
    if(!lcd->cursor){
        free(code);
        free(lcd);
        return NULL;
    }
    lcd->cursor->x = 0;
    lcd->cursor->y = 0;
    lcd->cursor->xoff = 0;
    lcd->cursor->yoff = 0;

    lcd->finding_substr = NULL;

    lcd->filename = NULL;
    for(size_t i = 0; i < default_filename_length; i++){
        arrput(lcd->filename, default_filename[i]);
    }
    arrput(lcd->filename, '\0');
    lcd->code_buffer = NULL;
    lcd->saved = false;
    code->layer_data = lcd;

    lcd->parser = NULL;
    lcd->tree = NULL;
    lcd->lang = LANG_UNKNOWN;

    return code;
}


Layer* new_layer_console(){
    Layer* console = malloc(sizeof(Layer));
    if(!console){
        return NULL;
    }
    console->type = LAYER_CONSOLE;
    console->consume_input = true;
    console->draws_fullscreen = false;

    LayerConsoleData* lcd = malloc(sizeof(LayerConsoleData));
    if(!lcd){
        free(console);
        return NULL;
    }
    lcd->console_buffer = NULL;
    arrput(lcd->console_buffer, '\0');
    lcd->console_buffer_x = 0;
    console->layer_data = lcd;
    return console;
}


Layer* new_layer_dir_walk(char* dir){
    Layer* tree = malloc(sizeof(Layer));
    if(!tree){
        return NULL;
    }
    tree->type = LAYER_DIR_WALK;
    tree->consume_input = true;
    tree->draws_fullscreen = true;

    LayerDirWalkData* ldwd = malloc(sizeof(LayerDirWalkData));
    if(!ldwd){
        free(tree);
        return NULL;
    }
    ldwd->current_dir_path = dir;
    ldwd->current_dir_files = NULL;
    ldwd->selected = 0;
    ldwd->offset = 0;

    tree->layer_data = ldwd;
    return tree;
}

/*

    Layers managment helpers

*/



void push_layer_to_top(CCode* ccode, Layer* to_top){
    if(!to_top)
        return;

    size_t len = arrlenu(ccode->layers);

    if(len > 0 && ccode->layers[0] == to_top)
        return;

    for(size_t i = 0; i < len; i++){
        if(ccode->layers[i] == to_top){
            arrdel(ccode->layers, i);
            break;
        }
    }

    arrins(ccode->layers, 0, to_top);
}


void push_layer_to_bot(CCode* ccode, Layer* to_bot){
    if (arrlen(ccode->layers) > 0 && ccode->layers[arrlen(ccode->layers)-1] == to_bot) {
        return;
    }

    // Remove the layer if it is present in layers
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i] == to_bot){
            arrdel(ccode->layers, i);
            break;
        }
    }

    // insert to bot
    arrput(ccode->layers, to_bot);
}


Layer* top_layer(CCode* ccode){
    return arrlen(ccode->layers) > 0 ? ccode->layers[0] : NULL;
}


void remove_layer(CCode* ccode, Layer* target){
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i] == target){
            arrdel(ccode->layers, i);
            break;
        }
    }
}


Layer** all_type_layers(CCode* ccode, LayerType type){
    Layer** same_type_layers = NULL;
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == type) 
            arrput(same_type_layers, ccode->layers[i]);
    }
    return same_type_layers;
}


Layer* top_type_layer(CCode* ccode, LayerType type){
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == type) return ccode->layers[i];
    }
    return NULL;
}


Cursor* top_code_layer_cursor(CCode* ccode){
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE) return ((LayerCodeData*) ccode->layers[i]->layer_data)->cursor;
    }
    return NULL;
}


int contains_layer(CCode* ccode, Layer* layer){
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i] == layer) return i;
    }
    return -1;
}


Layer* layer_at_index(CCode* ccode, int index){
    return index < arrlen(ccode->layers) ? ccode->layers[index] : NULL;
}

/*

    File manipulation

*/


size_t buffer_byte_offset(LayerCodeData *code, int row, int col){
    size_t offset = 0;

    for(int i = 0; i < row; i++) {
        offset += strlen(code->code_buffer[i]);
    }

    offset += col;
    return offset;
}


char *flatten_buffer(LayerCodeData *code){
    size_t total = 0;

    for(int i = 0; i < arrlen(code->code_buffer); i++) {
        total += strlen(code->code_buffer[i]);
    }

    char *result = malloc(total + 1);
    size_t pos = 0;

    for (int i = 0; i < arrlen(code->code_buffer); i++) {
        size_t len = strlen(code->code_buffer[i]);
        memcpy(result + pos, code->code_buffer[i], len);
        pos += len;
    }

    result[pos] = '\0';
    return result;
}


// Supported parsers
extern const TSLanguage *tree_sitter_c();
extern const TSLanguage *tree_sitter_json();
extern const TSLanguage *tree_sitter_python();

const TSLanguage* get_filetype_language_parser(char* filename, SyntaxLanguage* lang){
    size_t size = strlen(filename);
    size_t i = size;
    while(i > 0 && filename[i] != '.'){
        i--;
    }
    i++; // remove ".""
    char* suffix = stringview_to_str(filename+i, size-i);
    if(strlen(suffix) == 1 && (strncmp(suffix, "c", 1) == 0 || strncmp(suffix, "h", 1) == 0)){
        free(suffix);
        *lang = LANG_C;
        return tree_sitter_c();
    }else if(strlen(suffix) == 4 && strncmp(suffix, "json", 4) == 0){
        *lang = LANG_JSON;
        free(suffix);
        return tree_sitter_json();
    }else if(strlen(suffix) == 2 && strncmp(suffix, "py", 2) == 0){
        *lang = LANG_PYTHON;
        free(suffix);
        return tree_sitter_python();
    }
    *lang = LANG_UNKNOWN;
    free(suffix);
    return NULL;
}


void make_parser(CCode* ccode, char* filename){
    Layer* layer = top_type_layer(ccode, LAYER_CODE);
    if(!layer){
        return;
    }
    LayerCodeData* lcd = layer->layer_data;
    const TSLanguage* lang = get_filetype_language_parser(filename, &(lcd->lang));
    if(!lang){
        lcd->lang = LANG_UNKNOWN;
        return;
    }
    lcd->parser = ts_parser_new();
    if(!ts_parser_set_language(lcd->parser, lang)){
        ts_parser_delete(lcd->parser);
        lcd->parser = NULL;
        fprintf(stderr, "Language version mismatch\n");
        return;
    }
    

    char* flatten = flatten_buffer(lcd);
    lcd->tree = ts_parser_parse_string(
        lcd->parser,
        NULL,
        flatten,
        strlen(flatten)
    );

    free(flatten);
}


void read_file_to_code_layer(CCode* ccode, const char* filename_start, size_t size){
    Nob_String_Builder sb = {0};
    Nob_String_Builder filename = {0};
    nob_da_append_many(&filename, filename_start, size);
    nob_da_append(&filename, '\0');

    if(!nob_read_entire_file(filename.items, &sb)){
        fprintf(stderr, "Error reading file\n");
        nob_sb_free(sb);
    }
    Layer* file_layer = new_layer_code();
    LayerCodeData* lcd = (LayerCodeData*) file_layer->layer_data;
    size_t zero = 0;
    arrsetlen(lcd->filename, zero);
    nob_da_foreach(char, c, &filename){
        arrput(lcd->filename, *c);
    }

    char* line = NULL;
    nob_da_foreach(char, c, &sb){
        if(*c == '\t'){
            arrput(line, ' ');
            arrput(line, ' ');
            arrput(line, ' ');
            arrput(line, ' ');
        }else if(*c == '\n'){
            arrput(line, '\n');
            arrput(line, '\0');
            arrput(lcd->code_buffer, line);
            line = NULL;
        }else if(*c == '\r'){
            continue;
        }
        else{
            arrput(line, *c);
        }
    }
    if (line != NULL) {
        arrput(line, '\0');
        arrput(lcd->code_buffer, line);
    }
    lcd->saved = true;
    push_layer_to_top(ccode, file_layer);
    make_parser(ccode, filename.items);

    nob_sb_free(sb);
    nob_sb_free(filename);
}


void change_filename(CCode* ccode, const char* new_filename, size_t size){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    // No filename to change
    if(top_code_layer == NULL){
        return;
    }

    LayerCodeData* lcd = (LayerCodeData*)top_code_layer->layer_data;
    size_t zero = 0;
    arrsetlen(lcd->filename, zero);
    lcd->filename = NULL;
    for(size_t i = 0; i < size; i++){
        arrput(lcd->filename, new_filename[i]);
    }
    arrput(lcd->filename, '\0');
    make_parser(ccode, lcd->filename);
}


void write_code_layer_to_file(CCode* ccode){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    // No code layer to save
    if(top_code_layer == NULL){
        return;
    }
    LayerCodeData* lcd = (LayerCodeData*) top_code_layer->layer_data; 

    Nob_String_Builder sb = {0};
    // build code layer to continues string for writing
    for(int line_n = 0; line_n < arrlen(lcd->code_buffer); line_n++){
        char* line = lcd->code_buffer[line_n];
        nob_sb_append_cstr(&sb, line);
    }

    if(nob_write_entire_file(lcd->filename, sb.items, sb.count)){
        lcd->saved = true;
    }

    nob_sb_free(sb);
}


/*

    Console Commands

*/

char* layertype_to_str(LayerType lt){
    if(lt == LAYER_CODE){
        return "LAYER_CODE";
    }else if(lt == LAYER_CONSOLE){
        return "LAYER_CONSOLE";
    }else if(lt == LAYER_DIR_WALK){
        return "TREE";
    }
    return "(null)";
}


void print_layer(Layer* l){
    if(l->type == LAYER_CODE){
        printf("  Layer: %p type: %s filename: %s\n", l, layertype_to_str(l->type), ((LayerCodeData*)l->layer_data)->filename);
    }else{
        printf("  Layer: %p type: %s\n", l, layertype_to_str(l->type));
    }
}


void free_layer(Layer* layer){
    if(!layer) return;

    if(layer->type == LAYER_CODE){
        LayerCodeData* lcd = (LayerCodeData*) layer->layer_data;
        if(lcd){
            if(lcd->code_buffer){
                for(size_t i = 0; i < arrlenu(lcd->code_buffer); i++){
                    arrfree(lcd->code_buffer[i]);
                }
                arrfree(lcd->code_buffer);
            }
            if(lcd->cursor){
                free(lcd->cursor);
            }
            if(lcd->filename){
                arrfree(lcd->filename);
            }
            if(lcd->parser){
                ts_parser_delete(lcd->parser);
            }
            if(lcd->tree){
                ts_tree_delete(lcd->tree);
            }
            free(lcd);
        }
    } else if(layer->type == LAYER_CONSOLE){
        LayerConsoleData* lcd = (LayerConsoleData*) layer->layer_data;
        if(lcd){
            if(lcd->console_buffer){
                arrfree(lcd->console_buffer);
            }
            free(lcd);
        }
    }else if(layer->type == LAYER_DIR_WALK){
        LayerDirWalkData* ldwd = (LayerDirWalkData*) layer->layer_data;
        if(ldwd){
            if(ldwd->current_dir_path){
                arrfree(ldwd->current_dir_path);
            }
            if(ldwd->current_dir_files){
                for(size_t i = 0; i < arrlenu(ldwd->current_dir_files); i++){
                    arrfree(ldwd->current_dir_files[i]);
                }
                arrfree(ldwd->current_dir_files);
            }
            free(ldwd);
        }
    }
    free(layer);
}


void message_to_console(CCode* ccode, const char* message){
    if(CLOSE_CONSOLE){
        CLOSE_CONSOLE = false;
    }
    Layer* console = top_type_layer(ccode, LAYER_CONSOLE);
    LayerConsoleData* layer_console_data = (LayerConsoleData*) console->layer_data;
    arrsetlen(layer_console_data->console_buffer, 0);

    for(size_t i = 0; i < strlen(message); i++){
        arrput(layer_console_data->console_buffer, message[i]);
    }

    arrput(layer_console_data->console_buffer, '\0');
    layer_console_data->console_buffer_x = arrlen(layer_console_data->console_buffer)-1;
}


void close_code_layer(CCode* ccode, bool forced){
    Layer* to_close = top_type_layer(ccode, LAYER_CODE);
    if(!to_close){
        return;
    }
    LayerCodeData* lcd = to_close->layer_data;
    if(lcd->saved == false && forced == false){
        message_to_console(ccode, "Cannot close unsaved file save it or use ':c!' to force close");
        return;
    }else{
        free_layer(to_close);
        remove_layer(ccode, to_close);
    }
}


void find_jump(CCode* ccode, char *finding, int32_t size, int32_t nth_occurence) {
    if (!ccode || !finding || size <= 0 || nth_occurence <= 0) {
        return;
    }
    Layer* layer = top_type_layer(ccode, LAYER_CODE);
    if (!layer) return;
    LayerCodeData* lcd = (LayerCodeData*) layer->layer_data;
    if (!lcd || !lcd->cursor) return;

    int y, x;
    getmaxyx(stdscr, y, x);
    (void)x;

    char* substr = malloc(size+1);
    memcpy(substr, finding, size);
    substr[size] = '\0';

    if(lcd->finding_substr == NULL){
        FindingSubstr* fss = malloc(sizeof(FindingSubstr));

        fss->substr = substr;
        fss->at_nth_occurence = nth_occurence;

        lcd->finding_substr = fss;
    }

    int32_t first_x = 0;
    int32_t first_y = 0;

    int32_t found = 0;
    for(int32_t nline = 0; nline < arrlen(lcd->code_buffer); nline++){
        char* line = lcd->code_buffer[nline];
        char* pos = line;
        while ((pos = strstr(pos, substr)) != NULL) {
            found++;
            if(found == 1){
                first_x = (pos-line);
                first_y = nline;
            }
            if (found == nth_occurence) {
                lcd->cursor->x = (pos - line);
                lcd->cursor->y = nline;

                // not sure if i like xoff yet
                //lcd->cursor->xoff = lcd->cursor->x - x / 2;
                lcd->cursor->yoff = lcd->cursor->y - y / 2;

                // if (lcd->cursor->xoff < 0) lcd->cursor->xoff = 0;
                if (lcd->cursor->yoff < 0) lcd->cursor->yoff = 0;

                return;
            }
            pos++;
        }
    }
    if(found < nth_occurence){
        if(lcd->finding_substr != NULL){
            lcd->finding_substr->at_nth_occurence = 1;
        }
        lcd->cursor->x = first_x;
        lcd->cursor->y = first_y;
    }

    if(found == 0){
        char message[128];
        char* str = stringview_to_str(finding, size);
        snprintf(message, sizeof(message), "Could not find '%s'", str);
        message_to_console(ccode, message);

        free(str);

        free(lcd->finding_substr->substr);
        free(lcd->finding_substr);
        lcd->finding_substr = NULL;
    }
}


char* run_command_buffer(const char* cmd){
    FILE* fp = popen(cmd, "r");
    if (!fp) return NULL;

    char* buf = NULL;
    char tmp[1024];

    while (fgets(tmp, sizeof(tmp), fp)) {
        for (char* p = tmp; *p; p++) {
            if(*p == '\n'){
                continue;
            }
            arrput(buf, *p);
        }
    }

    pclose(fp);

    arrput(buf, '\0');
    return buf;
}


void change_tree_path(Layer* layer, char* new_path){
    if(layer->type != LAYER_DIR_WALK){
        return;
    }

    LayerDirWalkData* ldwd = layer->layer_data;
    if(ldwd->current_dir_path != NULL){
        arrfree(ldwd->current_dir_path);
    }
    if(ldwd->current_dir_files != NULL){
        for(size_t i = 0; i < arrlenu(ldwd->current_dir_files); i++){
            arrfree(ldwd->current_dir_files[i]);
        }
        arrfree(ldwd->current_dir_files);
    }

    
    ldwd->current_dir_path = new_path;
    ldwd->current_dir_files = NULL;
}


void console_execute_command(CCode* ccode, const char* buffer){
    TokenizationOutput to = {0};
    if(tokenize_console_command(buffer, &to)){
        fprintf(stderr, "Error while tokenizing console command\n");
        return;
    }
    if (arrlen(to.tokens) < 1 || to.tokens[0].type != TOKEN_COMMAND) {
        return;
    }

    switch (to.tokens[0].command_type) {
        case COMMAND_QUIT: {
            RUNNING = false;
            break;
        } 
    
        case COMMAND_OPEN: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                read_file_to_code_layer(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
            }
            break;
        } 
    
        case COMMAND_GOTO: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_INTEGER && to.tokens[1].integer >= 0) {
                Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
                LayerCodeData* lcd = top_code_layer->layer_data;
                if (!lcd->cursor) break;
    
                lcd->cursor->y = to.tokens[1].integer > arrlen(lcd->code_buffer) ? arrlen(lcd->code_buffer) : to.tokens[1].integer;
                if (arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER && to.tokens[2].integer >= 0) {
                    lcd->cursor->x = to.tokens[2].integer;
                }
            }
            break;
        } 
    
        case COMMAND_CHANGE_NAME: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
            }
            break;
        } 
    
        case COMMAND_WRITE: {
            write_code_layer_to_file(ccode);
            break;
        } 
    
        case COMMAND_SYS: {
            if (arrlen(to.tokens) >= 2) {
                char* command = NULL;
                for(size_t i = 1; i < arrlenu(to.tokens); i++){
                    if(to.tokens[i].type == TOKEN_STRING){
                        for(size_t j = 0; j < to.tokens[i].string.size; j++){
                            arrput(command, to.tokens[i].string.start[j]);
                        }
                    }else if(to.tokens[i].type == TOKEN_INTEGER){
                        char itoa_str[64];
                        itoa(to.tokens[i].integer, itoa_str, 10);
                        for(size_t j = 0; j < strlen(itoa_str); j++){
                            arrput(command, itoa_str[j]);
                        }
                    }
                    if(i != arrlenu(to.tokens)-1){
                        arrput(command, ' ');
                    }
                }
                arrput(command, '\0');
            
                char* output = run_command_buffer(command);

                if(output == NULL){
                    arrfree(command);
                    return;
                }
            
                if (arrlen(output) > 1) {
                    message_to_console(ccode, output);
                }else {
                    message_to_console(ccode, "Command didnt respond");
                }
                arrfree(output);
                arrfree(command);
            }
            break;
        }
    
        case COMMAND_SAVE: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
                write_code_layer_to_file(ccode);
            }
            break;
        }

        case COMMAND_FIND: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                int32_t nth_occurence = 1;
                if(arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER){
                    nth_occurence = to.tokens[2].integer;
                }
                Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
                if(top_code_layer){
                    LayerCodeData* lcd = top_code_layer->layer_data;
                    if(lcd->finding_substr != NULL){
                        free(lcd->finding_substr->substr);
                        free(lcd->finding_substr);
                        lcd->finding_substr = NULL;
                    }
                }
                find_jump(ccode, to.tokens[1].string.start, to.tokens[1].string.size, nth_occurence);
            }
            break;
        }
        
        case COMMAND_CLOSE: {
            Layer* second_layer = layer_at_index(ccode, 1); // first will always be console
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE){
                close_code_layer(ccode, false);
            }else if(second_layer->type == LAYER_DIR_WALK){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }
            break;
        }

        case COMMAND_FORCE_CLOSE: {
            Layer* second_layer = layer_at_index(ccode, 1); // first will always be console
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE){
                close_code_layer(ccode, true);
            }else if(second_layer->type == LAYER_DIR_WALK){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }
            break;
        }

        case COMMAND_TREE: {
            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                char* dir_arr = NULL;

                if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING){
                    // change to single function call
                    char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                    dir_arr = str_to_arr(as_str);
                    free(as_str);
                }

                if(dir_arr == NULL){
                    const char* cwd = nob_get_current_dir_temp();
                    if(cwd == NULL){
                        printf("Could not get cwd!\n");
                        break;
                    }
                    dir_arr = str_to_arr(cwd);
                    nob_temp_reset();
                }

                Layer* tree = new_layer_dir_walk(dir_arr);
                push_layer_to_top(ccode, tree);
            }else{
                LayerDirWalkData* ldwd = top_tree_layer->layer_data;

                char* dir_arr = NULL;

                if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING){
                    // change to single function call
                    char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                    dir_arr = str_to_arr(as_str);
                    free(as_str);
                }

                if(dir_arr == NULL && ldwd->current_dir_path == NULL){
                    const char* cwd = nob_get_current_dir_temp();
                    if(cwd == NULL){
                        printf("Could not get cwd!\n");
                        break;
                    }
                    dir_arr = str_to_arr(cwd);
                    nob_temp_reset();
                }

                if(dir_arr != NULL){
                    change_tree_path(top_tree_layer, dir_arr);
                }

                push_layer_to_top(ccode, top_tree_layer);
            }
            break;
        }

        case COMMAND_SET_TAB_SIZE: {
            if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_INTEGER){
                TAB_SIZE = to.tokens[1].integer;
            }

            break;
        }

        case COMMAND_TREE_CHANGE_DIR: {
            if(arrlen(to.tokens) <= 1){
                return;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot change directory outside tree");
                return;
            }

            LayerDirWalkData* ldwd = top_tree_layer->layer_data;

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                if(strncmp(as_str, "..", 2) == 0){
                    // TODO: this allows using ../.. (etc) but it is implemented kinda unintuitively
                    char* new_dir = nob_temp_sprintf("%s/%s", ldwd->current_dir_path, as_str);
                    char resolved_path[MAX_PATH];
                    if(resolve_path(new_dir, resolved_path) == NULL){
                        free(as_str);
                        nob_temp_reset();
                        return;
                    }
                    change_tree_path(top_tree_layer, str_to_arr(resolved_path));
                    nob_temp_reset();
                }else {
                    // change to absolute path
                    if(!nob_file_exists(as_str)){
                        message_to_console(ccode, "invalid path");
                        return;
                    }
                    change_tree_path(top_tree_layer, str_to_arr(as_str));
                }

                free(as_str);
            }

            break;
        } 

        default: {
            // Unknown command
            break;
        } 
    }

    printf("Tokenization output:\n");
    for(size_t i = 0; i < arrlenu(to.tokens); i++ ){
        if(to.tokens[i].type == TOKEN_STRING || to.tokens[i].type == TOKEN_COMMAND){
            printf("  %d (%.*s)\n", to.tokens[i].type, to.tokens[i].string.size, to.tokens[i].string.start);
        }else if(to.tokens[i].type == TOKEN_INTEGER){
            printf("  %d N=%d\n", to.tokens[i].type, to.tokens[i].integer);
        }
    }

    arrfree(to.tokens);
    free(to.string_storage);
}



/*

    Layer character handling

*/

#define TS_REPARSE(code_data)                                                        \
    do {                                                                             \
        if((code_data)->parser && (code_data)->tree){                                \
            char *src = flatten_buffer(code_data);                                   \
            TSTree *new_tree = ts_parser_parse_string(                               \
                (code_data)->parser, (code_data)->tree, src, strlen(src));           \
            free(src);                                                               \
            ts_tree_delete((code_data)->tree);                                       \
            (code_data)->tree = new_tree;                                            \
        }                                                                            \
    } while(0)


void layer_code_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL){
        return;
    }
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;

    bool inFindSubstrMode = code_data->finding_substr != NULL;

    if(code_data->code_buffer == NULL){
        char* line = NULL;
        arrput(line, '\0');
        arrput(code_data->code_buffer, line);
    }
    
    if(arrlen(code_data->code_buffer) <= code_data->cursor->y){
        int add_n = code_data->cursor->y - arrlen(code_data->code_buffer) + 1;
        for(int i = 0; i < add_n; i++){
            char* line = NULL;
            arrput(line, '\n');
            arrput(line, '\0');
            arrput(code_data->code_buffer, line);
        }
    }

    if(code_data->cursor->y >= arrlen(code_data->code_buffer)){
        return;
    }

    if(chr == CUSTOM_CTL_F){
        Layer* console_in_layers = top_type_layer(ccode, LAYER_CONSOLE);
        if(console_in_layers == NULL){
            Layer* new_console = new_layer_console();
            push_layer_to_top(ccode, new_console);
            LayerConsoleData* console_data = new_console->layer_data;
            arrsetlen(console_data->console_buffer, 0);
            arrput(console_data->console_buffer, ':');
            arrput(console_data->console_buffer, 'f');
            arrput(console_data->console_buffer, ' ');
            arrput(console_data->console_buffer, '\0');
            console_data->console_buffer_x = 3;
        }
    }

    if(chr == 9){
        for(size_t i = 0; i < TAB_SIZE; i++){
            layer_code_update(ccode, layer, ' ');
        }
    }

    int y, x;
    getmaxyx(stdscr, y, x);

    if(inFindSubstrMode){
        FindingSubstr* fss = code_data->finding_substr;
        bool change = false;
        if(chr == KEY_RIGHT){
            fss->at_nth_occurence++;
            change = true;
            chr = -1;
        }else if(chr == KEY_LEFT){
            if(fss->at_nth_occurence - 1 >= 1){
                fss->at_nth_occurence--;
                change = true;
            }
            chr = -1;
        }
        if(change){
            find_jump(ccode, fss->substr, strlen(fss->substr), fss->at_nth_occurence);
        }
        if(chr == CUSTOM_KEY_ENTER){
            free(fss->substr);
            free(fss);
            code_data->finding_substr = NULL;
            chr = -1;
        }
    }



    if((chr >= 0 && chr <= 255) && isprint(chr)){
        char* line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(line);
        
        if(code_data->cursor->x > line_len - 1){
            code_data->cursor->x = line_len - 1;
        }
        
        if(line_len > 0){
            (void) arrpop(line);
        }
        
        if(code_data->cursor->x >= 0 && code_data->cursor->x <= arrlen(line)){
            arrins(line, code_data->cursor->x, (char)chr);
        } else {
            arrput(line, (char)chr);
        }
        code_data->saved = false;
        arrput(line, '\0');
        code_data->code_buffer[code_data->cursor->y] = line;

        int row = code_data->cursor->y;
        int col = code_data->cursor->x;

        if(code_data->parser && code_data->tree){
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, col),
                .old_end_byte  = buffer_byte_offset(code_data, row, col),
                .new_end_byte  = buffer_byte_offset(code_data, row, col + 1),
                .start_point   = {row, col},
                .old_end_point = {row, col},
                .new_end_point = {row, col + 1},
            };
            ts_tree_edit(code_data->tree, &edit);
        }
        TS_REPARSE(code_data);

        code_data->cursor->x++;
    }


    else if(chr == CUSTOM_KEY_BACKSPACE){
        char* line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(line);

        bool do_parse = false;
    
        if(code_data->cursor->x > 0 && line_len > 1){
            int row = code_data->cursor->y;
            int col = code_data->cursor->x;

            if(code_data->parser && code_data->tree){
                TSInputEdit edit = {
                    .start_byte    = buffer_byte_offset(code_data, row, col - 1),
                    .old_end_byte  = buffer_byte_offset(code_data, row, col),
                    .new_end_byte  = buffer_byte_offset(code_data, row, col - 1),
                    .start_point   = {row, col - 1},
                    .old_end_point = {row, col},
                    .new_end_point = {row, col - 1},
                };
                ts_tree_edit(code_data->tree, &edit);
            }
            do_parse = true;

            (void)arrpop(line);
            arrdel(line, code_data->cursor->x - 1);
            arrput(line, '\0');
            code_data->cursor->x--;
            code_data->code_buffer[code_data->cursor->y] = line;
        }
        else if(code_data->cursor->x == 0 && code_data->cursor->y > 0){
            char* prev_line = code_data->code_buffer[code_data->cursor->y - 1];
            int prev_line_len = arrlen(prev_line);
            int cur_row = code_data->cursor->y;

            int del_row = cur_row - 1;
            int del_col = (prev_line_len >= 2) ? prev_line_len - 2 : 0;

            if(code_data->parser && code_data->tree){
                TSInputEdit edit = {
                    .start_byte    = buffer_byte_offset(code_data, del_row, del_col),
                    .old_end_byte  = buffer_byte_offset(code_data, cur_row, 0),
                    .new_end_byte  = buffer_byte_offset(code_data, del_row, del_col),
                    .start_point   = {del_row, del_col},
                    .old_end_point = {cur_row, 0},
                    .new_end_point = {del_row, del_col},
                };
                ts_tree_edit(code_data->tree, &edit);
            }
            do_parse = true;

            int prev_len = arrlen(prev_line);
            code_data->cursor->x = (prev_len >= 2) ? prev_len - 2 : 0;

            if(prev_len >= 2){
                (void) arrpop(prev_line); // '\0'
                (void) arrpop(prev_line); // '\n'
            }

            char* current_line = code_data->code_buffer[code_data->cursor->y];
            int current_line_len = arrlen(current_line);
            for(int i = 0; i < current_line_len - 2; i++){
                arrput(prev_line, current_line[i]);
            }
            arrput(prev_line, '\n');
            arrput(prev_line, '\0');

            code_data->code_buffer[code_data->cursor->y - 1] = prev_line;
            arrfree(current_line);
            arrdel(code_data->code_buffer, code_data->cursor->y);
            code_data->cursor->y--;
        }

        if(do_parse){
            TS_REPARSE(code_data);
        }

        code_data->saved = false;
    }


    else if(chr == CUSTOM_KEY_ENTER){
        char* current_line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(current_line);
        
        if(code_data->cursor->x < 0) code_data->cursor->x = 0;
        if(code_data->cursor->x > line_len) code_data->cursor->x = line_len;

        int row = code_data->cursor->y;
        int col = code_data->cursor->x;

        if(code_data->parser && code_data->tree){
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, col),
                .old_end_byte  = buffer_byte_offset(code_data, row, col),
                .new_end_byte  = buffer_byte_offset(code_data, row, col) + 1,
                .start_point   = {row, col},
                .old_end_point = {row, col},
                .new_end_point = {row + 1, 0},
            };
            ts_tree_edit(code_data->tree, &edit);
        }

        char* new_line = NULL;
        if(line_len > 0 && code_data->cursor->x < line_len - 2){
            for(int i = code_data->cursor->x; i < line_len - 2; i++){
                arrput(new_line, current_line[i]);
            }
        }
        arrput(new_line, '\n');
        arrput(new_line, '\0');

        if(line_len > 0){
            (void) arrpop(current_line);
            while(arrlen(current_line) > code_data->cursor->x){
                (void) arrpop(current_line);
            }
        }
        arrput(current_line, '\n');
        arrput(current_line, '\0');

        code_data->code_buffer[code_data->cursor->y] = current_line;
        arrins(code_data->code_buffer, code_data->cursor->y + 1, new_line);

        code_data->cursor->y++;
        code_data->cursor->x = 0;
        code_data->saved = false;

        TS_REPARSE(code_data);
    }


    else if(!inFindSubstrMode && chr >= KEY_DOWN && chr <= KEY_RIGHT){
        switch(chr){
            case KEY_DOWN: {
                if(arrlen(code_data->code_buffer) <= code_data->cursor->y+1){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y+1];
                int length = arrlen(new_line)-2;
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length >= 0 ? length : 0;
                }
                code_data->cursor->y++;
                break;
            }
            case KEY_UP: {
                if(code_data->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y-1];
                int length = arrlen(new_line)-2;
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length >= 0 ? length : 0;
                }
                code_data->cursor->y--;
                break;
            }
            case KEY_LEFT: {
                if(code_data->cursor->x != 0){
                    code_data->cursor->x--;
                    break;
                }
                if(code_data->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y-1];
                int length = arrlen(new_line)-2;
                code_data->cursor->x = length >= 0 ? length : 0;
                code_data->cursor->y--;
                break;
            }
            case KEY_RIGHT: {
                char* current_line = code_data->code_buffer[code_data->cursor->y];
                if(code_data->cursor->x < arrlen(current_line)-2){
                    code_data->cursor->x++;
                    break;
                }
                if(arrlen(code_data->code_buffer) <= code_data->cursor->y+1){
                    break;
                }
                code_data->cursor->y++;
                code_data->cursor->x = 0;
                break;
            }
        }
    }
    else if(!inFindSubstrMode && chr == CTL_PGUP){
        code_data->cursor->y -= (y-2)*2;
        if(code_data->cursor->y < 0){
            code_data->cursor->y = 0;
        }
    }
    else if(!inFindSubstrMode && chr == CTL_PGDN){
        code_data->cursor->y += (y-2)*2;
    }
    else if (!inFindSubstrMode && chr == CTL_UP) {
        if (code_data->cursor->yoff > 0) {
            code_data->cursor->yoff--;
        }
    }
    else if (!inFindSubstrMode && chr == CTL_DOWN) {
        int screen_rows = y-1;
        int buffer_size = arrlen(code_data->code_buffer);
        int max_scroll = buffer_size - screen_rows;
        if (max_scroll < 0) max_scroll = 0;
        if (code_data->cursor->yoff < max_scroll) {
            code_data->cursor->yoff++;
        }
    }
    else if (!inFindSubstrMode && chr == CTL_RIGHT){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];
        int len = arrlen(line)-2;

        if(x >= len){
            if(code_data->cursor->y+1 < arrlen(code_data->code_buffer)){
                code_data->cursor->x = 0;
                code_data->cursor->y++;
            }
            goto carried_to_next_line;
        }

        while(x < len && isspace(line[x]))
            x++;
        while(x < len && ispunct(line[x]) && !isspace(line[x]))
            x++;
        while(x < len && isalnum(line[x]))
            x++;

        code_data->cursor->x = x;

        LABEL(carried_to_next_line)
    }
    else if(!inFindSubstrMode && chr == CTL_LEFT){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];

        if(x == 0){
            if(code_data->cursor->y > 0){
                code_data->cursor->y--;
                int prev_line_size = arrlen(code_data->code_buffer[code_data->cursor->y]);
                code_data->cursor->x = prev_line_size-2 > 0 ? prev_line_size-2 : 0;
            }
            goto carried_to_previous_line;
        }

        while(x > 0 && isspace(line[x-1]))
            x--;
        while(x > 0 && ispunct(line[x-1]) && !isspace(line[x-1]))
            x--;
        while(x > 0 && isalnum(line[x-1]))
            x--;

        code_data->cursor->x = x;

        LABEL(carried_to_previous_line)
    }
    else if(!inFindSubstrMode && chr == CUSTOM_CTL_BACKSPACE){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];

        if(x == 0){
            if (code_data->cursor->y > 0) {
                int prev_len = arrlen(code_data->code_buffer[code_data->cursor->y - 1]);
                int curr_len = arrlen(code_data->code_buffer[code_data->cursor->y]);

                if(code_data->parser && code_data->tree){
                    int del_row = code_data->cursor->y - 1;
                    int del_col = prev_len >= 2 ? prev_len - 2 : 0;
                    TSInputEdit edit = {
                        .start_byte    = buffer_byte_offset(code_data, del_row, del_col),
                        .old_end_byte  = buffer_byte_offset(code_data, code_data->cursor->y, 0),
                        .new_end_byte  = buffer_byte_offset(code_data, del_row, del_col),
                        .start_point   = {del_row, del_col},
                        .old_end_point = {code_data->cursor->y, 0},
                        .new_end_point = {del_row, del_col},
                    };
                    ts_tree_edit(code_data->tree, &edit);
                }

                arrsetlen(code_data->code_buffer[code_data->cursor->y - 1], prev_len-2);
                for(size_t i = 0; i < curr_len; i++){
                    arrput(code_data->code_buffer[code_data->cursor->y - 1], code_data->code_buffer[code_data->cursor->y][i]);
                }
                arrdel(code_data->code_buffer, code_data->cursor->y);
                code_data->cursor->y--;
                code_data->cursor->x = prev_len-2;

                TS_REPARSE(code_data);
            }
            goto carried_to_previous_line;
        }

        int new_x = x;
        while (new_x > 0 && isspace(line[new_x - 1]))
            new_x--;
        while (new_x > 0 && ispunct(line[new_x - 1]) && !isspace(line[new_x - 1]))
            new_x--;
        while (new_x > 0 && isalnum(line[new_x - 1]))
            new_x--;

        if(code_data->parser && code_data->tree){
            int row = code_data->cursor->y;
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, new_x),
                .old_end_byte  = buffer_byte_offset(code_data, row, x),
                .new_end_byte  = buffer_byte_offset(code_data, row, new_x),
                .start_point   = {row, new_x},
                .old_end_point = {row, x},
                .new_end_point = {row, new_x},
            };
            ts_tree_edit(code_data->tree, &edit);
        }

        arrsetlen(code_data->code_buffer[code_data->cursor->y], new_x);

        if(new_x == 0 || code_data->code_buffer[code_data->cursor->y][new_x - 1] != '\n'){
            arrput(code_data->code_buffer[code_data->cursor->y], '\n');
        }
        arrput(code_data->code_buffer[code_data->cursor->y], '\0');

        code_data->cursor->x = new_x;

        TS_REPARSE(code_data);
    }

    /* For debug CTRL + G */
    else if (!inFindSubstrMode && chr == 7){
        for(size_t i = 0; i < arrlenu(code_data->code_buffer[code_data->cursor->y]); i++){
            printf("%d ", code_data->code_buffer[code_data->cursor->y][i]);
        }
        printf("\n");
    }

    int content_height = y - 1;
    int content_width = x;
    
    int screen_y = code_data->cursor->y - code_data->cursor->yoff;
    if(screen_y < 0){
        code_data->cursor->yoff = code_data->cursor->y;
    } else if(screen_y >= content_height){
        code_data->cursor->yoff = code_data->cursor->y - content_height + 1;
    }
    
    int screen_x = code_data->cursor->x - code_data->cursor->xoff;
    if(screen_x < 0){
        code_data->cursor->xoff = code_data->cursor->x;
    } else if(screen_x >= content_width){
        code_data->cursor->xoff = code_data->cursor->x - content_width + 1;
    }
}

void layer_code_render(CCode* ccode, Layer* layer) {
    if (!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL) {
        return;
    }
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;
    // clock_t begin = clock();

    // Renders syntax highlighting or plain text if no parser is present
    apply_tree_sitter_syntax_highlighting(code_data, code_data->lang);
    
    //clock_t end = clock();
    //double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Render time: %f\n", time_spent);
}


void layer_console_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
    (void)y;
    getmaxyx(stdscr, y, x);
    if((chr >= 0 && chr <= 255) && isprint(chr)){
        int line_len = arrlen(console_data->console_buffer);
        
        if(console_data->console_buffer_x > line_len - 1){
            console_data->console_buffer_x = line_len - 1;
        }
        
        if(line_len > 0){
            (void) arrpop(console_data->console_buffer);
        }
        
        if(console_data->console_buffer_x >= 0 && console_data->console_buffer_x <= arrlen(console_data->console_buffer)){
            arrins(console_data->console_buffer, console_data->console_buffer_x, (char)chr);
        } else {
            arrput(console_data->console_buffer, (char)chr);
        }
        arrput(console_data->console_buffer, '\0');
        
        if(console_data->console_buffer_x + 1 < x){
            console_data->console_buffer_x++;
        }
    } else if(chr == CUSTOM_KEY_ENTER){
        CLOSE_CONSOLE = true;
        console_execute_command(ccode, console_data->console_buffer);
    } else if(chr == CUSTOM_KEY_BACKSPACE){
        if(console_data->console_buffer_x > 0 && arrlen(console_data->console_buffer) > 1){
            (void) arrpop(console_data->console_buffer);
            arrdel(console_data->console_buffer, console_data->console_buffer_x - 1);
            arrput(console_data->console_buffer, '\0');
            
            console_data->console_buffer_x--;
        }
    } else if(chr == KEY_LEFT){
        if(console_data->console_buffer_x != 0){
            console_data->console_buffer_x--;
        }
    } else if(chr == KEY_RIGHT){
        if(console_data->console_buffer_x < arrlen(console_data->console_buffer)-1){
            console_data->console_buffer_x++;
        }
    }
}


void layer_console_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
    getmaxyx(stdscr, y, x);
    
    //clear();
    char top_line[x+1];
    size_t size = snprintf(top_line, x + 1, "%s", console_data->console_buffer);
    if ((int)size > x) size = x;
    for (int i = size; i < x; i++) top_line[i] = ' ';
    top_line[x] = '\0';
    
    attron(A_REVERSE);
    mvprintw(y-1, 0, "%s", top_line);
    attroff(A_REVERSE);
    // move(y-1, console_data->console_buffer_x);
    // refresh();
}


int load_dir(LayerDirWalkData* ldwd){
    if(ldwd->current_dir_path == NULL){
        return -1;
    }
    Nob_File_Paths paths = {0};
    ldwd->selected = 0;
    ldwd->offset = 0;
    bool status = nob_read_entire_dir(ldwd->current_dir_path, &paths);
    if(!status){
        return -1;
    }
    for(size_t i = 0; i < paths.count; i++){
        // Noticed that on some implementation .. and . arent on top of the directory
        // so were skipping them and adding .. later to top
        if(strncmp(paths.items[i], ".", 1) == 0 || strncmp(paths.items[i], "..", 2) == 0){
            continue;
        }
        char* arr = str_to_arr(paths.items[i]); // convert from c string to arr of chars ending with null
        arrput(ldwd->current_dir_files, arr);
    }
    char* go_back_dir = NULL;
    arrput(go_back_dir, '.');
    arrput(go_back_dir, '.');
    arrput(go_back_dir, '\0');

    arrins(ldwd->current_dir_files, 0, go_back_dir);

    free(paths.items);
    return 0;
}


bool layer_dir_walk_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_DIR_WALK || layer->layer_data == NULL){
        return false;
    }

    LayerDirWalkData* ldwd = (LayerDirWalkData*) layer->layer_data;

    if(ldwd->current_dir_files == NULL){
        if(load_dir(ldwd) == -1){
            printf("There was error reading the directory %s", ldwd->current_dir_path);
            return false;
        }
    }

    int screen_y, screen_x;
    getmaxyx(stdscr, screen_y, screen_x);

    (void)screen_x;

    int list_height = screen_y - 3; // because drawing starts at y=2

    if(chr == KEY_DOWN){
        if((ldwd->selected + 1) < arrlen(ldwd->current_dir_files)){
            ldwd->selected++;
        }
    }

    if(chr == KEY_UP){
        if(ldwd->selected > 0){
            ldwd->selected--;
        }
    }

    // adjust offset
    if(ldwd->selected < ldwd->offset){
        ldwd->offset = ldwd->selected;
    }

    if(ldwd->selected >= ldwd->offset + list_height){
        ldwd->offset = ldwd->selected - list_height + 1;
    }


    if(chr == CUSTOM_KEY_ENTER){
        char* dir = nob_temp_sprintf("%s/%s",
            ldwd->current_dir_path,
            ldwd->current_dir_files[ldwd->selected]);
    
        char resolved_path[MAX_PATH] = {0};
    
        if(resolve_path(dir, resolved_path) == NULL){
            return false;
        }
    
        switch(nob_get_file_type(resolved_path)){
            case NOB_FILE_DIRECTORY: {
                char* arr = str_to_arr(resolved_path);
    
                change_tree_path(layer, arr);
    
                load_dir(ldwd);
    
                nob_temp_reset();
                break;
            }
    
            default: {
                char* arr = str_to_arr(resolved_path);
                read_file_to_code_layer(ccode, arr, arrlen(arr)-1);
                arrfree(arr);
                remove_layer(ccode, layer);
                free_layer(layer);
                return true;
            }
        }
    }
    return false;
}


Nob_File_Type dir_walk_get_file_type(char* current_dir, char* filename){
    Nob_File_Type file_type = nob_get_file_type(nob_temp_sprintf("%s/%s", current_dir, filename));
    nob_temp_reset();
    return file_type;
}

void layer_dir_walk_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_DIR_WALK || layer->layer_data == NULL){
        return;
    }

    LayerDirWalkData* ldwd = (LayerDirWalkData*) layer->layer_data;

    //TODO: render tree
    mvprintw(1, 0, "Current dir: %s", ldwd->current_dir_path);
    int y, x;
    getmaxyx(stdscr, y, x);
    (void)x;

    int draw_y = 3;
    for(size_t i = ldwd->offset; i < arrlenu(ldwd->current_dir_files); i++){
        if(draw_y >= y) break;
        Nob_File_Type ft = dir_walk_get_file_type(ldwd->current_dir_path, ldwd->current_dir_files[i]);
        if(ft == NOB_FILE_DIRECTORY){
            attron(COLOR_PAIR(COLOR_DIR));
        }else{
            attron(COLOR_PAIR(COLOR_FILE));
        }
        if(i == ldwd->selected){
            attron(A_REVERSE);
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ldwd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ldwd->current_dir_files[i]);
            }
            attroff(A_REVERSE);
        } else {
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ldwd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ldwd->current_dir_files[i]);
            }
        }
        if(ft == NOB_FILE_DIRECTORY){
            attroff(COLOR_PAIR(COLOR_DIR));
        }else{
            attroff(COLOR_PAIR(COLOR_FILE));
        }
    
        draw_y++;
    }
}

void layer_dir_walk_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    bool redirected = layer_dir_walk_update(ccode, layer, chr);
    if(redirected){
        return;
    }
    if(should_draw){
        layer_dir_walk_render(ccode, layer);
    }
}

void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    layer_console_update(ccode, layer, chr);
    if(should_draw){
        layer_console_render(ccode, layer);
    }
}

void layer_code_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    layer_code_update(ccode, layer, chr);
    if(should_draw){
        layer_code_render(ccode, layer);
    }
}

void draw_ui(CCode* ccode) {
    Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    Layer* layer_at_top = top_layer(ccode);

    if(layer_at_top == NULL) {
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);
    if(code_layers != NULL){
        char top_line[x + 1];
        size_t size = 0;

        int cells_per_layer = x / arrlen(code_layers);

        for (size_t i = 0; i < arrlenu(code_layers); i++) {
            LayerCodeData* lcd = (LayerCodeData*) code_layers[i]->layer_data;
            const char* file_name = lcd->filename;
            if(arrlen(lcd->filename) > 16){
                file_name = nob_path_name(lcd->filename);
            }
            int written = snprintf(top_line + size,
                                   ((int)size < x) ? (x - size + 1) : 0,
                                   "%s%s",
                                   file_name,
                                   lcd->saved ? "" : "*");

            if (written < 0) written = 0;

            if ((size_t)written > (size_t)(x - size)) {
                written = x - size;
            }
            size += written;

            while ((int)(size % cells_per_layer) != 0 && size < (size_t)x) {
                top_line[size++] = ' ';
            }
        }

        if (size > (size_t)x) size = x;
        top_line[size] = '\0';

        attron(A_REVERSE);
        mvprintw(0, 0, "%s", top_line);
        attroff(A_REVERSE);
    }
 

    char mode = 'I';

    if(top_code_layer != NULL && ((LayerCodeData*) top_code_layer->layer_data)->finding_substr != NULL){
        mode = 'J';
    }

    if(layer_at_top->type == LAYER_CONSOLE){
        mode = 'C';
    }else if(layer_at_top->type == LAYER_DIR_WALK){
        mode = 'T';
    }

    if(layer_at_top->type == LAYER_CONSOLE){
        LayerConsoleData* lcd = layer_at_top->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, 0, lcd->console_buffer_x);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, 0, lcd->console_buffer_x);
    }else if(layer_at_top->type == LAYER_CODE){
        LayerCodeData* lcd = layer_at_top->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, lcd->cursor->y, lcd->cursor->x);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, lcd->cursor->y, lcd->cursor->x);
    }else if(layer_at_top->type == LAYER_DIR_WALK){
        LayerDirWalkData* ldwd = layer_at_top->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, ldwd->selected + ldwd->offset, 0);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, ldwd->selected + ldwd->offset, 0);
    }else{
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, 0, 0);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, 0, 0);
    }


    if(layer_at_top->type == LAYER_CODE){
        LayerCodeData* lcd = (LayerCodeData*) layer_at_top->layer_data;
        int cursor_screen_y = lcd->cursor->y - lcd->cursor->yoff + 1;
        int cursor_screen_x = lcd->cursor->x - lcd->cursor->xoff;
        move(cursor_screen_y, cursor_screen_x);
    }else if(layer_at_top->type == LAYER_CONSOLE){
        LayerConsoleData* lcd = (LayerConsoleData*) layer_at_top->layer_data;
        move(y-1, lcd->console_buffer_x);
    }else if(layer_at_top->type == LAYER_DIR_WALK){
        //LayerDirWalkData* ldwd = (LayerDirWalkData*) layer_at_top->layer_data;
        //move(ldwd->selected+ldwd->offset+2, 0);
    }
    else {
        assert(false && "unknown layer");
    }

    arrfree(code_layers);
}

#endif