#ifndef _H_LAYERS
#define _H_LAYERS

#include "defines.h"

#include "tokenizer.h"
#include "syntax_highlighting.h"

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

    lcd->filename = NULL;
    for(size_t i = 0; i < default_filename_length; i++){
        arrput(lcd->filename, default_filename[i]);
    }
    arrput(lcd->filename, '\0');
    lcd->code_buffer = NULL;
    lcd->saved = false;
    code->layer_data = lcd;

    return code;
}


Layer* new_layer_console(){
    Layer* console = malloc(sizeof(Layer));
    if(!console){
        return NULL;
    }
    console->type = LAYER_CONSOLE;
    console->consume_input = true;

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

/*

    Layers managment helpers

*/


void push_layer_to_top(CCode* ccode, Layer* to_top){
    if (arrlen(ccode->layers) > 0 && ccode->layers[0] == to_top) {
        return;
    }

    // Remove the layer if it is present in layers
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i] == to_top){
            arrdel(ccode->layers, i);
            break;
        }
    }

    // insert to top
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
// alias
#define at_index_layer contains_layer


/*

    File manipulation

*/


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
            arrput(line, '\0');
            arrput(lcd->code_buffer, line);
            line = NULL;
        }else{
            arrput(line, *c);
        }
    }
    if (line != NULL) {
        arrput(line, '\0');
        arrput(lcd->code_buffer, line);
    }
    lcd->saved = true;
    push_layer_to_top(ccode, file_layer);
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
        nob_da_append(&sb, '\n');
    }

    if(nob_write_entire_file(lcd->filename, sb.items, sb.count)){
        lcd->saved = true;
    }

    nob_sb_free(sb);
}


/*

    Console Commands

*/



void find_jump(CCode* ccode, char *finding, int32_t size, int32_t nth_occurence) {
    if (!ccode || !finding || size <= 0 || nth_occurence <= 0) {
        return;
    }
    Layer* layer = top_type_layer(ccode, LAYER_CODE);
    if (!layer) return;
    LayerCodeData* lcd = (LayerCodeData*) layer->layer_data;
    if (!lcd || !lcd->cursor) return;

    char* substr = malloc(size+1);
    memcpy(substr, finding, size);
    substr[size] = '\0';

    int32_t found = 0;
    for(int32_t nline = 0; nline < arrlen(lcd->code_buffer); nline++){
        char* line = lcd->code_buffer[nline];
        char* pos = line;
        while ((pos = strstr(pos, substr)) != NULL) {
            found++;
            if (found == nth_occurence) {
                lcd->cursor->x = (pos - line);
                lcd->cursor->y = nline;
                free(substr);
                return;
            }
            pos++;
        }
    }
    free(substr);
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
                Cursor* cursor = top_code_layer_cursor(ccode);
                if (!cursor) break;
    
                cursor->y = to.tokens[1].integer;
                if (arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER && to.tokens[2].integer >= 0) {
                    cursor->x = to.tokens[2].integer;
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
            assert(0 && "Not implemented");
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
                find_jump(ccode, to.tokens[1].string.start, to.tokens[1].string.size, nth_occurence);
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

void layer_code_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL){
        return;
    }
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;

    if(code_data->code_buffer == NULL){
        char* line = NULL;
        arrput(line, '\0');
        arrput(code_data->code_buffer, line);
    }
    
    // Ensure we have enough lines in the buffer
    if(arrlen(code_data->code_buffer) <= code_data->cursor->y){
        int add_n = code_data->cursor->y - arrlen(code_data->code_buffer) + 1;
        for(int i = 0; i < add_n; i++){
            char* line = NULL;
            arrput(line, '\0');
            arrput(code_data->code_buffer, line);
        }
    }

    if(code_data->cursor->y >= arrlen(code_data->code_buffer)){
        return;
    }

    // add character to buffer that we have our curses on
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
        
        code_data->cursor->x++;
        code_data->code_buffer[code_data->cursor->y] = line;
    }
    // delete char
    else if(chr == CUSTOM_KEY_BACKSPACE){
        char* line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(line);
        
        if(code_data->cursor->x > 0 && line_len > 1){
            (void) arrpop(line);
            arrdel(line, code_data->cursor->x - 1);
            arrput(line, '\0');
            
            code_data->cursor->x--;
            
            code_data->code_buffer[code_data->cursor->y] = line;
        } else if(code_data->cursor->x == 0 && code_data->cursor->y > 0){
            char* current_line = code_data->code_buffer[code_data->cursor->y];
            char* prev_line = code_data->code_buffer[code_data->cursor->y - 1];
            
            int prev_line_len = arrlen(prev_line);
            code_data->cursor->x = (prev_line_len > 0) ? prev_line_len - 1 : 0;
            
            if(prev_line_len > 0){
                (void) arrpop(prev_line);
            }
            
            int current_line_len = arrlen(current_line);
            for(int i = 0; i < current_line_len - 1; i++){
                arrput(prev_line, current_line[i]);
            }
            arrput(prev_line, '\0');
            
            code_data->code_buffer[code_data->cursor->y - 1] = prev_line;
            
            arrfree(current_line);
            arrdel(code_data->code_buffer, code_data->cursor->y);
            
            code_data->cursor->y--;
        }
        code_data->saved = false;
    }
    // new line
    else if(chr == CUSTOM_KEY_ENTER){
        char* current_line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(current_line);
        
        if(code_data->cursor->x < 0){
            code_data->cursor->x = 0;
        }
        if(code_data->cursor->x > line_len - 1){
            code_data->cursor->x = (line_len > 0) ? line_len - 1 : 0;
        }
        
        char* new_line = NULL;
        
        if(line_len > 0 && code_data->cursor->x < line_len - 1){
            for(int i = code_data->cursor->x; i < line_len - 1; i++){
                arrput(new_line, current_line[i]);
            }
        }
        arrput(new_line, '\0');
        
        if(line_len > 0){
            (void) arrpop(current_line);
            while(arrlen(current_line) > code_data->cursor->x){
                (void) arrpop(current_line);
            }
            arrput(current_line, '\0');
        }
        
        code_data->code_buffer[code_data->cursor->y] = current_line;
        
        arrins(code_data->code_buffer, code_data->cursor->y + 1, new_line);

        code_data->cursor->y++;
        code_data->cursor->x = 0;
        code_data->saved = false;
    }
    // moving cursor with keys
    else if(chr >= KEY_DOWN && chr <= KEY_RIGHT){
        switch(chr){
            case KEY_DOWN: {
                if(arrlen(code_data->code_buffer) <= code_data->cursor->y+1){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y+1];
                int length = (int) strlen(new_line);
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length;
                }
                code_data->cursor->y++;
                break;
            }
            case KEY_UP: {
                if(code_data->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y-1];
                int length = (int)strlen(new_line);
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length;
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
                int length = (int)strlen(new_line);
                code_data->cursor->x = length;
                code_data->cursor->y--;
                break;
            }
            case KEY_RIGHT: {
                char* current_line = code_data->code_buffer[code_data->cursor->y];
                if(code_data->cursor->x < arrlen(current_line)-1){
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
    
    int y, x;
    getmaxyx(stdscr, y, x);
    
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

    int y, x;
    getmaxyx(stdscr, y, x);

    int content_height = y - 1;
    int content_width = x;

    //clear();

    int buffer_size = arrlen(code_data->code_buffer);

    // allocate visible buffer
    char **visible_buffer = malloc(sizeof(char*) * content_height);

    for (int screen_line = 0; screen_line < content_height; screen_line++) {
        int buffer_line = screen_line + code_data->cursor->yoff;

        if (buffer_line >= buffer_size) {
            visible_buffer[screen_line] = strdup(""); // empty line
            continue;
        }

        char* line = code_data->code_buffer[buffer_line];
        if (!line) {
            visible_buffer[screen_line] = strdup("");
            continue;
        }

        int line_len = strlen(line);
        if (code_data->cursor->xoff >= line_len) {
            visible_buffer[screen_line] = strdup(""); // scrolled past end of line
        } else {
            int chars_to_display = line_len - code_data->cursor->xoff;
            if (chars_to_display > content_width) {
                chars_to_display = content_width;
            }

            visible_buffer[screen_line] = malloc(chars_to_display + 1);
            strncpy(visible_buffer[screen_line],
                    line + code_data->cursor->xoff,
                    chars_to_display);
            visible_buffer[screen_line][chars_to_display] = '\0';
        }

        // now print from visible buffer instead of slicing inline
        //mvprintw(screen_line + 1, 0, "%s", visible_buffer[screen_line]);
    }
    apply_c_syntax_highlighting(visible_buffer, content_height);

    // cleanup
    for (int i = 0; i < content_height; i++) {
        free(visible_buffer[i]);
    }
    free(visible_buffer);
}


void layer_console_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
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
        console_execute_command(ccode, console_data->console_buffer);
        CLOSE_CONSOLE = true;
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


void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr){
    layer_console_update(ccode, layer, chr);
    layer_console_render(ccode, layer);
}

void layer_code_handle_keypress(CCode* ccode, Layer* layer, int chr){
    layer_code_update(ccode, layer, chr);
    layer_code_render(ccode, layer);
}


void draw_ui(CCode* ccode) {
    Layer** layers = all_type_layers(ccode, LAYER_CODE);
    Cursor* top_code_cursor = top_code_layer_cursor(ccode);
    Layer* layer_at_top = top_layer(ccode);

    if (!layers || arrlen(layers) == 0 || top_code_cursor == NULL || layer_at_top == NULL) {
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);

    char top_line[x + 1];
    size_t size = 0;

    int cells_per_layer = x / arrlen(layers);

    for (size_t i = 0; i < arrlenu(layers); i++) {
        LayerCodeData* lcd = (LayerCodeData*) layers[i]->layer_data;

        int written = snprintf(top_line + size,
                               ((int)size < x) ? (x - size + 1) : 0,
                               "%s%s",
                               lcd->filename,
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
    size_t bot_line_size = snprintf(NULL, 0, "%d:%d", top_code_cursor->y, top_code_cursor->x);
    mvprintw(y - 1, x - bot_line_size, "%d:%d", top_code_cursor->y, top_code_cursor->x);

    if(layer_at_top->type == LAYER_CODE){
        LayerCodeData* lcd = (LayerCodeData*) layer_at_top->layer_data;
        int cursor_screen_y = lcd->cursor->y - lcd->cursor->yoff + 1;
        int cursor_screen_x = lcd->cursor->x - lcd->cursor->xoff;
        move(cursor_screen_y, cursor_screen_x);
    }else if(layer_at_top->type == LAYER_CONSOLE){
        LayerConsoleData* lcd = (LayerConsoleData*) layer_at_top->layer_data;
        move(y-1, lcd->console_buffer_x);
    }
    else {
        assert(false && "unknown layer");
    }

    arrfree(layers);
}


char* layertype_to_str(LayerType lt){
    if(lt == LAYER_CODE){
        return "LAYER_CODE";
    }else if(lt == LAYER_CONSOLE){
        return "LAYER_CONSOLE";
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
            if(lcd->filename){
                arrfree(lcd->filename);
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
    }
    free(layer);
}


#endif