#ifndef _H_LAYERS
#define _H_LAYERS

#include "defines.h"
#include "tokenizer.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

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

Layer* top_type_layer(CCode* ccode, LayerType type){
    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == type) return ccode->layers[i];
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

#define default_filename_length 8
static char* default_filename = "untitled";

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


void layer_code_handle_keypress(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL){
        return;
    }
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;
    
    int y, x;
    getmaxyx(stdscr, y, x);
    
    int content_height = y - 1;
    int content_width = x;

    if(code_data->code_buffer == NULL){
        char* line = NULL;
        arrput(line, '\0');
        arrput(code_data->code_buffer, line);
    }
    
    // Ensure we have enough lines in the buffer
    if(arrlen(code_data->code_buffer) <= ccode->cursor->y){
        int add_n = ccode->cursor->y - arrlen(code_data->code_buffer) + 1;
        for(int i = 0; i < add_n; i++){
            char* line = NULL;
            arrput(line, '\0');
            arrput(code_data->code_buffer, line);
        }
    }

    if(ccode->cursor->y >= arrlen(code_data->code_buffer)){
        return;
    }

    // add character to buffer that we have our curses on
    if((chr >= 0 && chr <= 255) && isprint(chr)){
        char* line = code_data->code_buffer[ccode->cursor->y];
        int line_len = arrlen(line);
        
        if(ccode->cursor->x > line_len - 1){
            ccode->cursor->x = line_len - 1;
        }
        
        if(line_len > 0){
            (void) arrpop(line);
        }
        
        if(ccode->cursor->x >= 0 && ccode->cursor->x <= arrlen(line)){
            arrins(line, ccode->cursor->x, (char)chr);
        } else {
            arrput(line, (char)chr);
        }
        code_data->saved = false;
        arrput(line, '\0');
        
        ccode->cursor->x++;
        code_data->code_buffer[ccode->cursor->y] = line;
    }
    // delete char
    else if(chr == CUSTOM_KEY_BACKSPACE){
        char* line = code_data->code_buffer[ccode->cursor->y];
        int line_len = arrlen(line);
        
        if(ccode->cursor->x > 0 && line_len > 1){
            (void) arrpop(line);
            arrdel(line, ccode->cursor->x - 1);
            arrput(line, '\0');
            
            ccode->cursor->x--;
            
            code_data->code_buffer[ccode->cursor->y] = line;
        } else if(ccode->cursor->x == 0 && ccode->cursor->y > 0){
            char* current_line = code_data->code_buffer[ccode->cursor->y];
            char* prev_line = code_data->code_buffer[ccode->cursor->y - 1];
            
            int prev_line_len = arrlen(prev_line);
            ccode->cursor->x = (prev_line_len > 0) ? prev_line_len - 1 : 0;
            
            if(prev_line_len > 0){
                (void) arrpop(prev_line);
            }
            
            int current_line_len = arrlen(current_line);
            for(int i = 0; i < current_line_len - 1; i++){
                arrput(prev_line, current_line[i]);
            }
            arrput(prev_line, '\0');
            
            code_data->code_buffer[ccode->cursor->y - 1] = prev_line;
            
            arrfree(current_line);
            arrdel(code_data->code_buffer, ccode->cursor->y);
            
            ccode->cursor->y--;
        }
        code_data->saved = false;
    }
    // new line
    else if(chr == CUSTOM_KEY_ENTER){
        char* current_line = code_data->code_buffer[ccode->cursor->y];
        int line_len = arrlen(current_line);
        
        if(ccode->cursor->x < 0){
            ccode->cursor->x = 0;
        }
        if(ccode->cursor->x > line_len - 1){
            ccode->cursor->x = (line_len > 0) ? line_len - 1 : 0;
        }
        
        char* new_line = NULL;
        
        if(line_len > 0 && ccode->cursor->x < line_len - 1){
            for(int i = ccode->cursor->x; i < line_len - 1; i++){
                arrput(new_line, current_line[i]);
            }
        }
        arrput(new_line, '\0');
        
        if(line_len > 0){
            (void) arrpop(current_line);
            while(arrlen(current_line) > ccode->cursor->x){
                (void) arrpop(current_line);
            }
            arrput(current_line, '\0');
        }
        
        code_data->code_buffer[ccode->cursor->y] = current_line;
        
        arrins(code_data->code_buffer, ccode->cursor->y + 1, new_line);

        ccode->cursor->y++;
        ccode->cursor->x = 0;
        code_data->saved = false;
    }
    // moving cursor with keys
    else if(chr >= KEY_DOWN && chr <= KEY_RIGHT){
        switch(chr){
            case KEY_DOWN: {
                if(arrlen(code_data->code_buffer) <= ccode->cursor->y+1){
                    break;
                }
                char* new_line = code_data->code_buffer[ccode->cursor->y+1];
                int length = (int) strlen(new_line);
                if(length < ccode->cursor->x){
                    ccode->cursor->x = length;
                }
                ccode->cursor->y++;
                break;
            }
            case KEY_UP: {
                if(ccode->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[ccode->cursor->y-1];
                int length = (int)strlen(new_line);
                if(length < ccode->cursor->x){
                    ccode->cursor->x = length;
                }
                ccode->cursor->y--;
                break;
            }
            case KEY_LEFT: {
                if(ccode->cursor->x != 0){
                    ccode->cursor->x--;
                    break;
                }
                if(ccode->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[ccode->cursor->y-1];
                int length = (int)strlen(new_line);
                ccode->cursor->x = length;
                ccode->cursor->y--;
                break;
            }
            case KEY_RIGHT: {
                char* current_line = code_data->code_buffer[ccode->cursor->y];
                if(ccode->cursor->x < arrlen(current_line)-1){
                    ccode->cursor->x++;
                    break;
                }
                if(arrlen(code_data->code_buffer) <= ccode->cursor->y+1){
                    break;
                }
                ccode->cursor->y++;
                ccode->cursor->x = 0;
                break;
            }
        }
    }
    
    int screen_y = ccode->cursor->y - ccode->cursor->yoff;
    if(screen_y < 0){
        ccode->cursor->yoff = ccode->cursor->y;
    } else if(screen_y >= content_height){
        ccode->cursor->yoff = ccode->cursor->y - content_height + 1;
    }
    
    int screen_x = ccode->cursor->x - ccode->cursor->xoff;
    if(screen_x < 0){
        ccode->cursor->xoff = ccode->cursor->x;
    } else if(screen_x >= content_width){
        ccode->cursor->xoff = ccode->cursor->x - content_width + 1;
    }

    // clear();
    
    char top_line[x+1];
    size_t size = 0;
    assert(code_data->filename != NULL && "code_data->filename should not be NULL");
    char* file_name = code_data->filename;
    size = snprintf(top_line, x + 1, "%s%s", file_name, code_data->saved ? "" : "*");

    if ((int)size > x) size = x;
    for (int i = size; i < x; i++) {
        top_line[i] = ' ';
    }
    top_line[x] = '\0';

    attron(A_REVERSE);
    mvprintw(0, 0, "%s", top_line);
    attroff(A_REVERSE);

    // Draw content lines with offset
    int buffer_size = arrlen(code_data->code_buffer);
    for(int screen_line = 0; screen_line < content_height; screen_line++){
        int buffer_line = screen_line + ccode->cursor->yoff;
        
        if(buffer_line >= buffer_size){
            break;
        }
        
        char* line = code_data->code_buffer[buffer_line];
        if(line != NULL){
            int line_len = strlen(line);
            
            if(ccode->cursor->xoff < line_len){
                int chars_to_display = line_len - ccode->cursor->xoff;
                if(chars_to_display > content_width){
                    chars_to_display = content_width;
                }
                
                char display_line[chars_to_display + 1];
                strncpy(display_line, line + ccode->cursor->xoff, chars_to_display);
                display_line[chars_to_display] = '\0';
                
                mvprintw(screen_line + 1, 0, "%s", display_line);
            }
        }
    }
    
    size_t bot_size = snprintf(NULL, 0, "%d:%d", ccode->cursor->y, ccode->cursor->x);
    mvprintw(y-1, x-bot_size, "%d:%d", ccode->cursor->y, ccode->cursor->x);
    
    int cursor_screen_y = ccode->cursor->y - ccode->cursor->yoff + 1;
    int cursor_screen_x = ccode->cursor->x - ccode->cursor->xoff;
    
    if(cursor_screen_y >= 1 && cursor_screen_y < y && 
       cursor_screen_x >= 0 && cursor_screen_x < x){
        move(cursor_screen_y, cursor_screen_x);
    }
    
    // refresh();
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


void console_execute_command(CCode* ccode, const char* buffer){
	TokenizationOutput to = {0};
	if(tokenize_console_command(buffer, &to)){
		fprintf(stderr, "Error while tokenizing console command\n");
		return;
	}
	if(arrlen(to.tokens) == 0){
		printf("Empty tokens\n");
		return;
	}
    // TODO: Make some system that eliminates these long if statments

    // Quit
	if(
		arrlen(to.tokens) >= 1 && 
		to.tokens[0].type == TOKEN_COMMAND &&
		to.tokens[0].command_type == COMMAND_QUIT
	){
		RUNNING = false;
	}
    // Read and open file
	else if(
		arrlen(to.tokens) >= 2 &&
		to.tokens[0].type == TOKEN_COMMAND &&
		to.tokens[0].command_type == COMMAND_OPEN &&
        to.tokens[1].type == TOKEN_STRING
    ){
        read_file_to_code_layer(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
	}
    // Goto line 
    else if(
        arrlen(to.tokens) >= 2 &&
        to.tokens[0].type == TOKEN_COMMAND &&
        to.tokens[0].command_type == COMMAND_GOTO &&
        to.tokens[1].type == TOKEN_INTEGER &&
        to.tokens[1].integer >= 0
    ){
        ccode->cursor->y = to.tokens[1].integer;

        if(arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER && to.tokens[2].integer >= 0){
            printf("setting cursor X to %d\n", to.tokens[2].integer);
            ccode->cursor->x = to.tokens[2].integer;
        }
    }
    // Change name
    else if(
        arrlen(to.tokens) >= 2 &&
        to.tokens[0].type == TOKEN_COMMAND &&
        to.tokens[0].command_type == COMMAND_CHANGE_NAME &&
        to.tokens[1].type == TOKEN_STRING
    ){
        change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
    }
    else if(
        arrlen(to.tokens) >= 1 &&
        to.tokens[0].type == TOKEN_COMMAND &&
        to.tokens[0].command_type == COMMAND_WRITE 
    ){
        write_code_layer_to_file(ccode);
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

void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr){
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
    //clear();
    char top_line[x+1];
    size_t size = snprintf(top_line, x + 1, "%s", console_data->console_buffer);
    if ((int)size > x) size = x;
    for (int i = size; i < x; i++) top_line[i] = ' ';
    top_line[x] = '\0';
    

    attron(A_REVERSE);
    mvprintw(y-1, 0, "%s", top_line);
    attroff(A_REVERSE);
    move(y-1, console_data->console_buffer_x);
    // refresh();
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