#ifndef _H_CCODE_MOUSE
#define _H_CCODE_MOUSE

// ncurses only enables click reporting, drag reporting (xterm mode 1002)
// has to be requested from the terminal directly
void set_mouse_drag_reporting(FILE* terminal, bool enable){
    fputs(enable ? "\033[?1002h" : "\033[?1002l", terminal);
    fflush(terminal);
}

// Code layer that mouse selection applies to, NULL if the top layer has none
LayerCodeData* mouse_selection_target(Layer* layer){
    if(layer->type == LAYER_SPLIT_VIEW){
        LayerSplitViewData* lsvd = layer->layer_data;
        return lsvd->splitten_layers[lsvd->focused]->layer_data;
    }
    if(layer->type == LAYER_CODE){
        return layer->layer_data;
    }
    return NULL;
}

// Converts screen position to clamped buffer position, false if outside the buffer
bool mouse_to_buffer_pos(LayerCodeData* lcd, MEVENT* event, int* x, int* y){
    return view_screen_to_pos(lcd, event->y, event->x, y, x, false);
}

// Milliseconds from a monotonic clock, for double clicks
static long long mouse_now_ms(void){
    #ifdef _WIN32
        return (long long)GetTickCount64();
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    #endif
}

// Scrolls a code layer by delta lines without moving its cursor
static void mouse_scroll_code(LayerCodeData* lcd, int delta){
    int n = arrlen(lcd->code_buffer);
    lcd->cursor->yoff += delta;
    if(lcd->cursor->yoff > n - 1) lcd->cursor->yoff = n - 1;
    if(lcd->cursor->yoff < 0) lcd->cursor->yoff = 0;
}

void handle_mouse(CCode* ccode){
    // with mouseinterval(0) ncurses never reports clicks, a release
    // where the button was pressed is treated as one
    static bool dragging = false;
    static int press_x = -1;
    static int press_y = -1;

    MEVENT event;
    if(getmouse(&event) != OK){
        return;
    }

    Layer* current_top_layer = top_layer(ccode);

    if(!current_top_layer){
        return;
    }

    // clicking a tab switches to it
    if(event.y == 0 && (event.bstate & BUTTON1_PRESSED) &&
       (current_top_layer->type == LAYER_CODE || current_top_layer->type == LAYER_SPLIT_VIEW)){
        Layer* tab = tab_at_x(event.x);
        if(tab) focus_tab(ccode, tab);
        dragging = false;
        return;
    }

    // the wheel scrolls the view, the cursor stays where it is
    if((event.bstate & (BUTTON4_PRESSED | BUTTON5_PRESSED)) &&
       (current_top_layer->type == LAYER_CODE || current_top_layer->type == LAYER_SPLIT_VIEW)){
        LayerCodeData* lcd = NULL;
        if(current_top_layer->type == LAYER_CODE){
            lcd = current_top_layer->layer_data;
        }else{
            LayerSplitViewData* lsvd = current_top_layer->layer_data;
            for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
                LayerCodeData* pane = lsvd->splitten_layers[i]->layer_data;
                if(pane->virtual_window && is_inside_virtual_window(event.x, event.y, pane->virtual_window)) lcd = pane;
            }
        }
        if(lcd){
            #ifdef BUTTON5_PRESSED
            mouse_scroll_code(lcd, (event.bstate & BUTTON4_PRESSED) ? -3 : 3);
            #endif
        }
        return;
    }

    // Pressing in a split view pane focuses it, so the selection starts there
    if(event.bstate & BUTTON1_PRESSED && current_top_layer->type == LAYER_SPLIT_VIEW){
        LayerSplitViewData* lsvd = current_top_layer->layer_data;
        for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
            LayerCodeData* lcd = lsvd->splitten_layers[i]->layer_data;
            if(lcd && lcd->virtual_window && is_inside_virtual_window(event.x, event.y, lcd->virtual_window)){
                lsvd->focused = i;
                break;
            }
        }
    }

    LayerCodeData* selection_lcd = mouse_selection_target(current_top_layer);

    static long long last_click_ms = 0;
    static int last_click_x = -1, last_click_y = -1;

    if(event.bstate & BUTTON1_PRESSED){
        dragging = true;
        press_x = event.x;
        press_y = event.y;

        // double click selects the word
        long long now = mouse_now_ms();
        if(selection_lcd && now - last_click_ms < 400 && event.x == last_click_x && event.y == last_click_y){
            int x, y;
            if(mouse_to_buffer_pos(selection_lcd, &event, &x, &y)){
                selection_lcd->cursor->x = x;
                selection_lcd->cursor->y = y;
                layer_code_select_word(selection_lcd);
            }
            dragging = false;
            last_click_ms = 0;
            return;
        }
        last_click_ms = now;
        last_click_x = event.x;
        last_click_y = event.y;

        // Create the base of selection
        if(selection_lcd){
            int x, y;
            if(mouse_to_buffer_pos(selection_lcd, &event, &x, &y)){
                if(!selection_lcd->select_end){
                    selection_lcd->select_end = malloc(sizeof(Cursor));
                }
                selection_lcd->select_end->x = x;
                selection_lcd->select_end->y = y;
                // cursor is the moving end of the selection
                selection_lcd->cursor->x = x;
                selection_lcd->cursor->y = y;
            }else if(selection_lcd->select_end){
                free(selection_lcd->select_end);
                selection_lcd->select_end = NULL;
            }
        }
    }else if(event.bstate & REPORT_MOUSE_POSITION){
        // Drag, moves the end of selection with the mouse
        if(dragging && selection_lcd && selection_lcd->select_end){
            // Dragging onto the tab bar puts the cursor above the view and scrolls up.
            // The code window ends at the screen bottom with no row past it,
            // so the last row is treated as one below to scroll down too
            int screen_h, screen_w;
            (void) screen_w;
            getmaxyx(stdscr, screen_h, screen_w);
            if(event.y >= screen_h - 1){
                event.y = screen_h;
            }

            int x, y;
            if(view_screen_to_pos(selection_lcd, event.y, event.x, &y, &x, true)){
                selection_lcd->cursor->x = x;
                selection_lcd->cursor->y = y;
                selection_lcd->preferred_col = -1;
            }
        }
    }else if(event.bstate & BUTTON1_RELEASED){
        bool was_dragging = dragging;
        dragging = false;
        if(was_dragging && event.x == press_x && event.y == press_y){
            // Click, drop the empty selection made by the press
            if(selection_lcd && selection_lcd->select_end){
                free(selection_lcd->select_end);
                selection_lcd->select_end = NULL;
            }
            event.bstate |= BUTTON1_CLICKED;
        }else if(selection_lcd && selection_lcd->select_end){
            // Creates the end of selection
            int x, y;
            if(mouse_to_buffer_pos(selection_lcd, &event, &x, &y)){
                selection_lcd->cursor->x = x;
                selection_lcd->cursor->y = y;
            }
        }
    }

    // scroll lists with the wheel
    if(event.bstate & BUTTON4_PRESSED){
        current_top_layer->update_function(ccode, current_top_layer, KEY_UP);
    }
    #ifdef BUTTON5_PRESSED
    if(event.bstate & BUTTON5_PRESSED){
        current_top_layer->update_function(ccode, current_top_layer, KEY_DOWN);
    }
    #endif

    if(event.x < 0) event.x = 0;
    if(event.y < 0) event.y = 0;

    if(event.bstate & BUTTON1_CLICKED){
        // handle click based on the type of layer
        switch(current_top_layer->type){
            case LAYER_CODE: {
                LayerCodeData* lcd = current_top_layer->layer_data;
                if(!lcd || !lcd->cursor) break;
                int x, y;
                if(!view_screen_to_pos(lcd, event.y, event.x, &y, &x, false)) break;
                cfp_free(lcd);
                completion_close(lcd);
                lcd->cursor->y = y;
                lcd->cursor->x = x;
                lcd->preferred_col = -1;
                sel_clear(lcd);
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

                    int bx = event.x + console_scroll_offset(lcd, x);
                    if(bx > arrlen(lcd->console_buffer)-1) break;

                    lcd->console_buffer_x = bx;
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
                    int x, y;
                    if(!view_screen_to_pos(lcd, event.y, event.x, &y, &x, false)) break;
                    lcd->cursor->y = y;
                    lcd->cursor->x = x;
                    lcd->preferred_col = -1;
                    sel_clear(lcd);
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