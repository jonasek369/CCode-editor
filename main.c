#include "defines.h"
#include "ccode_mouse.h"
#include "ccode_keys.h"

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

typedef struct {
    bool open_tree;
    char* file;
} Args;

// Runs before curses starts so usage and errors reach the terminal
Args parse_args(int argc, char** argv){
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

    return (Args){
        .open_tree = *open_tree || *open_tree_short,
        .file      = argc > 0 ? argv[0] : NULL,
    };
}

void handle_args(CCode* ccode, Args* args){
    if(args->file){
        read_file_to_code_layer(ccode, args->file, strlen(args->file), true);
    }

    if(args->open_tree){
        Layer* tree = new_layer_dir_walk();
        if(tree){
            push_layer_to_top(ccode, tree);
        }
    }
}



int main(int argc, char** argv) {
    // Unused in release build,
    // otherwise used for section profiling
    (void) prof_pop_print_ns;
    (void) prof_pop_print_ms;

    
    Args args = parse_args(argc, argv);

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

    #ifndef _WIN32
        // Debug prints and LSP server logs would draw over the editor,
        // send them to a log file and give curses its own handle to the terminal
        fflush(stdout);
        FILE* terminal = fdopen(dup(STDOUT_FILENO), "w");
        char log_path[MAX_PATH + 192];
        snprintf(log_path, sizeof(log_path), "%s/ccode.log", path);
        int log_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(terminal && log_fd != -1){
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            setvbuf(stdout, NULL, _IOLBF, 0);
        }
        if(log_fd != -1) close(log_fd);
    #endif

    CCode ccode   = {0};
    ccode.layers  = NULL;
    ccode.config  = load_config(config_path);
    register_lsp_from_config(&ccode);

    int ch;

    // ncurses waits a full second after ESC by default to see if a sequence follows
    set_escdelay(25);
    #ifndef _WIN32
        SCREEN* screen = newterm(NULL, terminal ? terminal : stdout, stdin);
    #else
        initscr();
    #endif
    raw();
    nonl(); // keep Enter as '\r' instead of translating it to '\n'
    keypad(stdscr, TRUE);
    // getch waits for input instead of returning right away, the loop
    // still wakes up regularly to pick up LSP messages
    timeout(INPUT_TIMEOUT_MS);
    noecho();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    // report press and release separately, clicks are resolved in handle_mouse
    mouseinterval(0);
    // flush ncurses' own mouse mode first, it would override drag reporting
    refresh();
    #ifndef _WIN32
        FILE* mouse_terminal = terminal ? terminal : stdout;
    #else
        FILE* mouse_terminal = stdout;
    #endif
    set_mouse_drag_reporting(mouse_terminal, true);
    init_keys();

    move(1, 0);

    init_syntax_highlighting(ccode.config->theme);
    init_lsp_handler();
    init_commands();

    detect_clipboard_support();
    
    handle_args(&ccode, &args);

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

    // Frames left to draw since the last input. Frames without input are skipped,
    // but one more is drawn after input because some state set during a keypress
    // (e.g. PrivateCloseConsole) is only acted on in the following frame
    int frames_to_draw = 2;
    Layer** layers_snapshot = NULL;

    while(ccode.config->PrivateRunning) {
        START_PROFILING();

        #ifdef _WIN32
            QueryPerformanceCounter(&frame_start);
        #else
            clock_gettime(CLOCK_MONOTONIC, &frame_start);
        #endif

        ch = read_key();
        
        // if(ch != -1){
        //     printf("%d\n", ch);
        // }    
        
        if(ch == KEY_MOUSE){
            handle_mouse(&ccode);
            frames_to_draw = 2;
            // we dont redraw on mouse event because
            // mouse event can be mouse move and if user
            // has mouse with poll rate high enough editor starts
            // to lag behind

            // reset the profiler depth because were skipping the frame
            depth--;
            continue;
        }

        if(ch == KEY_RESIZE){
            getmaxyx(stdscr, y, x);
            default_virtual_window.width = x;
            default_virtual_window.height = y;
            ch = -1;
            frames_to_draw = 2;
        }

        // handle LSPs if some are active
        bool lsp_activity = false;
        if(ccode.lsp_ctxs){
            for(size_t i = 0; i < arrlenu(ccode.lsp_ctxs); i++){
                if(handle_lsp(&ccode, ccode.lsp_ctxs[i])){
                    lsp_activity = true;
                }
            }
        }

        if(ch != -1 || lsp_activity){
            frames_to_draw = 2;
        }
        // nothing changed, skip the frame (profiling output updates every frame)
        if(frames_to_draw == 0 && !ccode.config->profiling){
            // reset the profiler depth because were skipping the frame
            depth--;
            continue;
        }
        if(frames_to_draw > 0){
            frames_to_draw--;
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
        // ncurses reserves 64 function keys, KEY_F(0) to KEY_F(63)
        if(ch > KEY_F0 && ch <= KEY_F0+63 && arrlen(ccode.layers) >= 1){
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
        erase();
        /*if(ch != -1){
            print_layers(&ccode);
        }*/
        START_PROFILING();
        // layers can add or remove layers while handling keys, iterate over a copy
        arrsetlen(layers_snapshot, arrlen(ccode.layers));
        if(arrlen(ccode.layers) > 0){
            memcpy(layers_snapshot, ccode.layers, arrlen(ccode.layers) * sizeof(Layer*));
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
    arrfree(layers_snapshot);
    free_ccode(&ccode);

    destroy_commands();
    destroy_syntax_highlighting();
    destory_lsp_handlers();
    destroy_registered_lsps();
    destroy_console_history();

    // Clean up
    set_mouse_drag_reporting(mouse_terminal, false);
    endwin();   // End curses mode
    #ifndef _WIN32
        delscreen(screen);
        if(terminal) fclose(terminal);
    #endif

    return 0;
}


