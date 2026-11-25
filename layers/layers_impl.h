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
        offset += line_length(code->code_buffer[i]);
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

void print_layers(CCode* ccode){
    printf("Layers:\n");
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        Layer* l = ccode->layers[i];
        print_layer(l);
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
            if(lcd->finding_substr){
                free(lcd->finding_substr->substr);
                free(lcd->finding_substr);
            }
            if(lcd->select_end){
                free(lcd->select_end);
            }
            completion_close(lcd);
            if(lcd->completion_function_params){
                arrfree(lcd->completion_function_params->index_char_positions);
                arrfree(lcd->completion_function_params->indecies_size);
                free(lcd->completion_function_params);
            }
            undo_entries_free(&lcd->undo_stack);
            undo_entries_free(&lcd->redo_stack);
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
            if(ltsd->theme_dir_path){
                arrfree(ltsd->theme_dir_path);
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

static int tab_order_cmp(const void* a, const void* b){
    LayerCodeData* la = (*(Layer**)a)->layer_data;
    LayerCodeData* lb = (*(Layer**)b)->layer_data;
    return la->tab_order - lb->tab_order;
}

// Code layers in the order they were opened, as shown in the tab bar
Layer** tabs_in_order(CCode* ccode){
    Layer** tabs = collect_code_layers(ccode);
    if(arrlen(tabs) > 1) qsort(tabs, arrlen(tabs), sizeof(Layer*), tab_order_cmp);
    return tabs;
}

// Brings the code layer at index (in tab order) to the top. Files in a split view focus their pane
void focus_tab(CCode* ccode, Layer* tab){
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        Layer* l = ccode->layers[i];
        if(l == tab){
            push_layer_to_top(ccode, l);
            return;
        }
        if(l->type == LAYER_SPLIT_VIEW){
            LayerSplitViewData* lsvd = l->layer_data;
            for(size_t j = 0; j < arrlenu(lsvd->splitten_layers); j++){
                if(lsvd->splitten_layers[j] == tab){
                    lsvd->focused = (int)j;
                    push_layer_to_top(ccode, l);
                    return;
                }
            }
        }
    }
}

void switch_tab_relative(CCode* ccode, int delta){
    Layer** tabs = tabs_in_order(ccode);
    int n = arrlen(tabs);
    if(n < 2){ arrfree(tabs); return; }
    Layer* active = top_type_layer(ccode, LAYER_CODE);
    int idx = 0;
    for(int i = 0; i < n; i++) if(tabs[i] == active) idx = i;
    idx = ((idx + delta) % n + n) % n;
    focus_tab(ccode, tabs[idx]);
    arrfree(tabs);
}

void switch_tab_index(CCode* ccode, int index){
    Layer** tabs = tabs_in_order(ccode);
    if(index >= 0 && index < arrlen(tabs)) focus_tab(ccode, tabs[index]);
    arrfree(tabs);
}

// Screen columns of the drawn tabs, used for mouse clicks
static struct { int x0, x1; Layer* layer; } tab_hits[64];
static int tab_hits_count = 0;

Layer* tab_at_x(int x){
    for(int i = 0; i < tab_hits_count; i++){
        if(x >= tab_hits[i].x0 && x < tab_hits[i].x1) return tab_hits[i].layer;
    }
    return NULL;
}

static void draw_tab_bar(CCode* ccode, int width){
    tab_hits_count = 0;
    Layer** tabs = tabs_in_order(ccode);
    int n = arrlen(tabs);
    Layer* active = top_type_layer(ccode, LAYER_CODE);
    Layer* top_split = top_type_layer(ccode, LAYER_SPLIT_VIEW);
    LayerSplitViewData* split = NULL;
    if(top_split && top_fullsceen_layer(ccode) == top_split) split = top_split->layer_data;

    attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
    mvhline(0, 0, ' ', width);

    char labels[64][96];
    int widths[64];
    if(n > 64) n = 64;
    int active_idx = 0;
    for(int i = 0; i < n; i++){
        LayerCodeData* lcd = tabs[i]->layer_data;
        const char* name = path_basename(lcd->filename);
        snprintf(labels[i], sizeof(labels[i]), " %d:%s%s ", i + 1, name, lcd->saved ? "" : "*");
        widths[i] = (int)strlen(labels[i]);
        if(tabs[i] == active) active_idx = i;
    }
    // scroll the tabs so the active one is visible
    int first = 0;
    for(;;){
        int sum = first > 0 ? 1 : 0;
        for(int i = first; i <= active_idx; i++) sum += widths[i] + 1;
        if(sum <= width || first >= active_idx) break;
        first++;
    }
    int x = 0;
    if(first > 0){
        mvaddch(0, 0, '<');
        x = 1;
    }
    for(int i = first; i < n && x < width; i++){
        bool is_active = tabs[i] == active;
        bool in_split = false;
        if(split){
            for(size_t j = 0; j < arrlenu(split->splitten_layers); j++) if(split->splitten_layers[j] == tabs[i]) in_split = true;
        }
        if(is_active) attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_BOLD);
        else if(in_split) attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE | A_BOLD);
        else attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
        int w = widths[i];
        if(x + w > width) w = width - x;
        mvaddnstr(0, x, labels[i], w);
        tab_hits[tab_hits_count].x0 = x;
        tab_hits[tab_hits_count].x1 = x + w;
        tab_hits[tab_hits_count].layer = tabs[i];
        tab_hits_count++;
        x += w;
        attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
        if(x < width){ mvaddch(0, x, '|'); x++; }
    }
    if(x >= width && n > 0 && first + 1 < n){
        mvaddch(0, width - 1, '>');
    }
    attrset(A_NORMAL);
    arrfree(tabs);
}

static const char* layer_mode_name(CCode* ccode, Layer* top){
    switch(top->type){
        case LAYER_CONSOLE: return "CONSOLE";
        case LAYER_DIR_WALK: return "TREE";
        case LAYER_THEME_SELECTOR: return "THEMES";
        case LAYER_FLOATING_TREE: return "FIND FILE";
        case LAYER_FLOATING_DIALOG: return "CONFIRM";
        case LAYER_SPLIT_VIEW:
            if(((LayerSplitViewData*)top->layer_data)->move_mode) return "RESIZE";
            break;
        default: break;
    }
    Layer* code = top_type_layer(ccode, LAYER_CODE);
    if(!code) return "EDIT";
    LayerCodeData* lcd = code->layer_data;
    if(lcd->finding_substr) return "FIND";
    if(lcd->completion_function_params) return "SNIPPET";
    if(lcd->select_end) return "SELECT";
    return "EDIT";
}

static void draw_status_bar(CCode* ccode, Layer* top, int y, int width){
    attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
    mvhline(y, 0, ' ', width);

    const char* mode = layer_mode_name(ccode, top);
    attrset(COLOR_PAIR(COLOR_PAIR_KEYWORD) | A_REVERSE | A_BOLD);
    mvprintw(y, 0, " %s ", mode);
    int x = (int)strlen(mode) + 2;

    Layer* code = top_type_layer(ccode, LAYER_CODE);
    LayerCodeData* lcd = code ? code->layer_data : NULL;
    bool code_focused = top->type == LAYER_CODE || top->type == LAYER_SPLIT_VIEW;

    // right side: position and file format
    char right[256] = {0};
    if(lcd){
        int col = view_display_col(lcd->code_buffer[lcd->cursor->y], lcd->cursor->x) + 1;
        int n = arrlen(lcd->code_buffer);
        char sel[48] = {0};
        int sr, sc, er, ec;
        if(sel_range(lcd, &sr, &sc, &er, &ec)){
            if(er > sr) snprintf(sel, sizeof(sel), " (%d lines)", er - sr + 1);
            else snprintf(sel, sizeof(sel), " (%d sel)", ec - sc);
        }
        char find[48] = {0};
        if(lcd->finding_substr){
            int total, cur;
            find_count(lcd, &total, &cur);
            snprintf(find, sizeof(find), "  match %d/%d", cur, total);
        }
        int errors = 0, warnings = 0;
        if(lcd->diagnostics && lcd->allow_diagnostics){
            JsonValue* params = shget(lcd->diagnostics->object, "params");
            JsonValue* diags = params ? shget(params->object, "diagnostics") : NULL;
            for(size_t i = 0; diags && diags->type == JSON_ARRAY && i < arrlenu(diags->array); i++){
                JsonValue* sev = shget(diags->array[i]->object, "severity");
                int s = sev ? (int)json_get_integer(sev) : 1;
                if(s == 1) errors++; else if(s == 2) warnings++;
            }
        }
        char indent[24];
        if(lcd->use_tabs) snprintf(indent, sizeof(indent), "Tabs:%d", view_tab_size());
        else snprintf(indent, sizeof(indent), "Spaces:%d", view_tab_size());
        int w = 0;
        if(find[0]) w += snprintf(right + w, sizeof(right) - w, "%s | ", find + 2);
        if(errors || warnings) w += snprintf(right + w, sizeof(right) - w, "E:%d W:%d | ", errors, warnings);
        snprintf(right + w, sizeof(right) - w, "Ln %d/%d, Col %d%s | %s | %s | %s ",
                 lcd->cursor->y + 1, n, col, sel, indent,
                 lcd->crlf ? "CRLF" : "LF", language_display_name(lcd->filename));
    }
    int right_len = (int)strlen(right);

    // middle: file name and the status message
    attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
    int avail = width - x - right_len - 1;
    bool has_message = status_message[0] && time(NULL) - status_message_time < 6;
    if(lcd && code_focused && avail > 0){
        char name[600];
        // a message needs the room, the full path is only shown without one
        snprintf(name, sizeof(name), " %s%s%s ", has_message ? path_basename(lcd->filename) : lcd->filename, lcd->saved ? "" : " [+]", lcd->read_only ? " [RO]" : "");
        int l = (int)strlen(name);
        if(l > avail){
            // keep the end of long paths
            const char* p = name + (l - avail);
            mvaddnstr(y, x, p, avail);
            x += avail;
        }else{
            attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE | A_BOLD);
            mvaddstr(y, x, name);
            x += l;
        }
        avail = width - x - right_len - 1;
    }
    if(has_message && avail > 2){
        attrset((status_message_error ? COLOR_PAIR(COLOR_PAIR_ERROR) : COLOR_PAIR(COLOR_PAIR_DEFAULT)) | A_REVERSE);
        mvaddstr(y, x, " ");
        mvaddnstr(y, x + 1, status_message, avail - 1);
    }
    attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
    if(right_len < width) mvaddstr(y, width - right_len, right);
    attrset(A_NORMAL);
}

void draw_ui(CCode* ccode) {
    Layer* layer_at_top = top_layer(ccode);
    if(layer_at_top == NULL) {
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);

    draw_tab_bar(ccode, x);

    Layer* console = top_type_layer(ccode, LAYER_CONSOLE);
    if(!console){
        draw_status_bar(ccode, layer_at_top, y - 1, x);
    }

    // terminal cursor
    bool show_cursor = false;
    if(layer_at_top->type == LAYER_CODE || layer_at_top->type == LAYER_SPLIT_VIEW){
        Layer* code = layer_at_top->type == LAYER_CODE ? layer_at_top : layer_split_view_get_active(layer_at_top);
        if(code){
            int sy, sx;
            if(view_cursor_screen(code->layer_data, &sy, &sx)){
                move(sy, sx);
                show_cursor = true;
            }
        }
    }else if(layer_at_top->type == LAYER_CONSOLE){
        LayerConsoleData* lcd = (LayerConsoleData*) layer_at_top->layer_data;
        move(y-1, lcd->console_buffer_x - console_scroll_offset(lcd, x));
        show_cursor = true;
    }else if(layer_at_top->type == LAYER_FLOATING_TREE){
        LayerFloatingTreeData* lftd = layer_at_top->layer_data;
        move(lftd->virtual_window->y, lftd->virtual_window->x + lftd->search_buffer_x);
        show_cursor = true;
    }
    curs_set(show_cursor ? 1 : 0);
}

Layer* layer_split_view_get_active(Layer* split_view){
    if(split_view == NULL || split_view->layer_data == NULL) return NULL;
    LayerSplitViewData* lsvd = split_view->layer_data;
    if(lsvd->splitten_layers == NULL || lsvd->focused == -1) return NULL;
    return lsvd->splitten_layers[lsvd->focused];
}

#endif // _H_LAYERS_IMPL
