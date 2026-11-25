#ifndef _H_LAYER_CONSOLE
#define _H_LAYER_CONSOLE


void layer_console_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);
bool layer_console_update(CCode* ccode, Layer* layer, int chr);

// Defined outside the layer because we need this to persisnt between console opens
static char** console_history = NULL;
static int64_t console_history_index = 0;
static char* console_history_draft = NULL;
static bool console_history_navigating = false;


bool has_token_types(Token* tokens, ...){
    va_list args;
    va_start(args, tokens);

    bool result = true;
    int i = 1;
    int len = arrlen(tokens);

    while (true){
        ConsoleTokenType expected = va_arg(args, ConsoleTokenType);

        if(expected == TOKEN_INVALID)
            break;

        if(i >= len || tokens[i].type != expected){
            result = false;
            break;
        }

        i++;
    }

    va_end(args);

    return result;
}


void destroy_console_history(){
    if(console_history){
        for(int i = 0; i < arrlen(console_history); i++){
            arrfree(console_history[i]);
        }
        arrfree(console_history);
    }
    if(console_history_draft){
        arrfree(console_history_draft);
    }
}


Layer* new_layer_console(){
    Layer* console = malloc(sizeof(Layer));
    if(!console){
        return NULL;
    }
    console->type = LAYER_CONSOLE;
    console->consume_input = true;
    console->draws_fullscreen = false;
    console->handle_keypress_function = &layer_console_handle_keypress;
    console->update_function = &layer_console_update;

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
    if(console == NULL){
        set_status(true, "%s", message);
        return;
    }
    LayerConsoleData* layer_console_data = (LayerConsoleData*) console->layer_data;
    size_t zero = 0;
    arrsetlen(layer_console_data->console_buffer, zero);

    for(size_t i = 0; i < strlen(message); i++){
        arrput(layer_console_data->console_buffer, message[i]);
    }

    arrput(layer_console_data->console_buffer, '\0');
    layer_console_data->console_buffer_x = arrlen(layer_console_data->console_buffer)-1;
}



// Every code layer, including the ones inside split views
Layer** collect_code_layers(CCode* ccode){
    Layer** out = NULL;
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        Layer* l = ccode->layers[i];
        if(l->type == LAYER_CODE){
            arrput(out, l);
        }else if(l->type == LAYER_SPLIT_VIEW){
            LayerSplitViewData* lsvd = l->layer_data;
            for(size_t j = 0; j < arrlenu(lsvd->splitten_layers); j++) arrput(out, lsvd->splitten_layers[j]);
        }
    }
    return out;
}

// Number of code layers with unsaved changes, the first one's name is stored in first
int count_unsaved(CCode* ccode, const char** first){
    Layer** all = collect_code_layers(ccode);
    int count = 0;
    for(size_t i = 0; i < arrlenu(all); i++){
        LayerCodeData* lcd = all[i]->layer_data;
        if(!lcd->saved){
            if(count == 0 && first) *first = lcd->filename;
            count++;
        }
    }
    arrfree(all);
    return count;
}

// Raw text after the command name, the find/replace arguments keep their exact
// spelling this way (numbers, leading dashes, ...). Quoted arguments may contain spaces
static const char* console_args_start(const char* buffer){
    const char* p = buffer;
    while(*p && !is_whitespace[(uint8_t)*p]) p++;
    while(*p && is_whitespace[(uint8_t)*p]) p++;
    return p;
}

// Reads one argument (quoted or up to whitespace) into a malloc'd string
static char* console_next_arg(const char** p){
    const char* s = *p;
    while(*s && is_whitespace[(uint8_t)*s]) s++;
    if(!*s){ *p = s; return NULL; }
    Nob_String_Builder sb = {0};
    if(*s == '"'){
        s++;
        while(*s && *s != '"'){
            if(*s == '\\' && (s[1] == '"' || s[1] == '\\')) s++;
            nob_da_append(&sb, *s);
            s++;
        }
        if(*s == '"') s++;
    }else{
        while(*s && !is_whitespace[(uint8_t)*s]){
            nob_da_append(&sb, *s);
            s++;
        }
    }
    nob_da_append(&sb, '\0');
    *p = s;
    char* out = strdup(sb.items);
    nob_sb_free(sb);
    return out;
}

static bool parse_bool_value(const char* v, bool current, bool* ok){
    *ok = true;
    if(!v) return !current;
    if(strcasecmp(v, "on") == 0 || strcasecmp(v, "true") == 0 || strcmp(v, "1") == 0 || strcasecmp(v, "yes") == 0) return true;
    if(strcasecmp(v, "off") == 0 || strcasecmp(v, "false") == 0 || strcmp(v, "0") == 0 || strcasecmp(v, "no") == 0) return false;
    *ok = false;
    return current;
}

void console_set_option(CCode* ccode, const char* args){
    CCodeConfig* c = ccode->config;
    const char* p = args;
    char* name = console_next_arg(&p);
    char* value = console_next_arg(&p);
    Layer* top = top_type_layer(ccode, LAYER_CODE);
    LayerCodeData* lcd = top ? top->layer_data : NULL;

    if(!name){
        message_to_console(ccode, nob_temp_sprintf(
            "tab_size=%d use_tabs=%s line_numbers=%s relative_line_numbers=%s auto_indent=%s auto_pair=%s "
            "show_whitespace=%s trim_trailing_whitespace=%s insert_final_newline=%s scroll_margin=%d crlf=%s",
            (int)c->tab_size, (lcd ? lcd->use_tabs : c->use_tabs) ? "on" : "off", c->line_numbers ? "on" : "off",
            c->relative_line_numbers ? "on" : "off", c->auto_indent ? "on" : "off", c->auto_pair ? "on" : "off",
            c->show_whitespace ? "on" : "off", c->trim_trailing_whitespace ? "on" : "off",
            c->insert_final_newline ? "on" : "off", c->scroll_margin, (lcd && lcd->crlf) ? "on" : "off"));
        nob_temp_reset();
        return;
    }

    struct { const char* name; bool* field; } bools[] = {
        {"line_numbers", &c->line_numbers}, {"relative_line_numbers", &c->relative_line_numbers},
        {"auto_indent", &c->auto_indent}, {"auto_pair", &c->auto_pair},
        {"show_whitespace", &c->show_whitespace}, {"trim_trailing_whitespace", &c->trim_trailing_whitespace},
        {"insert_final_newline", &c->insert_final_newline}, {"highlight_current_line", &c->highlight_current_line},
        {"highlight_matching_bracket", &c->highlight_matching_bracket},
        {"use_tabs", lcd ? &lcd->use_tabs : &c->use_tabs}, {"crlf", lcd ? &lcd->crlf : NULL},
    };
    bool handled = false;
    for(size_t i = 0; i < sizeof(bools)/sizeof(bools[0]); i++){
        if(strcmp(name, bools[i].name) != 0 || !bools[i].field) continue;
        bool ok;
        bool v = parse_bool_value(value, *bools[i].field, &ok);
        if(!ok){
            message_to_console(ccode, "Expected on/off");
        }else{
            *bools[i].field = v;
            // a different line ending is a change that has to be saved
            if(strcmp(name, "crlf") == 0 && lcd) lcd->saved = false;
            message_to_console(ccode, nob_temp_sprintf("%s = %s", name, v ? "on" : "off"));
            nob_temp_reset();
        }
        handled = true;
        break;
    }
    if(!handled && (strcmp(name, "tab_size") == 0 || strcmp(name, "scroll_margin") == 0)){
        int v = value ? atoi(value) : -1;
        if(strcmp(name, "tab_size") == 0){
            if(v < 1 || v > 16) message_to_console(ccode, "tab_size has to be between 1 and 16");
            else { c->tab_size = (size_t)v; message_to_console(ccode, nob_temp_sprintf("tab_size = %d", v)); }
        }else{
            if(v < 0) message_to_console(ccode, "scroll_margin has to be 0 or more");
            else { c->scroll_margin = v; message_to_console(ccode, nob_temp_sprintf("scroll_margin = %d", v)); }
        }
        nob_temp_reset();
        handled = true;
    }
    if(!handled){
        message_to_console(ccode, nob_temp_sprintf("Unknown option '%s', type :set to list options", name));
        nob_temp_reset();
    }
    free(name);
    free(value);
}

void open_help_buffer(CCode* ccode){
    Layer* help = new_layer_code();
    LayerCodeData* lcd = help->layer_data;
    arrsetlen(lcd->filename, 0);
    const char* name = "[help]";
    for(const char* p = name; *p; p++) arrput(lcd->filename, *p);
    arrput(lcd->filename, '\0');
    layer_code_make_welcome_screen(help);
    lcd->is_welcome = true;
    push_layer_to_top(ccode, help);
}

static void quit_confirmed_callback(CCode* ccode, void* data){
    (void) data;
    ccode->config->PrivateRunning = false;
}

static void close_confirmed_callback(CCode* ccode, void* data){
    (void) data;
    close_code_layer(ccode, true);
}

static void dialog_noop_callback(CCode* ccode, void* data){
    (void) ccode;
    (void) data;
}

static bool dialog_open(CCode* ccode){
    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_FLOATING_DIALOG) return true;
    }
    return false;
}

void console_execute_command(CCode* ccode, const char* buffer){
    TokenizationOutput to = {0};
    if(tokenize_console_command(buffer, &to)){
        fprintf(stderr, "Error while tokenizing console command\n");
        return;
    }
    if (arrlen(to.tokens) < 1 || to.tokens[0].type != TOKEN_COMMAND) {
        const char* b = buffer;
        while(*b && is_whitespace[(uint8_t)*b]) b++;
        if(*b){
            char name[64] = {0};
            int k = 0;
            while(b[k] && !is_whitespace[(uint8_t)b[k]] && k < 63){ name[k] = b[k]; k++; }
            message_to_console(ccode, nob_temp_sprintf("Unknown command '%s', :help lists all commands", name));
            nob_temp_reset();
        }
        goto defer;
    }

    switch (to.tokens[0].command_type) {
        case COMMAND_QUIT: {
            const char* first = NULL;
            int unsaved = count_unsaved(ccode, &first);
            if(unsaved > 0){
                if(!dialog_open(ccode)){
                    const char* q = unsaved == 1
                        ? nob_temp_sprintf("'%s' has unsaved changes. Quit anyway?", path_basename(first))
                        : nob_temp_sprintf("%d files have unsaved changes. Quit anyway?", unsaved);
                    push_layer_to_top(ccode, new_layer_floating_dialog(q, &quit_confirmed_callback, &dialog_noop_callback, NULL));
                    nob_temp_reset();
                }
                set_status(true, "Unsaved changes: :wa saves all, :q! quits without saving");
                break;
            }
            ccode->config->PrivateRunning = false;
            break;
        }

        case COMMAND_FORCE_QUIT: {
            ccode->config->PrivateRunning = false;
            break;
        }

        case COMMAND_WRITE_QUIT: {
            write_code_layer_to_file(ccode, false);
            const char* first = NULL;
            int unsaved = count_unsaved(ccode, &first);
            if(unsaved > 0){
                message_to_console(ccode, nob_temp_sprintf("'%s' is not saved, use :wa to save all or :q! to discard", first));
                nob_temp_reset();
                break;
            }
            ccode->config->PrivateRunning = false;
            break;
        }

        case COMMAND_WRITE_ALL: {
            Layer** all = collect_code_layers(ccode);
            int written = 0;
            Layer* top = top_layer(ccode);
            for(size_t i = 0; i < arrlenu(all); i++){
                LayerCodeData* lcd = all[i]->layer_data;
                if(lcd->saved || lcd->is_welcome) continue;
                write_layer_to_file(ccode, all[i], false);
                if(lcd->saved) written++;
            }
            if(top) push_layer_to_top(ccode, top);
            arrfree(all);
            message_to_console(ccode, nob_temp_sprintf("Saved %d file%s", written, written == 1 ? "" : "s"));
            nob_temp_reset();
            break;
        }

        case COMMAND_NEW: {
            Layer* layer = new_layer_code();
            push_layer_to_top(ccode, layer);
            break;
        }

        case COMMAND_HELP: {
            open_help_buffer(ccode);
            break;
        }

        case COMMAND_SET: {
            console_set_option(ccode, console_args_start(buffer));
            break;
        }

        case COMMAND_RELOAD: {
            Layer* top = top_type_layer(ccode, LAYER_CODE);
            if(!top) break;
            reload_code_layer(ccode, top);
            break;
        }

        case COMMAND_REPLACE: {
            Layer* top = top_type_layer(ccode, LAYER_CODE);
            if(!top){
                message_to_console(ccode, "No file to replace in");
                break;
            }
            const char* p = console_args_start(buffer);
            char* from = console_next_arg(&p);
            char* to_text = console_next_arg(&p);
            if(!from || !*from){
                message_to_console(ccode, "Usage: :r <find> <replace>");
            }else{
                LayerCodeData* lcd = top->layer_data;
                int count = layer_code_replace_all(lcd, from, to_text ? to_text : "");
                layer_code_flush_edit(ccode, top);
                message_to_console(ccode, nob_temp_sprintf("Replaced %d occurrence%s of '%s'", count, count == 1 ? "" : "s", from));
                nob_temp_reset();
            }
            free(from);
            free(to_text);
            break;
        }

        case COMMAND_OPEN: {
            if (has_token_types(to.tokens, TOKEN_STRING, NULL)) {
                read_file_to_code_layer(ccode, to.tokens[1].string.start, to.tokens[1].string.size, true);
            }
            break;
        }

        case COMMAND_GOTO: {
            if (has_token_types(to.tokens, TOKEN_INTEGER, NULL) && to.tokens[1].integer >= 0) {
                Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
                if(!top_code_layer) break;
                LayerCodeData* lcd = top_code_layer->layer_data;
                if (!lcd->cursor) break;

                // lines and columns are counted from 1 like in the status bar
                int64_t line = to.tokens[1].integer - 1;
                if(line < 0) line = 0;
                if(line > arrlen(lcd->code_buffer)-1) line = arrlen(lcd->code_buffer)-1;
                lcd->cursor->y = (int)line;
                lcd->cursor->x = 0;
                if (has_token_types(to.tokens, TOKEN_INTEGER, TOKEN_INTEGER, NULL) && to.tokens[2].integer >= 1) {
                    lcd->cursor->x = view_byte_at_col(lcd->code_buffer[line], (int)to.tokens[2].integer - 1);
                }else{
                    lcd->cursor->x = first_non_blank(lcd->code_buffer[line]);
                }
                lcd->preferred_col = -1;
                sel_clear(lcd);
                // center the target line
                VirtualWindow text = view_text_area(lcd);
                lcd->cursor->yoff = lcd->cursor->y - text.height / 2;
                if(lcd->cursor->yoff < 0) lcd->cursor->yoff = 0;
            }
            break;
        }

        case COMMAND_CHANGE_NAME: {
            if (has_token_types(to.tokens, TOKEN_STRING, NULL)) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
            }
            break;
        }

        case COMMAND_WRITE: {
            write_code_layer_to_file(ccode, false);
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
                    break;
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
            if (has_token_types(to.tokens, TOKEN_STRING, NULL)) {
                change_filename(ccode, to.tokens[1].string.start, to.tokens[1].string.size);
                write_code_layer_to_file(ccode, false);
            }
            break;
        }

        case COMMAND_FIND: {
            Layer* top_fs_layer = top_fullsceen_layer(ccode);
            if(top_fs_layer == NULL || (top_fs_layer->type != LAYER_CODE && top_fs_layer->type != LAYER_SPLIT_VIEW)){
                message_to_console(ccode, "cannot find text in this layer");
                break;
            }
            // the whole rest of the line is the search text, "quotes" are optional
            const char* raw = console_args_start(buffer);
            char* text = NULL;
            if(*raw == '"'){
                const char* p = raw;
                text = console_next_arg(&p);
            }else{
                text = strdup(raw);
                size_t l = strlen(text);
                while(l > 0 && is_whitespace[(uint8_t)text[l-1]]) text[--l] = '\0';
            }
            if(!text || !*text){
                free(text);
                message_to_console(ccode, "Usage: :f <text>");
                break;
            }
            Layer* code = top_type_layer(ccode, LAYER_CODE);
            if(code){
                LayerCodeData* lcd = code->layer_data;
                if(lcd->finding_substr != NULL){
                    free(lcd->finding_substr->substr);
                    free(lcd->finding_substr);
                    lcd->finding_substr = NULL;
                }
                // start at the first match after the cursor
                int nth = 1;
                bool ic = search_ignore_case(text);
                int count = 0;
                bool found_after = false;
                for(int r = 0; r < arrlen(lcd->code_buffer) && !found_after; r++){
                    char* line = lcd->code_buffer[r];
                    char* p = line;
                    while((p = find_case_strstr(p, text, ic)) != NULL){
                        count++;
                        int c = (int)(p - line);
                        if(r > lcd->cursor->y || (r == lcd->cursor->y && c >= lcd->cursor->x)){
                            nth = count;
                            found_after = true;
                            break;
                        }
                        p += strlen(text);
                    }
                }
                find_jump(ccode, text, (int32_t)strlen(text), nth);
            }
            free(text);
            break;
        }

        case COMMAND_CLOSE: {
            // the layer below the console (commands also run without it from shortcuts)
            Layer* second_layer = NULL;
            for(size_t li = 0; li < arrlenu(ccode->layers); li++){
                if(ccode->layers[li]->type != LAYER_CONSOLE){ second_layer = ccode->layers[li]; break; }
            }
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE){
                Layer* code = top_type_layer(ccode, LAYER_CODE);
                LayerCodeData* lcd = code ? code->layer_data : NULL;
                if(lcd && !lcd->saved){
                    if(!dialog_open(ccode)){
                        const char* q = nob_temp_sprintf("Discard unsaved changes to '%s'?", path_basename(lcd->filename));
                        push_layer_to_top(ccode, new_layer_floating_dialog(q, &close_confirmed_callback, &dialog_noop_callback, NULL));
                        nob_temp_reset();
                    }
                }else{
                    close_code_layer(ccode, false);
                }
            }else if(second_layer->type == LAYER_DIR_WALK || second_layer->type == LAYER_THEME_SELECTOR){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }else if(second_layer->type == LAYER_SPLIT_VIEW){
                layer_split_view_close(ccode, second_layer);
            }
            break;
        }

        case COMMAND_FORCE_CLOSE: {
            // the layer below the console (commands also run without it from shortcuts)
            Layer* second_layer = NULL;
            for(size_t li = 0; li < arrlenu(ccode->layers); li++){
                if(ccode->layers[li]->type != LAYER_CONSOLE){ second_layer = ccode->layers[li]; break; }
            }
            if(!second_layer){
                break;
            }

            if(second_layer->type == LAYER_CODE || second_layer->type == LAYER_SPLIT_VIEW){
                close_code_layer(ccode, true);
            }else if(second_layer->type == LAYER_DIR_WALK || second_layer->type == LAYER_THEME_SELECTOR){
                remove_layer(ccode, second_layer);
                free_layer(second_layer);
            }
            break;
        }

        case COMMAND_TREE: {
            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                char* dir_arr = NULL;

                if(has_token_types(to.tokens, TOKEN_STRING, NULL)){
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

                if(dir_arr){
                    nob_set_current_dir(dir_arr);
                    arrfree(dir_arr);
                }

                Layer* tree = new_layer_dir_walk();
                push_layer_to_top(ccode, tree);
            }else{
                char* dir_arr = NULL;

                if(has_token_types(to.tokens, TOKEN_STRING, NULL)){
                    char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                    dir_arr = str_to_arr(as_str);
                    free(as_str);
                }

                if(dir_arr == NULL){
                    const char* cwd = nob_get_current_dir_temp();
                    if(cwd == NULL){
                        fprintf(stderr, "Could not get cwd!\n");
                        break;
                    }
                    dir_arr = str_to_arr(cwd);
                    nob_temp_reset();
                }

                if(dir_arr != NULL){
                    nob_set_current_dir(dir_arr);
                    refresh_tree_files(top_tree_layer);
                    arrfree(dir_arr);
                }

                push_layer_to_top(ccode, top_tree_layer);
            }
            break;
        }

        case COMMAND_TREE_CHANGE_DIR: {
            if(arrlen(to.tokens) <= 1){
                break;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot change directory outside tree");
                break;
            }

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
                if(strncmp(as_str, "..", 2) == 0){
                    char* new_dir = nob_temp_sprintf("%s/%s", nob_get_current_dir_temp(), as_str);
                    char resolved_path[MAX_PATH];
                    if(resolve_path(new_dir, resolved_path) == NULL){
                        free(as_str);
                        nob_temp_reset();
                        break;
                    }
                    nob_set_current_dir(resolved_path);
                    refresh_tree_files(top_tree_layer);
                    nob_temp_reset();
                }else {
                    if(!nob_file_exists(as_str)){
                        message_to_console(ccode, "invalid path");
                    }
                    nob_set_current_dir(as_str);
                    refresh_tree_files(top_tree_layer);
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
            if(has_token_types(to.tokens, TOKEN_STRING, NULL)){
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

        case COMMAND_MAKE_DIRECTORY: {
            if(arrlen(to.tokens) <= 1){
                break;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot make directory outside tree");
                break;
            }

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);

                char* full_path = nob_temp_sprintf("%s/%s", nob_get_current_dir_temp(), as_str);

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
                refresh_tree_files(top_tree_layer);
            }
            break;
        }

        case COMMAND_MAKE_FILE: {
            if(arrlen(to.tokens) <= 1){
                break;
            }

            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot make file outside tree");
                break;
            }

            if(to.tokens[1].type == TOKEN_STRING){
                char* as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);

                char* full_path = nob_temp_sprintf("%s/%s", nob_get_current_dir_temp(), as_str);

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
                refresh_tree_files(top_tree_layer);
            }
            break;
        }

        case COMMAND_SPLIT_VIEW: {
            Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
        
            int layer_count = arrlen(code_layers);
            if (layer_count < 2) {
                message_to_console(ccode, "At least two code layers have to be opened!");
                goto sw_defer;
            }
        
            int win_a_index = 0;
            int win_b_index = 1;
        
            if (has_token_types(to.tokens, TOKEN_INTEGER, NULL)) {
                int idx = to.tokens[1].integer - 1;
                if (idx >= 0 && idx < layer_count) {
                    win_a_index = idx;
                }
            }
        
            if (has_token_types(to.tokens, TOKEN_INTEGER, TOKEN_INTEGER, NULL)) {
                int idx = to.tokens[2].integer - 1;
                if (idx >= 0 && idx < layer_count) {
                    win_b_index = idx;
                }
            }
        
            if (win_a_index == win_b_index) {
                message_to_console(ccode, "Cannot split view with the same layer twice!");
                goto sw_defer;
            }
        
            Layer* layer_a = code_layers[win_a_index];
            Layer* layer_b = code_layers[win_b_index];
        
            remove_layer(ccode, layer_a);
            remove_layer(ccode, layer_b);
        
            Layer* split_view = new_layer_split_view();
            if (!split_view || !make_split_view(split_view, layer_a, layer_b)) {
                if (split_view) free_layer(split_view);
                goto sw_defer;
            }
        
            push_layer_to_top(ccode, split_view);
        sw_defer:
            arrfree(code_layers);
            break;
        }
        
        case COMMAND_FLOATING_WINDOW: {
            Layer* ft = new_layer_floting_tree();
            push_layer_to_top(ccode, ft);
            break;
        }

        case COMMAND_DELETE_FILE: {
            Layer* top_tree_layer = top_type_layer(ccode, LAYER_DIR_WALK);
            if(top_tree_layer == NULL){
                message_to_console(ccode, "Cannot delete file outside tree");
                break;
            }
            LayerDirWalkData* ldwd = top_tree_layer->layer_data;
            const char* cwd = nob_get_current_dir_temp();
            char* as_str = NULL ;
            if(has_token_types(to.tokens, TOKEN_STRING, NULL)){
                as_str = stringview_to_str(to.tokens[1].string.start, to.tokens[1].string.size);
            }else{
                if(ldwd->current_dir_files == NULL || ldwd->selected == 0){
                    message_to_console(ccode, "File could not be find");
                    nob_temp_reset();
                    break;
                }
                as_str = strdup(ldwd->current_dir_files[ldwd->selected]);
            }
            if(as_str == NULL){
                message_to_console(ccode, "File could not be find");
                nob_temp_reset();
                break;
            }
            char question[4096] = {0};
            
            const char* dir_question  = "Remove directory '%s'?";
            const char* file_question = "Remove file '%s'?";
            
            Nob_File_Type ft = nob_get_file_type(nob_temp_sprintf("%s/%s", cwd, as_str));
            
            const char* fmt = (ft == NOB_FILE_DIRECTORY)
                ? dir_question
                : file_question;

            snprintf(question, sizeof(question), fmt, as_str);

            Layer* dlg = new_layer_floating_dialog(
                question,
                &file_remove_callback,
                &file_not_remove_callback,
                as_str
            );
            push_layer_to_top(ccode, dlg);

            nob_temp_reset();
            break;
        }

        case COMMAND_CONFIG: {
            char path[4096];
            char config_path[8096];

            get_config_directory(path, sizeof(path));
            snprintf(config_path, sizeof(config_path), "%s/config.json", path);
            read_file_to_code_layer(ccode, config_path, strlen(config_path), true);
            break;
        }

        case COMMAND_DIAGNOSTICS: {
            Layer* first_code_layer = NULL;
            for(int i = arrlen(ccode->layers) - 1; i >= 0; i--){
                if(ccode->layers[i]->type == LAYER_CODE){
                    first_code_layer = ccode->layers[i];
                }else if(ccode->layers[i]->type == LAYER_SPLIT_VIEW){
                    LayerSplitViewData* lsvd = ccode->layers[i]->layer_data;
                    first_code_layer = lsvd->splitten_layers[lsvd->focused];
                    // Always true for now (planned feature to make other layers splitable)
                    assert(first_code_layer->type == LAYER_CODE);
                }
            }
            if(!first_code_layer){
                break;
            }
            LayerCodeData* lcd = first_code_layer->layer_data;
            lcd->allow_diagnostics = !lcd->allow_diagnostics;

            break;
        }

        case COMMAND_MOVE_MODE: {
            Layer* top_sv_layer = top_type_layer(ccode, LAYER_SPLIT_VIEW);
            if(!top_sv_layer) break;
            LayerSplitViewData* lsvd = top_sv_layer->layer_data;
            lsvd->move_mode = !lsvd->move_mode;
            break;
        } 

        default: {
            break;
        }
    }

defer:
    arrfree(to.tokens);
    free(to.string_storage);
}


CommandMap* layer_console_get_type_hint(Layer* layer){
    CommandMap* match = NULL;

    LayerConsoleData* console_data = layer->layer_data;

    for(size_t i = 0; i < shlen(commands); i++){
        CommandMap t = commands[i];
        size_t buff_len = arrlen(console_data->console_buffer);
        
        if (buff_len == 0) {
            continue;
        }

        if(strncmp(t.key, console_data->console_buffer, buff_len-1) == 0){
            match = &commands[i];
            break;
        }
    }
    return match;
}


bool layer_console_has_params(Layer* layer){
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;

    bool has_params = false;
    for(int i = arrlen(console_data->console_buffer) - 2; i >= 0; i--){
        if(console_data->console_buffer[i] == ' '){
            has_params = true;
            break;
        }
    }
    return has_params;
}

void layer_console_draw_type_hint(CCode* ccode, Layer* layer){
    (void) ccode;
    //START_PROFILING();
    int y, x;
    getmaxyx(stdscr, y, x);
    (void) x;

    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;

    int target = y-1;
    if(arrlen(console_data->console_buffer) <= 1) return;

    bool has_params = layer_console_has_params(layer);
    if(has_params) return;


    CommandMap* match = layer_console_get_type_hint(layer);
    if(!match) return;
    

    attron(A_REVERSE | COLOR_PAIR(6));
    for(size_t i = arrlenu(console_data->console_buffer)-1; i < strlen(match->key); i++){
        mvaddch(target, i, match->key[i]);
    }
    attroff(A_REVERSE | COLOR_PAIR(6));
    //END_PROFILING("console_type_hinting");
    //prof_pop_print_ms();
}

bool layer_console_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return false;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;

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

        console_data->console_buffer_x++;
    } else if(chr == KEY_PASTE && pasted_text){
        // pasted text goes in at the cursor, line breaks become spaces
        for(size_t i = 0; i + 1 < arrlenu(pasted_text); i++){
            char c = pasted_text[i] == '\n' || pasted_text[i] == '\t' ? ' ' : pasted_text[i];
            if(!isprint((unsigned char)c) && (unsigned char)c < 0x80) continue;
            (void) arrpop(console_data->console_buffer);
            arrins(console_data->console_buffer, console_data->console_buffer_x, c);
            arrput(console_data->console_buffer, '\0');
            console_data->console_buffer_x++;
        }
    } else if(chr == KEY_HOME){
        console_data->console_buffer_x = 0;
    } else if(chr == KEY_END){
        console_data->console_buffer_x = (int)strlen(console_data->console_buffer);
    } else if(chr == KEY_DC){
        int len = (int)strlen(console_data->console_buffer);
        if(console_data->console_buffer_x < len){
            arrdel(console_data->console_buffer, console_data->console_buffer_x);
        }
    } else if(chr == CUSTOM_KEY_ENTER){
        ccode->config->PrivateCloseConsole = true;
        console_execute_command(ccode, console_data->console_buffer);
        char* arr_copy = str_to_arr(console_data->console_buffer);
        
        arrput(console_history, arr_copy);
        console_history_index = arrlen(console_history);
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
    } else if(chr == KEY_UP) {
        if(!console_history || arrlen(console_history) == 0)
            return true;
    
        if(!console_history_navigating){
            console_history_navigating = true;
            if(console_history_draft){
                arrfree(console_history_draft);
            }
            console_history_draft = str_to_arr(console_data->console_buffer);
            console_history_index = arrlen(console_history);
        }
    
        if(console_history_index > 0){
            console_history_index--;
        }
    
        arrfree(console_data->console_buffer);
        console_data->console_buffer = str_to_arr(console_history[console_history_index]);
        console_data->console_buffer_x = strlen(console_data->console_buffer);
    } else if(chr == KEY_DOWN) {
        if(!console_history_navigating)
            return true;
    
        if(console_history_index < arrlen(console_history) - 1){
            console_history_index++;
    
            arrfree(console_data->console_buffer);
            console_data->console_buffer = str_to_arr(console_history[console_history_index]);
        } else {
            // restore draft
            console_history_index = arrlen(console_history);
    
            arrfree(console_data->console_buffer);
            console_data->console_buffer = str_to_arr(console_history_draft);
    
            console_history_navigating = false;
    
            arrfree(console_history_draft);
            console_history_draft = NULL;
        }
    
        console_data->console_buffer_x = strlen(console_data->console_buffer);
    }else if(chr == CUSTOM_KEY_TAB){
        // typehint autocomplete
        if(arrlen(console_data->console_buffer) <= 1) return true;

        bool has_params = layer_console_has_params(layer);
        if(has_params) return true;

        CommandMap* match = layer_console_get_type_hint(layer);
        if(!match) return true;

        int line_len = arrlen(console_data->console_buffer);

        if(console_data->console_buffer_x > line_len - 1){
            console_data->console_buffer_x = line_len - 1;
        }

        if(line_len > 0){
            (void) arrpop(console_data->console_buffer);
        }

        if(console_data->console_buffer_x != arrlenu(console_data->console_buffer)){
            arrput(console_data->console_buffer, '\0');
            return true;
        }

        for(size_t i = arrlenu(console_data->console_buffer); i < strlen(match->key); i++){
            arrput(console_data->console_buffer, match->key[i]);
            console_data->console_buffer_x++;
        }
        arrput(console_data->console_buffer, '\0');
    }

    return true;
}


// First buffer column shown on screen
int console_scroll_offset(LayerConsoleData* console_data, int width){
    int off = console_data->console_buffer_x - (width - 1);
    return off > 0 ? off : 0;
}

void layer_console_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_CONSOLE || layer->layer_data == NULL){
        return;
    }
    LayerConsoleData* console_data = (LayerConsoleData*) layer->layer_data;
    int y, x;
    getmaxyx(stdscr, y, x);

    // long commands scroll so the cursor stays visible
    int off = console_scroll_offset(console_data, x);
    const char* visible = console_data->console_buffer + off;

    attron(A_REVERSE);
    mvhline(y-1, 0, ' ', x);
    mvaddnstr(y-1, 0, visible, x);
    attroff(A_REVERSE);
    if(off == 0) layer_console_draw_type_hint(ccode, layer);
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