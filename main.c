#include "layers.h"
#include "lsp_handler.h"

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
    if(ccode->lsp_ctx){
        destroy_lsp(ccode->lsp_ctx);
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

    handle_args(&ccode, argc, argv);

    if(ccode.layers == NULL){
        arrpush(ccode.layers, new_layer_code());
    }

    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();

    move(1, 0);

    init_syntax_highlighting();
    init_lsp_handlers();

    init_commands();
    while(RUNNING) {
        ch = getch();
        /*
        if(ch != -1){
            printf("%d\n", ch);
        }
        */
        // handle LSP if it is active
        if(ccode.lsp_ctx){
            handle_lsp(&ccode);
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
        // Console switching
        if(ch == CUSTOM_KEY_ESCAPE || CLOSE_CONSOLE){
            Layer* top_console = top_type_layer(&ccode, LAYER_CONSOLE);
        
            if(!top_console && !CLOSE_CONSOLE){
                Layer* new_layer = new_layer_console();
                push_layer_to_top(&ccode, new_layer);
            } else if(top_console){
                int index = contains_layer(&ccode, top_console);
                if(index != -1){
                    free_layer(ccode.layers[index]);
                    arrdel(ccode.layers, index);
                }
        
                if(CLOSE_CONSOLE){
                    CLOSE_CONSOLE = false;
                }
            }
        }
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
        /*if(ch != -1){
            print_layers(&ccode);
        }*/
        for (int i = arrlen(ccode.layers) - 1; i >= 0; i--){
            Layer* layer = ccode.layers[i];
            propagated_ch = ch;
            if(stop_input_propagation < i){
                propagated_ch = -1;
            }
            bool should_draw = (i <= stop_drawing);
            switch(layer->type){
                case LAYER_DIR_WALK: {
                    layer_dir_walk_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    break;
                }
                case LAYER_CODE: {
                    layer_code_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    break;
                }
                case LAYER_CONSOLE: {
                    layer_console_handle_keypress(&ccode, layer, propagated_ch, should_draw);
                    break;
                }
                default: {
                    fprintf(stderr, "Unknown Layer type %d\n", layer->type);
                    break;
                }
            }
        }
        draw_ui(&ccode);
        refresh();
    }
    free_ccode(&ccode);

    destroy_commands();
    destroy_syntax_highlihting();
    destory_lsp_handlers();

    // Clean up
    endwin();   // End curses mode
    wait(NULL);
    return 0;
}


