#include "defines.h"

/*

    CCode main

*/

void print_layers(CCode* ccode){
    printf("Layers:\n");
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        Layer* l = ccode->layers[i];
        print_layer(l);
    }
}

void free_ccode(CCode* ccode){
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        free_layer(ccode->layers[i]);
    }
    arrfree(ccode->layers);
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            destroy_lsp(ccode->lsp_ctxs[i]);
        }
        arrfree(ccode->lsp_ctxs);
    }
    if(ccode->config){
        if(ccode->config->theme){
            free_theme(ccode->config->theme);
        }
        free(ccode->config);
    }
}

void handle_args(CCode* ccode, int argc, char** argv){
    if(argc <= 1){
        return;
    }
    if(strncmp(argv[1], "-t", 2) == 0){
        char* cwd = str_to_arr(nob_get_current_dir_temp());
        nob_temp_reset();
        Layer* tree = new_layer_dir_walk(cwd);
        push_layer_to_top(ccode, tree);
    }else{
        read_file_to_code_layer(ccode, argv[1], strlen(argv[1]));
    }
}

bool is_inside_virtual_window(int x, int y, VirtualWindow* w){
    return (
        x >= w->x &&
        x <  w->x + w->width &&
        y >= w->y &&
        y <  w->y + w->height
    );
}

// XCurses*lines:  78
// XCurses*cols:   320
void handle_mouse(CCode* ccode){
    MEVENT event;
    nc_getmouse(&event);
    // printf("%d\n", event.bstate);

    Layer* current_top_layer = top_layer(ccode);

    if(!current_top_layer){
        return;
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
            
                lcd->cursor->y = target_y;
            
                char* line = lcd->code_buffer[target_y];
                int len = arrlen(line)-2;
            
                int target_x = lcd->cursor->xoff + event.x;
            
                if(target_x > len) target_x = len;
                if(target_x < 0) target_x = 0;
            
                lcd->cursor->x = target_x;
            
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
            }
            default: {
                break;
            }
        }
    }
}

int main(int argc, char** argv) {
    // For random file ids for LSP
    srand(time(NULL));
    CCode ccode   = {0};
    ccode.layers  = NULL;
    ccode.config  = load_config("./config.json");

    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    move(1, 0);

    init_syntax_highlighting(ccode.config->theme);
    init_lsp_handler();

    init_commands();
    
    handle_args(&ccode, argc, argv);

    if(ccode.layers == NULL){
        arrpush(ccode.layers, new_layer_code());
    }

    #ifdef _WIN32
        init_timer();
        LARGE_INTEGER frame_start;
    #else
        struct timespec frame_start;
    #endif

    int x, y;
    getmaxyx(stdscr, y, x);


    default_virtual_window.width = x;
    default_virtual_window.height = y;

    while(ccode.config->PrivateRunning) {
        START_PROFILING();
        #ifdef _WIN32
            QueryPerformanceCounter(&frame_start);
        #else
            clock_gettime(CLOCK_MONOTONIC, &frame_start);
        #endif
        ch = getch();

        /*
        if(ch != -1){
            printf("%d\n", ch);
        }
        */

        if(ch == KEY_MOUSE){
            handle_mouse(&ccode);
        }


        // handle LSPs if some are active
        if(ccode.lsp_ctxs){
            for(size_t i = 0; i < arrlenu(ccode.lsp_ctxs); i++){
                handle_lsp(&ccode, ccode.lsp_ctxs[i]);
            }
        }

        // Code switch
        if(ch == CTL_TAB && arrlen(ccode.layers) >= 2 && top_layer(&ccode)->type != LAYER_CONSOLE){
            Layer* top = top_type_layer(&ccode, LAYER_CODE);
            if(top == NULL){
                continue;
            }
            remove_layer(&ccode, top);
            Layer* next = top_type_layer(&ccode, LAYER_CODE);
            if(next == NULL){
                push_layer_to_top(&ccode, top);
                continue;
            }
            push_layer_to_top(&ccode, next);
            push_layer_to_bot(&ccode, top);
        }
        // 64 Reserved by pdcurses
        if(ch > KEY_F0 && ch <= KEY_F0+64 && arrlen(ccode.layers) >= 1){
            if(top_layer(&ccode)->type == LAYER_CODE){
                int index = (ch-KEY_F0)-1;
                Layer** code_layers = all_type_layers(&ccode, LAYER_CODE);
                if(arrlen(code_layers) <= index){
                    arrfree(code_layers);
                }else{
                    Layer* layer = code_layers[index];
                    remove_layer(&ccode, layer);
                    push_layer_to_top(&ccode, layer);
                    arrfree(code_layers);
                } 
            }else if(top_layer(&ccode)->type == LAYER_SPLIT_VIEW){
                int index = (ch-KEY_F0)-1;
                if(index >= 0 && index <= 1){
                    LayerSplitViewData* lsvd = top_layer(&ccode)->layer_data;
                    lsvd->focused = index;
                }
            }
        }
        // Console switching
        if(ch == CUSTOM_KEY_ESCAPE || ccode.config->PrivateCloseConsole){
            // if top layer has opened completion window remove it
            if(arrlenu(ccode.layers) >= 1 && ch == CUSTOM_KEY_ESCAPE){
                int index = top_layer(&ccode)->type == LAYER_CONSOLE ? 1 : 0;
                switch(ccode.layers[index]->type){
                    case(LAYER_CODE): {
                        LayerCodeData* lcd = ccode.layers[index]->layer_data;
                        if(lcd->completion_window){
                            json_free(lcd->completion_window->completion);
                            free(lcd->completion_window);
                            lcd->completion_window = NULL;
                            continue;
                        }
                        break;
                    }
                    case(LAYER_FLOATING_TREE): {
                        Layer* ft = ccode.layers[index];
                        remove_layer(&ccode, ft);
                        free_layer(ft);
                        continue;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            Layer* top_console = top_type_layer(&ccode, LAYER_CONSOLE);
        
            if(!top_console && !ccode.config->PrivateCloseConsole){
                Layer* new_layer = new_layer_console();
                push_layer_to_top(&ccode, new_layer);
            } else if(top_console){
                int index = contains_layer(&ccode, top_console);
                if(index != -1){
                    free_layer(ccode.layers[index]);
                    arrdel(ccode.layers, index);
                }
        
                if(ccode.config->PrivateCloseConsole){
                    ccode.config->PrivateCloseConsole = false;
                }
            }
        }
        START_PROFILING();
        // figure out where to stop propagation
        int stop_input_propagation = arrlen(ccode.layers);
        for(size_t i = 0; i < arrlenu(ccode.layers); i++){
            if(ccode.layers[i]->consume_input){
                stop_input_propagation = i;
                break;
            }
        }
        int stop_drawing = arrlen(ccode.layers);
        for(size_t i = 0; i < arrlenu(ccode.layers); i++){
            if(ccode.layers[i]->draws_fullscreen){
                stop_drawing = i;
                break;
            }
        }
        int propagated_ch = ch;
        END_PROFILING("propagation");
        clear();
        /*if(ch != -1){
            print_layers(&ccode);
        }*/
        START_PROFILING();
        Layer** layers_snapshot = NULL;
        for(int i = 0; i < arrlen(ccode.layers); i++){
            arrput(layers_snapshot, ccode.layers[i]);
        }
        for (int i = arrlen(layers_snapshot) - 1; i >= 0; i--){
            Layer* layer = layers_snapshot[i];
            propagated_ch = ch;
            if(stop_input_propagation < i){
                propagated_ch = -1;
            }
            bool should_draw = (i <= stop_drawing);
            switch(layer->type){
                case LAYER_DIR_WALK: 
                case LAYER_CODE: 
                case LAYER_CONSOLE: 
                case LAYER_THEME_SELECTOR:
                case LAYER_SPLIT_VIEW:
                case LAYER_FLOATING_TREE: {
                    layer->handle_keypress_function(&ccode, layer, propagated_ch, should_draw);
                    break;
                }
                default: {
                    fprintf(stderr, "Unknown Layer type %d\n", layer->type);
                    break;
                }
            }
        }
        arrfree(layers_snapshot);

        END_PROFILING("Layers updates");
        START_PROFILING();
        draw_ui(&ccode);
        END_PROFILING("UI");
        #if LIMIT_FPS == 1
            START_PROFILING();
            sleep_until_next_frame(frame_start);
            END_PROFILING("sleep_until_next_frame");
        #endif
        END_PROFILING("Frame");
        if(ccode.config->profiling){
            prof_print(stdscr);
        }
        prof_reset();
        refresh();
    }
    free_ccode(&ccode);

    destroy_commands();
    destroy_syntax_highlighting();
    destory_lsp_handlers();

    arrfree(installed_lsps);

    // Clean up
    endwin();   // End curses mode

    return 0;
}


