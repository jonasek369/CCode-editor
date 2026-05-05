#ifndef _H_LAYER_CONSOLE
#define _H_LAYER_CONSOLE


void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);

Layer* new_layer_console(){
    Layer* console = malloc(sizeof(Layer));
    if(!console){
        return NULL;
    }
    console->type = LAYER_CONSOLE;
    console->consume_input = true;
    console->draws_fullscreen = false;
    console->handle_keypress_function = &layer_console_handle_keypress;

    LayerConsoleData* lcd = malloc(sizeof(LayerConsoleData));
    if(!lcd){
        free(console);
        return NULL;
    }
    lcd->console_buffer = NULL;
    arrput(lcd->console_buffer, '\0');
    lcd->console_buffer_x = 0;
    console->layer_data = lcd;
    return console;
}


char* run_command_buffer(const char* cmd){
    FILE* fp = popen(cmd, "r");
    if (!fp) return NULL;

    char* buf = NULL;
    char tmp[1024];

    while (fgets(tmp, sizeof(tmp), fp)) {
        for (char* p = tmp; *p; p++) {
            if(*p == '\n'){
                continue;
            }
            arrput(buf, *p);
        }
    }

    pclose(fp);

    arrput(buf, '\0');
    return buf;
}


void message_to_console(CCode* ccode, const char* message){
    if(ccode->config->PrivateCloseConsole){
        ccode->config->PrivateCloseConsole = false;
    }
    Layer* console = top_type_layer(ccode, LAYER_CONSOLE);
    LayerConsoleData* layer_console_data = (LayerConsoleData*) console->layer_data;
    size_t zero = 0;
    arrsetlen(layer_console_data->console_buffer, zero);

    for(size_t i = 0; i < strlen(message); i++){
        arrput(layer_console_data->console_buffer, message[i]);
    }

    arrput(layer_console_data->console_buffer, '\0');
    layer_console_data->console_buffer_x = arrlen(layer_console_data->console_buffer)-1;
}


void console_execute_command(CCode* ccode, const char* buffer){
    TokenizationOutput to = {0};
    if(tokenize_console_command(buffer, &to)){
        fprintf(stderr, "Error while tokenizing console command\n");
        return;
    }
    if (arrlen(to.tokens) < 1 || to.tokens[0].type != TOKEN_COMMAND) {
        goto defer;
    }

    switch (to.tokens[0].command_type) {
        case COMMAND_QUIT: {
            ccode->config->PrivateRunning = false;
            break;
        }

        case COMMAND_OPEN: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                read_file_to_code_layer(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
            }
            break;
        }

        case COMMAND_GOTO: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_INTEGER && to.tokens[1].integer >= 0) {
                Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
                LayerCodeData* lcd = top_code_layer->layer_data;
                if (!lcd->cursor) break;

                lcd->cursor->y = to.tokens[1].integer > arrlen(lcd->code_buffer) ? arrlen(lcd->code_buffer) : to.tokens[1].integer;
                if (arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER && to.tokens[2].integer >= 0) {
                    lcd->cursor->x = to.tokens[2].integer;
                }
            }
            break;
        }

        case COMMAND_CHANGE_NAME: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
            }
            break;
        }

        case COMMAND_WRITE: {
            write_code_layer_to_file(ccode);
            break;
        }

        case COMMAND_SYS: {
            if (arrlen(to.tokens) >= 2) {
                char* command = NULL;
                for(size_t i = 1; i < arrlenu(to.tokens); i++){
                    if(to.tokens[i].type == TOKEN_STRING){
                        for(size_t j = 0; j < to.tokens[i].string.size; j++){
                            arrput(command, to.tokens[i].string.start[j]);
                        }
                    }else if(to.tokens[i].type == TOKEN_INTEGER){
                        char itoa_str[64];
                        itoa(to.tokens[i].integer, itoa_str, 10);
                        for(size_t j = 0; j < strlen(itoa_str); j++){
                            arrput(command, itoa_str[j]);
                        }
                    }
                    if(i != arrlenu(to.tokens)-1){
                        arrput(command, ' ');
                    }
                }
                arrput(command, '\0');

                char* output = run_command_buffer(command);

                if(output == NULL){
                    arrfree(command);
                    goto defer;
                }

                if (arrlen(output) > 1) {
                    message_to_console(ccode, output);
                }else {
                    message_to_console(ccode, "Command didnt respond");
                }
                arrfree(output);
                arrfree(command);
            }
            break;
        }

        case COMMAND_SAVE: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
                write_code_layer_to_file(ccode);
            }
            break;
        }

        case COMMAND_FIND: {
            if (arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING) {
                int32_t nth_occurence = 1;
                if(arrlen(to.tokens) >= 3 && to.tokens[2].type == TOKEN_INTEGER){
                    nth_occurence = to.tokens[2].integer;
                }
                Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
                if(top_code_layer){
                    LayerCodeData* lcd = top_code_layer->layer_data;
                    if(lcd->finding_substr != NULL){
                        free(lcd->finding_substr->substr);
                        free(lcd->finding_substr);
                        lcd->finding_substr = NULL;
                    }
                }
                find_jump(ccode, to.tokens[1].string.start, to.tokens[1].string.size, nth_occurence);
            }
            break;
        }

        case COMMAND_CLOSE: {
            Layer* second_layer = layer_at_index(ccode, 1); // first will always be console
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE){
                close_code_layer(ccode, false);
            }else if(second_layer->type == LAYER_DIR_WALK || second_layer->type == LAYER_THEME_SELECTOR){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }
            break;
        }

        case COMMAND_FORCE_CLOSE: {
            Layer* second_layer = layer_at_index(ccode, 1); // first will always be console
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE){
                close_code_layer(ccode, true);
            }else if(second_layer->type == LAYER_DIR_WALK){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }
            break;
        }

        case COMMAND_TREE: {
            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                char* dir_arr = NULL;

                if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING){
                    char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                    dir_arr = str_to_arr(as_str);
                    free(as_str);
                }

                if(dir_arr == NULL){
                    const char* cwd = nob_get_current_dir_temp();
                    if(cwd == NULL){
                        printf("Could not get cwd!\n");
                        break;
                    }
                    dir_arr = str_to_arr(cwd);
                    nob_temp_reset();
                }

                Layer* tree = new_layer_dir_walk(dir_arr);
                push_layer_to_top(ccode, tree);
            }else{
                LayerDirWalkData* ldwd = top_tree_layer->layer_data;

                char* dir_arr = NULL;

                if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING){
                    char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                    dir_arr = str_to_arr(as_str);
                    free(as_str);
                }

                if(dir_arr == NULL && ldwd->current_dir_path == NULL){
                    const char* cwd = nob_get_current_dir_temp();
                    if(cwd == NULL){
                        printf("Could not get cwd!\n");
                        break;
                    }
                    dir_arr = str_to_arr(cwd);
                    nob_temp_reset();
                }

                if(dir_arr != NULL){
                    change_tree_path(top_tree_layer, dir_arr);
                }

                push_layer_to_top(ccode, top_tree_layer);
            }
            break;
        }

        case COMMAND_SET_TAB_SIZE: {
            if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_INTEGER){
                ccode->config->tab_size = to.tokens[1].integer;
            }
            break;
        }

        case COMMAND_TREE_CHANGE_DIR: {
            if(arrlen(to.tokens) <= 1){
                goto defer;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot change directory outside tree");
                goto defer;
            }

            LayerDirWalkData* ldwd = top_tree_layer->layer_data;

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                if(strncmp(as_str, "..", 2) == 0){
                    char* new_dir = nob_temp_sprintf("%s/%s", ldwd->current_dir_path, as_str);
                    char resolved_path[MAX_PATH];
                    if(resolve_path(new_dir, resolved_path) == NULL){
                        free(as_str);
                        nob_temp_reset();
                        goto defer;
                    }
                    change_tree_path(top_tree_layer, str_to_arr(resolved_path));
                    nob_temp_reset();
                }else {
                    if(!nob_file_exists(as_str)){
                        message_to_console(ccode, "invalid path");
                    }
                    change_tree_path(top_tree_layer, str_to_arr(as_str));
                }

                free(as_str);
            }

            break;
        }

        case COMMAND_PROFILING: {
            ccode->config->profiling = !ccode->config->profiling;
            break;
        }

        case COMMAND_THEME: {
            if(arrlen(to.tokens) >= 2 && to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                printf("loading %s\n", as_str);
                ColorTheme* new_theme = load_theme(as_str);
                if(new_theme){
                    free_theme(ccode->config->theme);
                    ccode->config->theme = new_theme;
                    init_syntax_colors(ccode->config->theme);
                }
                free(as_str);
            }else if (arrlen(to.tokens) >= 1){
                Layer* layer = new_layer_theme_selector();
                push_layer_to_top(ccode, layer);
            }
            break;
        }

        case COMMAND_WRITE_CONFIG: {
            save_config(ccode->config);
            break;
        }

        case COMMAND_MAKE_DIRECTORY: {
            if(arrlen(to.tokens) <= 1){
                goto defer;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot make directory outside tree");
                goto defer;
            }

            LayerDirWalkData* ldwd = top_tree_layer->layer_data;

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);

                char* full_path = nob_temp_sprintf("%s/%s", ldwd->current_dir_path, as_str);

                bool status = nob_mkdir_if_not_exists(full_path);
                if(!status){
                    char* message = nob_temp_sprintf("Directory '%s' could not be created!", as_str);
                    message_to_console(ccode, message);
                }else{
                    char* message = nob_temp_sprintf("Directory '%s' was created!", as_str);
                    message_to_console(ccode, message);
                }
                nob_temp_reset();
                free(as_str);
                char* dir = str_to_arr(ldwd->current_dir_path);
                change_tree_path(top_tree_layer, dir);
            }
            break;
        }

        case COMMAND_MAKE_FILE: {
            if(arrlen(to.tokens) <= 1){
                goto defer;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot make file outside tree");
                goto defer;
            }

            LayerDirWalkData* ldwd = top_tree_layer->layer_data;

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);

                char* full_path = nob_temp_sprintf("%s/%s", ldwd->current_dir_path, as_str);

                bool status = nob_write_entire_file(full_path, NULL, 0);
                if(!status){
                    char* message = nob_temp_sprintf("File '%s' could not be created!", as_str);
                    message_to_console(ccode, message);
                }else{
                    char* message = nob_temp_sprintf("File '%s' was created!", as_str);
                    message_to_console(ccode, message);
                }
                nob_temp_reset();
                free(as_str);
                char* dir = str_to_arr(ldwd->current_dir_path);
                change_tree_path(top_tree_layer, dir);
            }
            break;
        }

        default: {
            break;
        }
    }

    printf("Tokenization output:\n");
    for(size_t i = 0; i < arrlenu(to.tokens); i++ ){
        if(to.tokens[i].type == TOKEN_STRING || to.tokens[i].type == TOKEN_COMMAND){
            printf("  %d (%.*s)\n", to.tokens[i].type, to.tokens[i].string.size, to.tokens[i].string.start);
        }else if(to.tokens[i].type == TOKEN_INTEGER){
            printf("  %d N=%d\n", to.tokens[i].type, to.tokens[i].integer);
        }
    }
defer:
    arrfree(to.tokens);
    free(to.string_storage);
}


void layer_console_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
    (void)y;
    getmaxyx(stdscr, y, x);
    if((chr >= 0 && chr <= 255) && isprint(chr)){
        int line_len = arrlen(console_data->console_buffer);

        if(console_data->console_buffer_x > line_len - 1){
            console_data->console_buffer_x = line_len - 1;
        }

        if(line_len > 0){
            (void) arrpop(console_data->console_buffer);
        }

        if(console_data->console_buffer_x >= 0 && console_data->console_buffer_x <= arrlen(console_data->console_buffer)){
            arrins(console_data->console_buffer, console_data->console_buffer_x, (char)chr);
        } else {
            arrput(console_data->console_buffer, (char)chr);
        }
        arrput(console_data->console_buffer, '\0');

        if(console_data->console_buffer_x + 1 < x){
            console_data->console_buffer_x++;
        }
    } else if(chr == CUSTOM_KEY_ENTER){
        ccode->config->PrivateCloseConsole = true;
        console_execute_command(ccode, console_data->console_buffer);
    } else if(chr == CUSTOM_KEY_BACKSPACE){
        if(console_data->console_buffer_x > 0 && arrlen(console_data->console_buffer) > 1){
            (void) arrpop(console_data->console_buffer);
            arrdel(console_data->console_buffer, console_data->console_buffer_x - 1);
            arrput(console_data->console_buffer, '\0');

            console_data->console_buffer_x--;
        }
    } else if(chr == KEY_LEFT){
        if(console_data->console_buffer_x != 0){
            console_data->console_buffer_x--;
        }
    } else if(chr == KEY_RIGHT){
        if(console_data->console_buffer_x < arrlen(console_data->console_buffer)-1){
            console_data->console_buffer_x++;
        }
    }
}


void layer_console_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
    getmaxyx(stdscr, y, x);

    char top_line[x+1];
    size_t size = snprintf(top_line, x + 1, "%s", console_data->console_buffer);
    if ((int)size > x) size = x;
    for (int i = size; i < x; i++) top_line[i] = ' ';
    top_line[x] = '\0';

    attron(A_REVERSE);
    mvprintw(y-1, 0, "%s", top_line);
    attroff(A_REVERSE);
}


void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    layer_console_update(ccode, layer, chr);
    END_PROFILING("layer_console_update");
    if(should_draw){
        START_PROFILING();
        layer_console_render(ccode, layer);
        END_PROFILING("layer_console_render");
    }
    END_PROFILING("layer_console");
}

#endif // _H_LAYER_CONSOLE
