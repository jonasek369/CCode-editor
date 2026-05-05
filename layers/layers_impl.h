#ifndef _H_LAYERS_IMPL
#define _H_LAYERS_IMPL


/*
    Layer management helpers
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

    for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i] == to_bot){
            arrdel(ccode->layers, i);
            break;
        }
    }

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
    Buffer helpers
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


/*
    Layer debug helpers
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


/*
    Layer cleanup
*/

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
            if(lcd->uri){
                arrfree(lcd->uri);
            }
            if(lcd->diagnostics){
                json_free(lcd->diagnostics);
            }
            if(lcd->ranges){
                for(size_t i = 0; i < arrlenu(lcd->ranges); i++){
                    if(lcd->ranges[i])
                        free(lcd->ranges[i]);
                }
                arrfree(lcd->ranges);
                lcd->ranges = NULL;
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
    }else if(layer->type == LAYER_THEME_SELECTOR){
        LayerThemeSelectorData* ltsd = (LayerThemeSelectorData*) layer->layer_data;
        if(ltsd){
            if(ltsd->current_dir_files){
                for(size_t i = 0; i < arrlenu(ltsd->current_dir_files); i++){
                    arrfree(ltsd->current_dir_files[i]);
                }
                arrfree(ltsd->current_dir_files);
            }
            free(ltsd);
        }
    }
    free(layer);
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

/*
    UI
*/

void draw_ui(CCode* ccode) {
    Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    Layer* layer_at_top = top_layer(ccode);

    if(layer_at_top == NULL) {
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);
    if (code_layers != NULL) {
        char top_line[x + 1];
        size_t size = 0;

        size_t num_layers = arrlenu(code_layers);
        int cells_per_layer = x / num_layers;

        for (int i = 0; i < num_layers; i++) {
            LayerCodeData* lcd = (LayerCodeData*) code_layers[i]->layer_data;
            const char* file_name = lcd->filename;
            if (strlen(lcd->filename) > 16){
                file_name = nob_path_name(lcd->filename);
            }

            char label[32];
            snprintf(label, sizeof(label), "[%d] %s%s", i+1, file_name, lcd->saved ? "" : "*");

            int label_len = strlen(label);
            if (label_len > cells_per_layer - 1) {
                label_len = cells_per_layer - 1;
            }

            for (int j = 0; j < label_len; j++) {
                top_line[size++] = label[j];
            }

            while (size % cells_per_layer != 0 && size < (size_t)x) {
                top_line[size++] = ' ';
            }
        }

        if (size > (size_t)x) size = x;
        top_line[size] = '\0';

        move(0, 0);
        for (int i = 0; i < size; i++) {
            if (i < cells_per_layer) {
                attron(A_REVERSE);
            } else {
                attroff(A_REVERSE);
            }
            addch(top_line[i]);
        }
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
    }else if(layer_at_top->type == LAYER_THEME_SELECTOR){
        LayerThemeSelectorData* ltsd = layer_at_top->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, ltsd->selected + ltsd->offset, 0);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, ltsd->selected + ltsd->offset, 0);
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
        // no cursor movement needed
    }else if(layer_at_top->type == LAYER_THEME_SELECTOR){
        // no cursor movement needed
    }else {
        assert(false && "unknown layer");
    }

    arrfree(code_layers);
}

#endif // _H_LAYERS_IMPL
