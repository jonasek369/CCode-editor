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


Layer* top_fullsceen_layer(CCode* ccode){
     for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->draws_fullscreen){
            return ccode->layers[i];
        }
    }
    return NULL;
}

Layer* top_input_consuming_layer(CCode* ccode){
     for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->consume_input){
            return ccode->layers[i];
        }
    }
    return NULL;
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
        if(type == LAYER_CODE && ccode->layers[i]->type == LAYER_SPLIT_VIEW ){
            Layer* sv_layer = ccode->layers[i];
            LayerSplitViewData* lsvd = sv_layer->layer_data;
            if(lsvd->splitten_layers == NULL || arrlen(lsvd->splitten_layers) == 0) 
                continue;
            return lsvd->splitten_layers[lsvd->focused];
        }
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
            if(lcd->query){
                ts_query_delete(lcd->query);
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
    }else if(layer->type == LAYER_CONSOLE){
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
    }else if(layer->type == LAYER_SPLIT_VIEW){
        LayerSplitViewData* lsvd = layer->layer_data;
        if(lsvd){
            if(lsvd->splitten_layers){
                for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
                    free_layer(lsvd->splitten_layers[i]);
                }
                arrfree(lsvd->splitten_layers);
            }
            if(lsvd->virtual_windows){
                for(size_t i = 0; i < arrlenu(lsvd->virtual_windows); i++){
                    free(lsvd->virtual_windows[i]);
                }
                arrfree(lsvd->virtual_windows);
            }
            free(lsvd);
        }
    }else if(layer->type == LAYER_FLOATING_TREE){
        LayerFloatingTreeData* lftd = layer->layer_data;
        if(lftd){
            if(lftd->virtual_window){
                free(lftd->virtual_window);
            }
            if(lftd->search_buffer){
                arrfree(lftd->search_buffer);
            }
            if(lftd->cwd){
                arrfree(lftd->cwd);
            }
            if(lftd->files){
                for(size_t i = 0; i < arrlenu(lftd->files); i++){
                    arrfree(lftd->files[i]);
                }
                arrfree(lftd->files);
            }
            if(lftd->filtered_files){
                for(size_t i = 0; i < arrlenu(lftd->filtered_files); i++){
                    arrfree(lftd->filtered_files[i]);
                }
                arrfree(lftd->filtered_files);
            }
        }
        free(lftd);
    }else if(layer->type == LAYER_FLOATING_DIALOG){
        LayerFloatingDialogData* lfdd = layer->layer_data;
        if(lfdd){
            if(lfdd->message){
                arrfree(lfdd->message);
            }
            if(lfdd->virtual_window){
                free(lfdd->virtual_window);
            }
            free(lfdd);
        }
    }
    
    free(layer);
}


void refresh_tree_files(Layer* layer){
    if(layer->type != LAYER_DIR_WALK){
        return;
    }

    LayerDirWalkData* ldwd = layer->layer_data;
    if(ldwd->current_dir_files != NULL){
        for(size_t i = 0; i < arrlenu(ldwd->current_dir_files); i++){
            arrfree(ldwd->current_dir_files[i]);
        }
        arrfree(ldwd->current_dir_files);
    }
    ldwd->current_dir_files = NULL;
}

/*
    UI
*/

void draw_ui(CCode* ccode) {
    Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    Layer* top_split_view = top_type_layer(ccode, LAYER_SPLIT_VIEW);
    Layer* layer_at_top = top_layer(ccode);

    if(layer_at_top == NULL) {
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);
    if (code_layers != NULL && top_split_view == NULL) {
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
    if(layer_at_top->type != LAYER_CODE && top_split_view != NULL){
        LayerSplitViewData* lsvd = top_split_view->layer_data;
        char top_line[x + 1];
        size_t size = 0;

        int num_layers = arrlen(lsvd->splitten_layers);
        int cells_per_layer = x / num_layers;

        for (int i = 0; i < num_layers; i++) {
            LayerCodeData* lcd = lsvd->splitten_layers[i]->layer_data;
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
        
        for(int i = 0; i < size; i++) {
            if(lsvd->focused * cells_per_layer <= i &&
                i < (lsvd->focused + 1) * cells_per_layer) {
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
    }else if(layer_at_top->type == LAYER_SPLIT_VIEW){
        LayerSplitViewData* lsvd = layer_at_top->layer_data;
    
        if(lsvd->splitten_layers == NULL || arrlenu(lsvd->splitten_layers) == 0){
            goto defer;
        }
        if(lsvd->focused > arrlenu(lsvd->splitten_layers)){
            lsvd->focused = 0;
        }
    
        LayerCodeData* lcd = (LayerCodeData*) lsvd->splitten_layers[lsvd->focused]->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, lcd->cursor->y, lcd->cursor->x);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, lcd->cursor->y, lcd->cursor->x);
    }else if(layer_at_top->type == LAYER_FLOATING_TREE){
        LayerFloatingTreeData* lftd = layer_at_top->layer_data;
        size_t bot_line_size = snprintf(NULL, 0, "%c%d:%d", mode, lftd->selected + lftd->offset, 0);
        mvprintw(y - 1, x - bot_line_size, "%c%d:%d", mode, lftd->selected + lftd->offset, 0);
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
    }else if(layer_at_top->type == LAYER_SPLIT_VIEW){
        LayerSplitViewData* lsvd = layer_at_top->layer_data;
    
        if(lsvd->splitten_layers == NULL || arrlenu(lsvd->splitten_layers) == 0){
            goto defer;
        }
        if(lsvd->focused > arrlenu(lsvd->splitten_layers)){
            lsvd->focused = 0;
        }
    
        LayerCodeData* lcd = (LayerCodeData*) lsvd->splitten_layers[lsvd->focused]->layer_data;
    
        int virt_x, virt_y;
        if (lcd->virtual_window != NULL) {
            virt_x = (lcd->cursor->x - lcd->cursor->xoff) + lcd->virtual_window->x;
            virt_y = (lcd->cursor->y - lcd->cursor->yoff) + lcd->virtual_window->y;
        } else {
            virt_x = lcd->cursor->x - lcd->cursor->xoff;
            virt_y = lcd->cursor->y - lcd->cursor->yoff + 1;
        }
    
        move(virt_y, virt_x);
    }else if(layer_at_top->type == LAYER_FLOATING_TREE){
        LayerFloatingTreeData* lftd = layer_at_top->layer_data;
        move(lftd->virtual_window->y, lftd->virtual_window->x + lftd->search_buffer_x);
    }else if(layer_at_top->type == LAYER_FLOATING_DIALOG){

    }else {
        assert(false && "unknown layer");
    }
defer:
    arrfree(code_layers);
}

#endif // _H_LAYERS_IMPL
