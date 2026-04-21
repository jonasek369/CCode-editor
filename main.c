#include "layers.h"


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

    move(1, 0);

    init_syntax_highlighting(ccode.config->theme);
    init_lsp_handler();

    init_commands();
    
    handle_args(&ccode, argc, argv);

    if(ccode.layers == NULL){
        arrpush(ccode.layers, new_layer_code());
    }

    while(ccode.config->PrivateRunning) {
        START_PROFILING();
        ch = getch();

        /*
        if(ch != -1){
            printf("%d\n", ch);
        }
        */

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
        // 64 Reserved
        if(ch >= KEY_F0 && ch <= KEY_F0+64 && top_layer(&ccode)->type != LAYER_CONSOLE){
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
        }
        // Console switching
        if(ch == CUSTOM_KEY_ESCAPE || ccode.config->PrivateCloseConsole){
            // if top layer has opened completion window remove it
            Layer* top = top_type_layer(&ccode, LAYER_CODE);
            if(top){
                LayerCodeData* lcd = top->layer_data;
                if(lcd->completion_window){
                    json_free(lcd->completion_window->completion);
                    free(lcd->completion_window);
                    lcd->completion_window = NULL;
                    continue;
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
        clear();
        END_PROFILING("propagation");
        /*if(ch != -1){
            print_layers(&ccode);
        }*/
        START_PROFILING();
        for (int i = arrlen(ccode.layers) - 1; i >= 0; i--){
            Layer* layer = ccode.layers[i];
            propagated_ch = ch;
            if(stop_input_propagation < i){
                propagated_ch = -1;
            }
            bool should_draw = (i <= stop_drawing);
            switch(layer->type){
                case LAYER_DIR_WALK: {
                    START_PROFILING();
                    layer_dir_walk_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    END_PROFILING("Layer dir walk");
                    break;
                }
                case LAYER_CODE: {
                    START_PROFILING();
                    layer_code_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    END_PROFILING("Layer code");
                    break;
                }
                case LAYER_CONSOLE: {
                    START_PROFILING();
                    layer_console_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    END_PROFILING("Layer console");
                    break;
                }
                case LAYER_THEME_SELECTOR: {
                    START_PROFILING();
                    layer_theme_selector_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    END_PROFILING("Layer theme selector");
                    break;
                }
                default: {
                    fprintf(stderr, "Unknown Layer type %d\n", layer->type);
                    break;
                }
            }
        }
        END_PROFILING("Layers updates");
        START_PROFILING();
        draw_ui(&ccode);
        END_PROFILING("UI");

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
    wait(NULL);
    return 0;
}


