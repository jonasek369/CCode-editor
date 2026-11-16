#ifndef _H_CCODE_MOUSE
#define _H_CCODE_MOUSE

void handle_mouse(CCode* ccode){
    MEVENT event;
    nc_getmouse(&event);

    Layer* current_top_layer = top_layer(ccode);

    if(!current_top_layer){
        return;
    }

    if(
        event.bstate & BUTTON1_PRESSED &&
        (current_top_layer->type == LAYER_CODE || current_top_layer->type == LAYER_SPLIT_VIEW)
    ){
        // Create the base of selection
        LayerCodeData* lcd = NULL;
        if(current_top_layer->type == LAYER_SPLIT_VIEW){
            LayerSplitViewData* lsvd = current_top_layer->layer_data;
            lcd = lsvd->splitten_layers[lsvd->focused]->layer_data; 
        }else {
            lcd = current_top_layer->layer_data;
        }        
        if(!lcd && lcd->select_end){
            return;
        }
        lcd->select_end = malloc(sizeof(Cursor));
        int target_y = lcd->cursor->yoff + event.y - 1;

        if(target_y < 0 || target_y >= arrlen(lcd->code_buffer)){
            free(lcd->select_end);
            lcd->select_end = NULL;
            return;
        }

        lcd->select_end->y = target_y;
            
        char* line = lcd->code_buffer[target_y];
        int len = arrlen(line)-2;
            
        int target_x = lcd->cursor->xoff + event.x;
            
        if(target_x > len) target_x = len;
        if(target_x < 0) target_x = 0;
            
        lcd->select_end->x = target_x;
    }else if(
        event.bstate & BUTTON1_RELEASED &&
        (current_top_layer->type == LAYER_CODE || current_top_layer->type == LAYER_SPLIT_VIEW)
    ){
        // Creates the end of selection
        LayerCodeData* lcd = NULL;
        if(current_top_layer->type == LAYER_SPLIT_VIEW){
            LayerSplitViewData* lsvd = current_top_layer->layer_data;
            lcd = lsvd->splitten_layers[lsvd->focused]->layer_data; 
        }else {
            lcd = current_top_layer->layer_data;
        }  
        if(!lcd && !lcd->select_end){
            return;
        }
        int target_y = lcd->cursor->yoff + event.y - 1;

        if(target_y < 0 || target_y >= arrlen(lcd->code_buffer))
            return;

        lcd->cursor->y = target_y;
            
        char* line = lcd->code_buffer[target_y];
        int len = arrlen(line)-2;
            
        int target_x = lcd->cursor->xoff + event.x;
            
        if(target_x > len) target_x = len;
        if(target_x < 0) target_x = 0;
            
        lcd->cursor->x = target_x;
    }   

    // scroll up
    if(event.bstate & BUTTON4_PRESSED){
        current_top_layer->update_function(ccode, current_top_layer, KEY_UP);
    }

    // scroll down
    if(event.bstate & BUTTON5_PRESSED){
        current_top_layer->update_function(ccode, current_top_layer, KEY_DOWN);
    }

    if(event.x < 0) event.x = 0;
    if(event.y < 0) event.y = 0;

    if(event.bstate & BUTTON1_CLICKED){
        // handle click based on the type of layer
        switch(current_top_layer->type){
            case LAYER_CODE: {
                LayerCodeData* lcd = current_top_layer->layer_data;
                if(!lcd || !lcd->cursor) break;
            
                int target_y = lcd->cursor->yoff + event.y - 1;

                if(target_y < 0 || target_y >= arrlen(lcd->code_buffer))
                    break;

                if(lcd->completion_function_params){
                    arrfree(lcd->completion_function_params->index_char_positions);
                    arrfree(lcd->completion_function_params->indecies_size);
                    free(lcd->completion_function_params);
                    lcd->completion_function_params = NULL;
                }

                lcd->cursor->y = target_y;
            
                char* line = lcd->code_buffer[target_y];
                int len = arrlen(line)-2;
            
                int target_x = lcd->cursor->xoff + event.x;
            
                if(target_x > len) target_x = len;
                if(target_x < 0) target_x = 0;
            
                lcd->cursor->x = target_x;
                
                if(lcd->select_end){
                    free(lcd->select_end);
                    lcd->select_end = NULL;
                }

                break;
            }
            case LAYER_DIR_WALK: {
                LayerDirWalkData* ldwd = current_top_layer->layer_data;
                if(!ldwd) break;
            
                int header_offset = 3;
                int list_height = event.y - header_offset;
                if(list_height < 0) break;
            
                int idx = event.y - header_offset + ldwd->offset;
            
                if(idx < 0) break;
                if(idx >= arrlen(ldwd->current_dir_files)) break;
            
                if(ldwd->selected == idx){
                    layer_dir_walk_open(ccode, current_top_layer, ldwd);
                } else {
                    ldwd->selected = idx;
                }
                break;
            }
            case LAYER_THEME_SELECTOR: {
                LayerThemeSelectorData* ltsd = current_top_layer->layer_data;
                if(!ltsd) break;
            
                int header_offset = 1;
            
                int screen_row = event.y - header_offset;
                if(screen_row < 0) break;
            
                int idx = screen_row + ltsd->offset;
            
                if(idx < 0 || idx >= arrlen(ltsd->current_dir_files)) break;
            
                ltsd->selected = idx;
                layer_theme_selector_set_theme(ccode, ltsd);
            
                break;
            }
            case LAYER_CONSOLE: {
                int x, y;
                (void) x;
                getmaxyx(stdscr, y, x);
                if(event.y == y-1){
                    LayerConsoleData* lcd = current_top_layer->layer_data;

                    if(event.x > arrlen(lcd->console_buffer)-1) break;

                    lcd->console_buffer_x = event.x;
                }
                break;
            }
            case LAYER_SPLIT_VIEW: {
                LayerSplitViewData* lsvd = current_top_layer->layer_data;
                for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
                    LayerCodeData* lcd = lsvd->splitten_layers[i]->layer_data;
                    if(!lcd || !lcd->virtual_window) continue;
                    if(!is_inside_virtual_window(event.x, event.y, lcd->virtual_window)) continue;
                    lsvd->focused = i;
                    int target_y = lcd->cursor->yoff + event.y - lcd->virtual_window->y;
            
                    if(target_y < 0 || target_y >= arrlen(lcd->code_buffer))
                        break;
                
                    lcd->cursor->y = target_y;
                
                    char* line = lcd->code_buffer[target_y];
                    int len = arrlen(line)-2;
                
                    int target_x = lcd->cursor->xoff + event.x - lcd->virtual_window->x;
                
                    if(target_x > len) target_x = len;
                    if(target_x < 0) target_x = 0;
                
                    lcd->cursor->x = target_x;
                
                    break;
                }
                break;
            }
            case LAYER_FLOATING_TREE: {
                LayerFloatingTreeData* lftd = current_top_layer->layer_data;
                if(event.y == lftd->virtual_window->y){
                    if(event.x-lftd->virtual_window->x > arrlen(lftd->search_buffer)-1) break;
                    if(event.x < lftd->virtual_window->x) break;

                    lftd->search_buffer_x = event.x - lftd->virtual_window->x;
                }
                if(
                    event.y > lftd->virtual_window->y &&
                    event.y <= lftd->virtual_window->y + lftd->virtual_window->height+1
                ){
                    int selecting = event.y - lftd->virtual_window->y-1 + lftd->offset;
                    if(selecting < 0 || selecting >= arrlen(lftd->filtered_files)) break;
                    if(lftd->selected == selecting){
                        layer_floating_tree_load_file(ccode, current_top_layer, lftd);
                    }else{
                        lftd->selected = selecting;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

#endif // _H_CCODE_MOUSE