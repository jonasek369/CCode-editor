#ifndef _H_LAYER_CODE
#define _H_LAYER_CODE

// Supported parsers
extern const TSLanguage *tree_sitter_c();
extern const TSLanguage *tree_sitter_json();
extern const TSLanguage *tree_sitter_python();
extern const TSLanguage *tree_sitter_c_sharp();

#define TS_REPARSE(code_data)                                                        \
    do {                                                                             \
        if((code_data)->parser && (code_data)->tree){                                \
            char *src = flatten_buffer(code_data);                                   \
            TSTree *new_tree = ts_parser_parse_string(                               \
                (code_data)->parser, (code_data)->tree, src, strlen(src));           \
            free(src);                                                               \
            ts_tree_delete((code_data)->tree);                                       \
            (code_data)->tree = new_tree;                                            \
        }                                                                            \
    } while(0)


void layer_code_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);

Layer* new_layer_code(){
    Layer* code = malloc(sizeof(Layer));
    if(!code){
        return NULL;
    }
    code->type = LAYER_CODE;
    code->consume_input = true;
    code->draws_fullscreen = true;
    code->handle_keypress_function = &layer_code_handle_keypress;

    LayerCodeData* lcd = malloc(sizeof(LayerCodeData));
    if(!lcd){
        free(code);
        return NULL;
    }
    lcd->cursor = malloc(sizeof(Cursor));
    if(!lcd->cursor){
        free(code);
        free(lcd);
        return NULL;
    }
    lcd->cursor->x = 0;
    lcd->cursor->y = 0;
    lcd->cursor->xoff = 0;
    lcd->cursor->yoff = 0;

    lcd->finding_substr = NULL;

    lcd->filename = NULL;
    for(size_t i = 0; i < default_filename_length; i++){
        arrput(lcd->filename, default_filename[i]);
    }
    arrput(lcd->filename, '\0');
    lcd->code_buffer = NULL;
    lcd->saved = false;
    code->layer_data = lcd;

    // Tree-sitter
    lcd->parser = NULL;
    lcd->tree = NULL;
    lcd->lang = LANG_UNKNOWN;

    // LSP
    lcd->uri = NULL;
    lcd->version = 1;
    lcd->id = random_id();
    lcd->diagnostics = NULL;
    lcd->ranges = NULL;
    lcd->completion_window = NULL;

    return code;
}


SyntaxLanguage get_language_from_suffix(const char* filename) {
    size_t size = strlen(filename);
    size_t i = size;

    while (i > 0 && filename[i] != '.') {
        i--;
    }
    if (i == 0) return LANG_UNKNOWN; // no suffix

    i++; // skip the dot
    const char* suffix = filename + i;
    size_t suffix_len = size - i;

    if (suffix_len == 1 && (strncmp(suffix, "c", 1) == 0 || strncmp(suffix, "h", 1) == 0)) {
        return LANG_C;
    } else if (suffix_len == 4 && strncmp(suffix, "json", 4) == 0) {
        return LANG_JSON;
    } else if (suffix_len == 2 && strncmp(suffix, "py", 2) == 0) {
        return LANG_PYTHON;
    } else if (suffix_len == 2 && strncmp(suffix, "cs", 2) == 0) {
        return LANG_C_SHARP;
    } else if(suffix_len == 2 && strncmp(suffix, "rs", 2) == 0){
        return LANG_RUST;
    }
    return LANG_UNKNOWN;
}

const TSLanguage* get_filetype_language_parser(const char* filename, SyntaxLanguage* lang) {
    *lang = get_language_from_suffix(filename);

    switch (*lang) {
        case LANG_C:
            return tree_sitter_c();
        case LANG_JSON:
            return tree_sitter_json();
        case LANG_PYTHON:
            return tree_sitter_python();
        case LANG_C_SHARP:
            return tree_sitter_c_sharp();
        default:
            return NULL;
    }
}


void make_parser(CCode* ccode, char* filename){
    Layer* layer = top_type_layer(ccode, LAYER_CODE);
    if(!layer){
        return;
    }
    LayerCodeData* lcd = layer->layer_data;
    const TSLanguage* parser = get_filetype_language_parser(filename, &(lcd->lang));
    if(!parser){
        return;
    }
    lcd->parser = ts_parser_new();
    if(!ts_parser_set_language(lcd->parser, parser)){
        ts_parser_delete(lcd->parser);
        lcd->parser = NULL;
        fprintf(stderr, "Language version mismatch\n");
        return;
    }

    char* flatten = flatten_buffer(lcd);
    lcd->tree = ts_parser_parse_string(
        lcd->parser,
        NULL,
        flatten,
        strlen(flatten)
    );

    free(flatten);
}


void read_file_to_code_layer(CCode* ccode, const char* filename_start, size_t size){
    Nob_String_Builder sb = {0};
    Nob_String_Builder filename = {0};
    nob_da_append_many(&filename, filename_start, size);
    nob_da_append(&filename, '\0');

    if(!can_read_file(filename.items)){
        Layer* new_layer = new_layer_console();
        push_layer_to_top(ccode, new_layer);

        char message[1024];
        snprintf(message, 1024, "cannot read '%s'", filename.items);

        message_to_console(ccode, message);
        nob_sb_free(sb);
        return;
    }

    if(!nob_read_entire_file(filename.items, &sb)){
        fprintf(stderr, "Error reading file\n");
        nob_sb_free(sb);
    }
    Layer* file_layer = new_layer_code();
    LayerCodeData* lcd = (LayerCodeData*) file_layer->layer_data;
    size_t zero = 0;
    arrsetlen(lcd->filename, zero);
    nob_da_foreach(char, c, &filename){
        arrput(lcd->filename, *c);
    }
    if(!lcd->uri){
        char path[MAX_PATH] = {0};
        resolve_path(lcd->filename, path);
        lcd->uri = get_file_uri(path);
    }

    char* line = NULL;
    nob_da_foreach(char, c, &sb){
        if(*c == '\t'){
            arrput(line, ' ');
            arrput(line, ' ');
            arrput(line, ' ');
            arrput(line, ' ');
        }else if(*c == '\n'){
            arrput(line, '\n');
            arrput(line, '\0');
            arrput(lcd->code_buffer, line);
            line = NULL;
        }else if(*c == '\r'){
            continue;
        }
        else{
            arrput(line, *c);
        }
    }
    if (line != NULL) {
        arrput(line, '\0');
        arrput(lcd->code_buffer, line);
    }
    lcd->saved = true;
    push_layer_to_top(ccode, file_layer);
    make_parser(ccode, filename.items);
    LSPKind lsp_kind = lang_to_lspkind[lcd->lang];
    if(lsp_kind != LSPKIND_UNKNOWN && is_lspkind_installed(lsp_kind) && is_lspkind_running(ccode, lsp_kind) == false){
        LSPContext* ctx = start_lsp(lsp_kind, make_ccode_initialize_params(lcd->uri));
        arrput(ccode->lsp_ctxs, ctx);
        printf("Started LSP %s\n", lsp_kind_cstr[lsp_kind]);
    }
    if(is_lspkind_running(ccode, lsp_kind)){
        LSPContext* ctx = get_running_lsp(ccode, lsp_kind);
        char* flatten = flatten_buffer(lcd);
        char* escaped = json_escape(flatten);
        JsonValue* params = make_didOpen_params(lcd->uri, lsp_to_cstr_lang[lsp_kind], 1, escaped);
        JsonValue* message = make_didOpen_notification(params);
        tiny_queue_push(ctx->sender_queue, message);
        free(flatten);
        free(escaped);
    }
    nob_sb_free(sb);
    nob_sb_free(filename);
}


void send_to_lsp(CCode* ccode, LSPContext* ctx){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    if(top_code_layer == NULL){
        return;
    }
    LayerCodeData* lcd = (LayerCodeData*) top_code_layer->layer_data;

    char* flatten = flatten_buffer(lcd);
    char* escaped = json_escape(flatten);

    JsonValue* params = make_didChange_params(lcd->uri, ++(lcd->version), escaped);
    JsonValue* message = make_didChange_notification(params);
    tiny_queue_push(ctx->sender_queue, message);
    free(flatten);
    free(escaped);
}


void get_completion(CCode* ccode, LSPContext* ctx){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    if(top_code_layer == NULL){
        return;
    }
    LayerCodeData* lcd = (LayerCodeData*) top_code_layer->layer_data;
    Cursor* cursor = lcd->cursor;

    unsigned int line = cursor->y;
    unsigned int character = cursor->x;

    JsonValue* params = make_completion_params(lcd->uri, line, character);
    JsonValue* message = make_completion_request(json_new_string("completion"), params);
    tiny_queue_push(ctx->sender_queue, message);
}


void change_filename(CCode* ccode, const char* new_filename, size_t size){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    if(top_code_layer == NULL){
        return;
    }

    LayerCodeData* lcd = (LayerCodeData*)top_code_layer->layer_data;
    size_t zero = 0;
    arrsetlen(lcd->filename, zero);
    lcd->filename = NULL;
    for(size_t i = 0; i < size; i++){
        arrput(lcd->filename, new_filename[i]);
    }
    arrput(lcd->filename, '\0');
    make_parser(ccode, lcd->filename);
}


void write_code_layer_to_file(CCode* ccode){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    if(top_code_layer == NULL){
        return;
    }
    LayerCodeData* lcd = (LayerCodeData*) top_code_layer->layer_data;

    Nob_String_Builder sb = {0};
    for(int line_n = 0; line_n < arrlen(lcd->code_buffer); line_n++){
        char* line = lcd->code_buffer[line_n];
        nob_sb_append_cstr(&sb, line);
    }

    if(nob_write_entire_file(lcd->filename, sb.items, sb.count)){
        lcd->saved = true;
    }

    if(is_lspkind_running(ccode, lang_to_lspkind[lcd->lang])){
        send_to_lsp(ccode, get_running_lsp(ccode, lang_to_lspkind[lcd->lang]));
    }

    nob_sb_free(sb);
}


bool do_completion(CCode* ccode){
    Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
    if(top_code_layer == NULL){
        return false;
    }
    LayerCodeData* lcd = (LayerCodeData*) top_code_layer->layer_data;
    if(!lcd->completion_window){
        return false;
    }
    if(!lcd->completion_window->completion || lcd->completion_window->completion->type == JSON_NULL){
        return false;
    }
    JsonValue* result = shget(lcd->completion_window->completion->object, "result");
    JsonValue* items = shget(result->object, "items");
    if(items == NULL || items->type != JSON_ARRAY){
        return false;
    }
    if(lcd->completion_window->selected >= arrlen(items->array)){
        json_free(lcd->completion_window->completion);
        free(lcd->completion_window);
        lcd->completion_window = NULL;
        return false;
    }
    JsonValue* to_add = items->array[lcd->completion_window->selected];
    if(to_add == NULL || to_add->type != JSON_OBJECT){
        return false;
    }
    JsonValue* text_edit = shget(to_add->object, "textEdit");
    if(text_edit == NULL || text_edit->type != JSON_OBJECT){
        return false;
    }
    LSPRange* range = get_range(text_edit);
    JsonValue* new_text = shget(text_edit->object, "newText");
    int row = lcd->cursor->y;
    char* line = lcd->code_buffer[row];
    int line_len = arrlen(line);

    if(lcd->cursor->x > line_len - 1){
        lcd->cursor->x = line_len - 1;
    }

    (void)arrpop(line); // \0
    (void)arrpop(line); // \n

    size_t start   = range->start_character;
    size_t end     = range->end_character;
    size_t new_len = strlen(new_text->string);
    int    cur_len = arrlen(line);

    if((int)end > cur_len) end = (size_t)cur_len;
    if((int)start > cur_len) start = (size_t)cur_len;

    char* new_line = NULL;

    for(size_t k = 0; k < start; k++){
        arrput(new_line, line[k]);
    }
    for(size_t k = 0; k < new_len; k++){
        arrput(new_line, new_text->string[k]);
    }
    for(int k = (int)end; k < cur_len; k++){
        arrput(new_line, line[k]);
    }
    arrput(new_line, '\n');
    arrput(new_line, '\0');

    arrfree(line);
    lcd->code_buffer[row] = new_line;
    line = new_line;

    lcd->cursor->x = (int)(start + new_len);

    if(lcd->parser && lcd->tree){
        size_t start_col   = range->start_character;
        size_t old_end_col = end;
        size_t new_end_col = start + new_len;

        TSInputEdit edit = {
            .start_byte    = buffer_byte_offset(lcd, row, start_col),
            .old_end_byte  = buffer_byte_offset(lcd, row, old_end_col),
            .new_end_byte  = buffer_byte_offset(lcd, row, new_end_col),
            .start_point   = {row, start_col},
            .old_end_point = {row, old_end_col},
            .new_end_point = {row, new_end_col},
        };
        ts_tree_edit(lcd->tree, &edit);
    }
    TS_REPARSE(lcd);

    free(range);

    json_free(lcd->completion_window->completion);
    free(lcd->completion_window);
    lcd->completion_window = NULL;

    if(is_lspkind_running(ccode, lang_to_lspkind[lcd->lang])){
        send_to_lsp(ccode, get_running_lsp(ccode, lang_to_lspkind[lcd->lang]));
    }

    return true;
}


void close_code_layer(CCode* ccode, bool forced){
    Layer* to_close = top_type_layer(ccode, LAYER_CODE);
    if(!to_close){
        return;
    }
    LayerCodeData* lcd = to_close->layer_data;
    if(lcd->saved == false && forced == false){
        message_to_console(ccode, "Cannot close unsaved file save it or use ':c!' to force close");
        return;
    }else{
        LSPKind kind = lang_to_lspkind[lcd->lang];
        if(is_lspkind_running(ccode, kind)){
            size_t same_langs = 0;
            Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
            for(size_t i = 0; i < arrlenu(code_layers); i++){
                if(to_close == code_layers[i]) continue;
                if(lcd->lang == ((LayerCodeData*)code_layers[i]->layer_data)->lang) same_langs++;
            }
            if(same_langs == 0){
                size_t at_index = 0;
                for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
                    if(ccode->lsp_ctxs[i]->kind == kind){
                        at_index = i;
                        break;
                    }
                }
                destroy_lsp(ccode->lsp_ctxs[at_index]);
                arrdel(ccode->lsp_ctxs, at_index);
            }
            arrfree(code_layers);
        }
        free_layer(to_close);
        remove_layer(ccode, to_close);
    }
}


void find_jump(CCode* ccode, char *finding, int32_t size, int32_t nth_occurence) {
    if (!ccode || !finding || size <= 0 || nth_occurence <= 0) {
        return;
    }
    Layer* layer = top_type_layer(ccode, LAYER_CODE);
    if (!layer) return;
    LayerCodeData* lcd = (LayerCodeData*) layer->layer_data;
    if (!lcd || !lcd->cursor) return;

    int y, x;
    getmaxyx(stdscr, y, x);
    (void)x;

    char* substr = malloc(size+1);
    memcpy(substr, finding, size);
    substr[size] = '\0';

    if(lcd->finding_substr == NULL){
        FindingSubstr* fss = malloc(sizeof(FindingSubstr));
        fss->substr = substr;
        fss->at_nth_occurence = nth_occurence;
        lcd->finding_substr = fss;
    }

    int32_t first_x = 0;
    int32_t first_y = 0;

    int32_t found = 0;
    for(int32_t nline = 0; nline < arrlen(lcd->code_buffer); nline++){
        char* line = lcd->code_buffer[nline];
        char* pos = line;
        while ((pos = strstr(pos, substr)) != NULL) {
            found++;
            if(found == 1){
                first_x = (pos-line);
                first_y = nline;
            }
            if (found == nth_occurence) {
                lcd->cursor->x = (pos - line);
                lcd->cursor->y = nline;

                lcd->cursor->yoff = lcd->cursor->y - y / 2;
                if (lcd->cursor->yoff < 0) lcd->cursor->yoff = 0;

                return;
            }
            pos++;
        }
    }
    if(found < nth_occurence){
        if(lcd->finding_substr != NULL){
            lcd->finding_substr->at_nth_occurence = 1;
        }
        lcd->cursor->x = first_x;
        lcd->cursor->y = first_y;
    }

    if(found == 0){
        char message[128];
        char* str = stringview_to_str(finding, size);
        snprintf(message, sizeof(message), "Could not find '%s'", str);
        message_to_console(ccode, message);

        free(str);

        free(lcd->finding_substr->substr);
        free(lcd->finding_substr);
        lcd->finding_substr = NULL;
    }
}


void layer_code_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL){
        return;
    }
    START_PROFILING();
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;

    bool inFindSubstrMode = code_data->finding_substr != NULL;

    if(code_data->code_buffer == NULL){
        char* line = NULL;
        arrput(line, '\0');
        arrput(code_data->code_buffer, line);
    }

    if(arrlen(code_data->code_buffer) <= code_data->cursor->y){
        int add_n = code_data->cursor->y - arrlen(code_data->code_buffer) + 1;
        for(int i = 0; i < add_n; i++){
            char* line = NULL;
            arrput(line, '\n');
            arrput(line, '\0');
            arrput(code_data->code_buffer, line);
        }
    }

    if(code_data->cursor->y >= arrlen(code_data->code_buffer)){
        return;
    }

    if(chr == CUSTOM_CTL_F){
        Layer* console_in_layers = top_type_layer(ccode, LAYER_CONSOLE);
        if(console_in_layers == NULL){
            Layer* new_console = new_layer_console();
            push_layer_to_top(ccode, new_console);
            LayerConsoleData* console_data = new_console->layer_data;
            size_t zero = 0;
            arrsetlen(console_data->console_buffer, zero);
            arrput(console_data->console_buffer, ':');
            arrput(console_data->console_buffer, 'f');
            arrput(console_data->console_buffer, ' ');
            arrput(console_data->console_buffer, '\0');
            console_data->console_buffer_x = 3;
        }
    }

    if(chr == CUSTOM_CTL_S){
        write_code_layer_to_file(ccode);
    }

    if(chr == 9){
        if(code_data->completion_window){
            if(do_completion(ccode)){
                chr = -1;
            }
        }else{
            for(size_t i = 0; i < ccode->config->tab_size; i++){
                layer_code_update(ccode, layer, ' ');
            }
        }
    }

    int y, x;
    getmaxyx(stdscr, y, x);

    if(inFindSubstrMode){
        FindingSubstr* fss = code_data->finding_substr;
        bool change = false;
        if(chr == KEY_RIGHT){
            fss->at_nth_occurence++;
            change = true;
            chr = -1;
        }else if(chr == KEY_LEFT){
            if(fss->at_nth_occurence - 1 >= 1){
                fss->at_nth_occurence--;
                change = true;
            }
            chr = -1;
        }
        if(change){
            find_jump(ccode, fss->substr, strlen(fss->substr), fss->at_nth_occurence);
        }
        if(chr == CUSTOM_KEY_ENTER){
            free(fss->substr);
            free(fss);
            code_data->finding_substr = NULL;
            chr = -1;
        }
    }

    if((chr >= 0 && chr <= 255) && isprint(chr)){
        char* line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(line);

        if(code_data->cursor->x > line_len - 1){
            code_data->cursor->x = line_len - 1;
        }

        if(line_len > 0){
            (void) arrpop(line);
        }

        if(code_data->cursor->x >= 0 && code_data->cursor->x <= arrlen(line)){
            arrins(line, code_data->cursor->x, (char)chr);
        } else {
            arrput(line, (char)chr);
        }
        code_data->saved = false;
        arrput(line, '\0');
        code_data->code_buffer[code_data->cursor->y] = line;

        int row = code_data->cursor->y;
        int col = code_data->cursor->x;

        if(code_data->parser && code_data->tree){
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, col),
                .old_end_byte  = buffer_byte_offset(code_data, row, col),
                .new_end_byte  = buffer_byte_offset(code_data, row, col + 1),
                .start_point   = {row, col},
                .old_end_point = {row, col},
                .new_end_point = {row, col + 1},
            };
            ts_tree_edit(code_data->tree, &edit);
        }
        TS_REPARSE(code_data);

        code_data->cursor->x++;
    }

    else if(chr == CUSTOM_KEY_BACKSPACE){
        char* line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(line);

        bool do_parse = false;

        if(code_data->cursor->x > 0 && line_len > 1){
            int row = code_data->cursor->y;
            int col = code_data->cursor->x;

            if(code_data->parser && code_data->tree){
                TSInputEdit edit = {
                    .start_byte    = buffer_byte_offset(code_data, row, col - 1),
                    .old_end_byte  = buffer_byte_offset(code_data, row, col),
                    .new_end_byte  = buffer_byte_offset(code_data, row, col - 1),
                    .start_point   = {row, col - 1},
                    .old_end_point = {row, col},
                    .new_end_point = {row, col - 1},
                };
                ts_tree_edit(code_data->tree, &edit);
            }
            do_parse = true;

            (void)arrpop(line);
            arrdel(line, code_data->cursor->x - 1);
            arrput(line, '\0');
            code_data->cursor->x--;
            code_data->code_buffer[code_data->cursor->y] = line;
        }
        else if(code_data->cursor->x == 0 && code_data->cursor->y > 0){
            char* prev_line = code_data->code_buffer[code_data->cursor->y - 1];
            int prev_line_len = arrlen(prev_line);
            int cur_row = code_data->cursor->y;

            int del_row = cur_row - 1;
            int del_col = (prev_line_len >= 2) ? prev_line_len - 2 : 0;

            if(code_data->parser && code_data->tree){
                TSInputEdit edit = {
                    .start_byte    = buffer_byte_offset(code_data, del_row, del_col),
                    .old_end_byte  = buffer_byte_offset(code_data, cur_row, 0),
                    .new_end_byte  = buffer_byte_offset(code_data, del_row, del_col),
                    .start_point   = {del_row, del_col},
                    .old_end_point = {cur_row, 0},
                    .new_end_point = {del_row, del_col},
                };
                ts_tree_edit(code_data->tree, &edit);
            }
            do_parse = true;

            int prev_len = arrlen(prev_line);
            code_data->cursor->x = (prev_len >= 2) ? prev_len - 2 : 0;

            if(prev_len >= 2){
                (void) arrpop(prev_line); // '\0'
                (void) arrpop(prev_line); // '\n'
            }

            char* current_line = code_data->code_buffer[code_data->cursor->y];
            int current_line_len = arrlen(current_line);
            for(int i = 0; i < current_line_len - 2; i++){
                arrput(prev_line, current_line[i]);
            }
            arrput(prev_line, '\n');
            arrput(prev_line, '\0');

            code_data->code_buffer[code_data->cursor->y - 1] = prev_line;
            arrfree(current_line);
            arrdel(code_data->code_buffer, code_data->cursor->y);
            code_data->cursor->y--;
        }

        if(do_parse){
            TS_REPARSE(code_data);
        }

        code_data->saved = false;
    }

    else if(chr == CUSTOM_KEY_ENTER && code_data->completion_window){
        if(do_completion(ccode)){
            chr = -1;
        }
    }
    else if(chr == CUSTOM_KEY_ENTER && !code_data->completion_window){
        char* current_line = code_data->code_buffer[code_data->cursor->y];
        int line_len = arrlen(current_line);

        if(code_data->cursor->x < 0) code_data->cursor->x = 0;
        if(code_data->cursor->x > line_len) code_data->cursor->x = line_len;

        int row = code_data->cursor->y;
        int col = code_data->cursor->x;

        if(code_data->parser && code_data->tree){
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, col),
                .old_end_byte  = buffer_byte_offset(code_data, row, col),
                .new_end_byte  = buffer_byte_offset(code_data, row, col) + 1,
                .start_point   = {row, col},
                .old_end_point = {row, col},
                .new_end_point = {row + 1, 0},
            };
            ts_tree_edit(code_data->tree, &edit);
        }

        char* new_line = NULL;
        if(line_len > 0 && code_data->cursor->x < line_len - 2){
            for(int i = code_data->cursor->x; i < line_len - 2; i++){
                arrput(new_line, current_line[i]);
            }
        }
        arrput(new_line, '\n');
        arrput(new_line, '\0');

        if(line_len > 0){
            (void) arrpop(current_line);
            while(arrlen(current_line) > code_data->cursor->x){
                (void) arrpop(current_line);
            }
        }
        arrput(current_line, '\n');
        arrput(current_line, '\0');

        code_data->code_buffer[code_data->cursor->y] = current_line;
        arrins(code_data->code_buffer, code_data->cursor->y + 1, new_line);

        code_data->cursor->y++;
        code_data->cursor->x = 0;
        code_data->saved = false;

        TS_REPARSE(code_data);
    }

    else if(!inFindSubstrMode && (chr == KEY_DOWN || chr == KEY_UP) && code_data->completion_window){
        if(chr == KEY_DOWN && code_data->completion_window->selected < code_data->completion_window->items_count){
            code_data->completion_window->selected++;
            chr = -1;
        }
        if(chr == KEY_UP && code_data->completion_window->selected > 0){
            code_data->completion_window->selected--;
            chr = -1;
        }
    }
    else if(!inFindSubstrMode && chr >= KEY_DOWN && chr <= KEY_RIGHT){
        switch(chr){
            case KEY_DOWN: {
                if(arrlen(code_data->code_buffer) <= code_data->cursor->y+1){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y+1];
                int length = arrlen(new_line)-2;
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length >= 0 ? length : 0;
                }
                code_data->cursor->y++;
                break;
            }
            case KEY_UP: {
                if(code_data->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y-1];
                int length = arrlen(new_line)-2;
                if(length < code_data->cursor->x){
                    code_data->cursor->x = length >= 0 ? length : 0;
                }
                code_data->cursor->y--;
                break;
            }
            case KEY_LEFT: {
                if(code_data->cursor->x != 0){
                    code_data->cursor->x--;
                    break;
                }
                if(code_data->cursor->y == 0){
                    break;
                }
                char* new_line = code_data->code_buffer[code_data->cursor->y-1];
                int length = arrlen(new_line)-2;
                code_data->cursor->x = length >= 0 ? length : 0;
                code_data->cursor->y--;
                break;
            }
            case KEY_RIGHT: {
                char* current_line = code_data->code_buffer[code_data->cursor->y];
                if(code_data->cursor->x < arrlen(current_line)-2){
                    code_data->cursor->x++;
                    break;
                }
                if(arrlen(code_data->code_buffer) <= code_data->cursor->y+1){
                    break;
                }
                code_data->cursor->y++;
                code_data->cursor->x = 0;
                break;
            }
        }
    }
    else if(!inFindSubstrMode && chr == CTL_PGUP){
        code_data->cursor->y -= (y-2)*2;
        if(code_data->cursor->y < 0){
            code_data->cursor->y = 0;
        }
    }
    else if(!inFindSubstrMode && chr == CTL_PGDN){
        code_data->cursor->y += (y-2)*2;
    }
    else if (!inFindSubstrMode && chr == CTL_UP) {
        if (code_data->cursor->yoff > 0) {
            code_data->cursor->yoff--;
        }
    }
    else if (!inFindSubstrMode && chr == CTL_DOWN) {
        int screen_rows = y-1;
        int buffer_size = arrlen(code_data->code_buffer);
        int max_scroll = buffer_size - screen_rows;
        if (max_scroll < 0) max_scroll = 0;
        if (code_data->cursor->yoff < max_scroll) {
            code_data->cursor->yoff++;
        }
    }
    else if (!inFindSubstrMode && chr == CTL_RIGHT){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];
        int len = arrlen(line)-2;

        if(x >= len){
            if(code_data->cursor->y+1 < arrlen(code_data->code_buffer)){
                code_data->cursor->x = 0;
                code_data->cursor->y++;
            }
            goto carried_to_next_line;
        }

        while(x < len && isspace(line[x]))
            x++;
        while(x < len && ispunct(line[x]) && !isspace(line[x]))
            x++;
        while(x < len && isalnum(line[x]))
            x++;

        code_data->cursor->x = x;

        LABEL(carried_to_next_line)
    }
    else if(!inFindSubstrMode && chr == CTL_LEFT){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];

        if(x == 0){
            if(code_data->cursor->y > 0){
                code_data->cursor->y--;
                int prev_line_size = arrlen(code_data->code_buffer[code_data->cursor->y]);
                code_data->cursor->x = prev_line_size-2 > 0 ? prev_line_size-2 : 0;
            }
            goto carried_to_previous_line;
        }

        while(x > 0 && isspace(line[x-1]))
            x--;
        while(x > 0 && ispunct(line[x-1]) && !isspace(line[x-1]))
            x--;
        while(x > 0 && isalnum(line[x-1]))
            x--;

        code_data->cursor->x = x;

        LABEL(carried_to_previous_line)
    }
    else if(!inFindSubstrMode && chr == CTL_BKSP){
        int x = code_data->cursor->x;
        char *line = code_data->code_buffer[code_data->cursor->y];

        if(x == 0){
            if (code_data->cursor->y > 0) {
                int prev_len = arrlen(code_data->code_buffer[code_data->cursor->y - 1]);
                int curr_len = arrlen(code_data->code_buffer[code_data->cursor->y]);

                if(code_data->parser && code_data->tree){
                    int del_row = code_data->cursor->y - 1;
                    int del_col = prev_len >= 2 ? prev_len - 2 : 0;
                    TSInputEdit edit = {
                        .start_byte    = buffer_byte_offset(code_data, del_row, del_col),
                        .old_end_byte  = buffer_byte_offset(code_data, code_data->cursor->y, 0),
                        .new_end_byte  = buffer_byte_offset(code_data, del_row, del_col),
                        .start_point   = {del_row, del_col},
                        .old_end_point = {code_data->cursor->y, 0},
                        .new_end_point = {del_row, del_col},
                    };
                    ts_tree_edit(code_data->tree, &edit);
                }

                arrsetlen(code_data->code_buffer[code_data->cursor->y - 1], prev_len-2);
                for(size_t i = 0; i < curr_len; i++){
                    arrput(code_data->code_buffer[code_data->cursor->y - 1], code_data->code_buffer[code_data->cursor->y][i]);
                }
                arrdel(code_data->code_buffer, code_data->cursor->y);
                code_data->cursor->y--;
                code_data->cursor->x = prev_len-2;

                TS_REPARSE(code_data);
            }
            goto carried_to_previous_line;
        }

        int new_x = x;
        while (new_x > 0 && isspace(line[new_x - 1]))
            new_x--;
        while (new_x > 0 && ispunct(line[new_x - 1]) && !isspace(line[new_x - 1]))
            new_x--;
        while (new_x > 0 && isalnum(line[new_x - 1]))
            new_x--;

        if(code_data->parser && code_data->tree){
            int row = code_data->cursor->y;
            TSInputEdit edit = {
                .start_byte    = buffer_byte_offset(code_data, row, new_x),
                .old_end_byte  = buffer_byte_offset(code_data, row, x),
                .new_end_byte  = buffer_byte_offset(code_data, row, new_x),
                .start_point   = {row, new_x},
                .old_end_point = {row, x},
                .new_end_point = {row, new_x},
            };
            ts_tree_edit(code_data->tree, &edit);
        }

        for(size_t i = 0; i < (x-new_x); i++){
            arrdel(line, new_x);
        }

        size_t new_len = arrlenu(code_data->code_buffer[code_data->cursor->y]);
        if(code_data->code_buffer[code_data->cursor->y][new_len - 2] != '\n'){
            arrput(code_data->code_buffer[code_data->cursor->y], '\n');
        }
        if(code_data->code_buffer[code_data->cursor->y][new_len - 1] != '\0'){
            arrput(code_data->code_buffer[code_data->cursor->y], '\0');
        }

        code_data->cursor->x = new_x;

        TS_REPARSE(code_data);
    }

    /* For debug CTRL + G */
    else if (!inFindSubstrMode && chr == 7){
        for(size_t i = 0; i < arrlenu(code_data->code_buffer[code_data->cursor->y]); i++){
            printf("%d ", code_data->code_buffer[code_data->cursor->y][i]);
        }
        printf("\n");
    }
    /* Temp shortcut for tree CTRL + T*/
    else if(!inFindSubstrMode && chr == 20) {
        console_execute_command(ccode, ":tree");
    }

    END_PROFILING("handle_keypress");
    START_PROFILING();

    bool is_file_edit = isprint(chr) ||
                        chr == CUSTOM_KEY_BACKSPACE ||
                        chr == CUSTOM_KEY_ENTER ||
                        chr == CTL_BKSP;

    bool is_printable = isprint(chr);

    LSPKind lsp_kind = lang_to_lspkind[code_data->lang];
    bool lsp_running = is_lspkind_running(ccode, lsp_kind);

    if(is_file_edit && lsp_running){
        send_to_lsp(ccode, get_running_lsp(ccode, lsp_kind));
    }

    if(code_data->completion_window){
        bool line_empty = arrlenu(code_data->code_buffer[code_data->cursor->y]) <= 2;
        if(chr != -1 && (!is_printable || line_empty || chr == CUSTOM_KEY_ENTER)){
            json_free(code_data->completion_window->completion);
            free(code_data->completion_window);
            code_data->completion_window = NULL;
        }
    }

    if(is_printable && lsp_running){
        LSPContext* ctx = get_running_lsp(ccode, lsp_kind);
        get_completion(ccode, ctx);
    }

    int content_height = y - 1;
    int content_width = x;

    int screen_y = code_data->cursor->y - code_data->cursor->yoff;
    if(screen_y < 0){
        code_data->cursor->yoff = code_data->cursor->y;
    } else if(screen_y >= content_height){
        code_data->cursor->yoff = code_data->cursor->y - content_height + 1;
    }

    int screen_x = code_data->cursor->x - code_data->cursor->xoff;
    if(screen_x < 0){
        code_data->cursor->xoff = code_data->cursor->x;
    } else if(screen_x >= content_width){
        code_data->cursor->xoff = code_data->cursor->x - content_width + 1;
    }
    END_PROFILING("send_to_lsp");
}


void layer_code_render(CCode* ccode, Layer* layer) {
    if (!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL) {
        return;
    }
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;

    START_PROFILING();
    apply_tree_sitter_syntax_highlighting(code_data, code_data->lang);
    END_PROFILING("apply_tree_sitter_syntax_highlighting");

    START_PROFILING();
    if(!code_data->completion_window){
        END_PROFILING("completion window");
        return;
    }

    int y, x;
    getmaxyx(stdscr, y, x);
    int cursor_screen_y = code_data->cursor->y - code_data->cursor->yoff + 1;
    int cursor_screen_x = code_data->cursor->x - code_data->cursor->xoff;
    JsonValue* result = shget(code_data->completion_window->completion->object, "result");
    JsonValue* items = shget(result->object, "items");

    size_t max_len = 0;
    for(size_t i = 0; i < arrlenu(items->array); i++){
        if(i > code_data->completion_window->items_count || cursor_screen_y+(i+1) > y) break;
        JsonValue* comp = items->array[i];
        JsonValue* text = shget(comp->object, "label");

        char* display_str = text->string;
        if(strlen(display_str) > 1 && (display_str[0] == ' ' || utf8_char_len(display_str[0]) != 1)){
            display_str += utf8_char_len(display_str[0]);
        }
        size_t len = strlen(display_str);

        if(len > max_len){
            int available = x - cursor_screen_x;
            if(available < 0) available = 0;
            max_len = (len > (size_t)available) ? (size_t)available : len;
        }
    }

    const char *padding = "                                                                                                                                                                                                  ";
    attron(COLOR_PAIR(COLOR_PAIR_COMPLETION));
    for(size_t i = 0; i < arrlenu(items->array); i++){
        if(i > code_data->completion_window->items_count || cursor_screen_y+(i+1) > y) break;
        if(i == code_data->completion_window->selected){
            attron(A_REVERSE);
        }

        JsonValue* comp = items->array[i];
        JsonValue* text = shget(comp->object, "label");
        char* display_str = text->string;
        if(strlen(display_str) > 1 && (display_str[0] == ' ' || utf8_char_len(display_str[0]) != 1)){
            display_str += utf8_char_len(display_str[0]);
        }

        size_t display_len = strlen(display_str);
        size_t print_len = (display_len < max_len) ? display_len : max_len;
        size_t padLen = max_len - print_len;

        mvprintw(cursor_screen_y+(i+1), cursor_screen_x,
                 "%.*s%*.*s", (int)print_len, display_str,
                 (int)padLen, (int)padLen, padding);

        if(i == code_data->completion_window->selected){
            attroff(A_REVERSE);
        }
    }
    attroff(COLOR_PAIR(COLOR_PAIR_COMPLETION));

    END_PROFILING("completion window");
}


void layer_code_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    layer_code_update(ccode, layer, chr);
    END_PROFILING("layer_code_update");
    if(should_draw){
        START_PROFILING();
        layer_code_render(ccode, layer);
        END_PROFILING("layer_code_render");
    }
    END_PROFILING("layer_code");
}

#endif // _H_LAYER_CODE