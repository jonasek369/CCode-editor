#include "defines.h"
#include "ccode_mouse.h"

#define FLAG_IMPLEMENTATION
#include "flag.h"

/*

    CCode main

*/


void usage(FILE *stream){
    fprintf(stream, "Usage: ./ccode [OPTIONS] [--] [ARGS] file_path\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

void handle_args(CCode* ccode, int argc, char** argv){
    bool* help            = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool* open_tree       = flag_bool("tree", false, "Open the tree layer on startup");
    bool* open_tree_short = flag_bool("t",    false, "Open the tree layer on startup");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }
    argc = flag_rest_argc();
    argv = flag_rest_argv();

    if (*help) {
        usage(stdout);
        exit(0);
    }

    if(argc > 0){
        read_file_to_code_layer(ccode, argv[0], strlen(argv[0]), true);
    }

    if(*open_tree || *open_tree_short){
        char* cwd = str_to_arr(nob_get_current_dir_temp());
        nob_temp_reset();
        Layer* tree = new_layer_dir_walk(cwd);
        push_layer_to_top(ccode, tree);
    }
}



int main(int argc, char** argv) {
    // Unused in release build,
    // otherwise used for section profiling
    (void) prof_pop_print_ns;
    (void) prof_pop_print_ms;

    
    // For random file ids for LSP
    srand(time(NULL));
    ensure_config_dir_existence();
    char path[MAX_PATH];
    char config_path[MAX_PATH + 192];


    get_config_directory(path, sizeof(path));
    snprintf(config_path, sizeof(config_path), "%s/config.json", path);

    #ifdef __linux__
        char themes_path[MAX_PATH + 192];
        snprintf(themes_path, sizeof(themes_path), "%s/themes", path);
        if(!nob_file_exists(themes_path) && nob_file_exists("/usr/share/ccode-editor/themes")){
            nob_copy_directory_recursively("/usr/share/ccode-editor/themes", themes_path);
        }
        char queries_path[MAX_PATH + 192];
        snprintf(queries_path, sizeof(queries_path), "%s/scm_queries", path);
        if(!nob_file_exists(queries_path) && nob_file_exists("/usr/share/ccode-editor/scm_queries")){
            nob_copy_directory_recursively("/usr/share/ccode-editor/scm_queries", queries_path);
        }
    #endif

    CCode ccode   = {0};
    ccode.layers  = NULL;
    ccode.config  = load_config(config_path);
    register_lsp_from_config(&ccode);

    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();
    mousemask(ALL_MOUSE_EVENTS, NULL);

    move(1, 0);

    init_syntax_highlighting(ccode.config->theme);
    init_lsp_handler();
    init_commands();

    detect_clipboard_support();
    
    handle_args(&ccode, argc, argv);

    if(ccode.layers == NULL){
        arrpush(ccode.layers, new_layer_code());
        layer_code_make_welcome_screen(ccode.layers[0]);
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
        
        // if(ch != -1){
        //     printf("%d\n", ch);
        // }    
        
        if(ch == KEY_MOUSE){
            handle_mouse(&ccode);
            // we dont redraw on mouse event because
            // mouse event can be mouse move and if user
            // has mouse with poll rate high enough editor starts
            // to lag behind

            // reset the profiler depth because were skipping the frame
            depth--;
            continue;
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
            if(arrlenu(ccode.layers) > 1 && ch == CUSTOM_KEY_ESCAPE){
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


            Layer* current_top_layer = top_layer(&ccode);
            Layer* top_console = top_type_layer(&ccode, LAYER_CONSOLE);

            bool removed_cfp = false;

            if(current_top_layer && current_top_layer->type == LAYER_CODE){
                LayerCodeData* code_data = current_top_layer->layer_data;
                if(code_data->completion_function_params != NULL){
                    arrfree(code_data->completion_function_params->index_char_positions);
                    arrfree(code_data->completion_function_params->indecies_size);
                    free(code_data->completion_function_params);
                    code_data->completion_function_params = NULL;
                    removed_cfp = true;
                }
            }
            // If we removed CFP we do not want to open the console that keypress
            if(!removed_cfp){
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
                case LAYER_FLOATING_TREE:
                case LAYER_FLOATING_DIALOG:
                {
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
    destroy_registered_lsps();
    destroy_console_history();

    // Clean up
    endwin();   // End curses mode

    return 0;
}


