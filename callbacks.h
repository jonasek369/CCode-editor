#ifndef _H_CALLBACKS
#define _H_CALLBACKS

void file_remove_callback(CCode* ccode, void* data){
    char* filename = (char*)data;
    if(!filename) return;
    char* cwd = strdup(nob_get_current_dir_temp());
    char* file_path = nob_temp_sprintf("%s/%s", cwd, filename);

    nob_delete_file(file_path);

    for(size_t i = 0; i < arrlenu(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_DIR_WALK) refresh_tree_files(ccode->layers[i]);
    }

    free(cwd);
    free(filename);
    nob_temp_reset();
}

void file_not_remove_callback(CCode* ccode, void* data){
    (void) ccode;
    char* filename = (char*)data;
    if(!filename) return;
    free(filename);
}

void config_save_callback(CCode* ccode, void* data){
    (void) data;
    write_code_layer_to_file(ccode, true);
}

void config_cleanup_callback(CCode* ccode, void* data){
    (void) ccode;
    (void) data;
    return;
}


bool file_before_save_callback(CCode* ccode, void* data){
    if(!data) return true;
    LayerCodeData* lcd = ((Layer*)data)->layer_data;
    
    char path[4096];
    char config_path[8096];

    get_config_directory(path, sizeof(path));
    snprintf(config_path, sizeof(config_path), "%s/config.json", path);
    if(strncmp(config_path, lcd->filename, strlen(config_path)) == 0){
        char question[4096] = {0};

        snprintf(question, sizeof(question), "Are you sure you want to save config.json? If the file contains invalid syntax, the editor will fallback to default");

        Layer* dlg = new_layer_floating_dialog(
            question,
            &config_save_callback,
            &config_cleanup_callback,
            NULL
        );
        push_layer_to_top(ccode, dlg);
        return false;
    }
    return true;
}


void file_on_save_callback(CCode* ccode, void* data){
    if(!data) return;
    LayerCodeData* lcd = ((Layer*)data)->layer_data;

    file_on_change_callback(ccode, data);

    char path[4096];
    char config_path[8096];

    get_config_directory(path, sizeof(path));
    snprintf(config_path, sizeof(config_path), "%s/config.json", path);
    if(strncmp(config_path, lcd->filename, strlen(config_path)) == 0){
        free_config(ccode->config);
        ccode->config = load_config(config_path);
        init_syntax_colors(ccode->config->theme);
        register_lsp_from_config(ccode);
    }
}

void file_on_open_callback(CCode* ccode, void* data){
    if(!data) return;
    Layer* code_layer = data;
    LayerCodeData* lcd = code_layer->layer_data;
    const char* suffix = get_suffix(lcd->filename);
    LSPID matching_lsp = shget(lsp_file_suffix_register, suffix);
    if(matching_lsp < 1) return;
    char* language_id = hmget(lsp_language_id_register, matching_lsp);
    bool running = false;
    LSPContext* running_ctx = NULL;
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            LSPContext* ctx = ccode->lsp_ctxs[i];
            if(ctx->kind == matching_lsp){
                running = true;
                running_ctx = ctx;
            }
        }
    }
    if(!running){
        running_ctx = start_lsp(matching_lsp, make_ccode_initialize_params(lcd->uri));
        arrput(ccode->lsp_ctxs, running_ctx);
        printf("Started LSP %d\n", matching_lsp);
    }
    char* flatten = flatten_buffer(lcd);
    char* escaped = json_escape(flatten);
    JsonValue* params = make_didOpen_params(lcd->uri, language_id, 1, escaped);
    JsonValue* message = make_didOpen_notification(params);
    tiny_queue_push(running_ctx->sender_queue, message);
    free(flatten);
    free(escaped);
}

void file_on_change_callback(CCode* ccode, void* data){
    if(!data) return;
    Layer* code_layer = data;
    LayerCodeData* lcd = code_layer->layer_data;
    const char* suffix = get_suffix(lcd->filename);
    LSPID matching_lsp = shget(lsp_file_suffix_register, suffix);
    if(matching_lsp < 1) return;
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            LSPContext* ctx = ccode->lsp_ctxs[i];
            if(ctx->kind == matching_lsp){
                // clear old diagnostics
                if(lcd->diagnostics != NULL){
                    json_free(lcd->diagnostics);
                    lcd->diagnostics = NULL;
                }
                char* flatten = flatten_buffer(lcd);
                char* escaped = json_escape(flatten);

                JsonValue* params = make_didChange_params(lcd->uri, ++(lcd->version), escaped);
                JsonValue* message = make_didChange_notification(params);
                tiny_queue_push(ctx->sender_queue, message);
                free(flatten);
                free(escaped);
            }
        }
    }
}


// when user types
void file_on_type_callback(CCode* ccode, void* data){
    if(!data) return;
    Layer* code_layer = data;
    LayerCodeData* lcd = code_layer->layer_data;
    const char* suffix = get_suffix(lcd->filename);
    LSPID matching_lsp = shget(lsp_file_suffix_register, suffix);
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            LSPContext* ctx = ccode->lsp_ctxs[i];
            if(ctx->kind == matching_lsp){
                Cursor* cursor = lcd->cursor;

                uint64_t line = cursor->y;
                uint64_t character = cursor->x;

                JsonValue* params = make_completion_params(lcd->uri, line, character);
                JsonValue* message = make_completion_request(json_new_string("completion"), params);
                tiny_queue_push(ctx->sender_queue, message);
            }
        }
    }
}

void file_on_close_callback(CCode* ccode, void* data){
    if(!data) return;
    Layer* code_layer = data;
    LayerCodeData* lcd = code_layer->layer_data;
    const char* suffix = get_suffix(lcd->filename);
    LSPID matching_lsp = shget(lsp_file_suffix_register, suffix);
    if(matching_lsp < 1) return;

    bool running = false;
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            LSPContext* ctx = ccode->lsp_ctxs[i];
            if(ctx->kind == matching_lsp){
                running = true;
            }
        }
    }
    if(!running) return;

    size_t same_langs = 0;
    Layer** code_layers = all_type_layers(ccode, LAYER_CODE);
    for(size_t i = 0; i < arrlenu(code_layers); i++){
        if(code_layer == code_layers[i]) continue;
        if(lcd->lang == ((LayerCodeData*)code_layers[i]->layer_data)->lang) same_langs++;
    }
    if(same_langs == 0){
        size_t at_index = 0;
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            if(ccode->lsp_ctxs[i]->kind == matching_lsp){
                at_index = i;
                break;
            }
        }
        destroy_lsp(ccode->lsp_ctxs[at_index]);
        arrdel(ccode->lsp_ctxs, at_index);
        printf("Shutdown LSP\n");
    }
    arrfree(code_layers);
}

void file_find_definition_callback(CCode* ccode, void* data){
    if(!data) return;
    Layer* code_layer = data;
    LayerCodeData* lcd = code_layer->layer_data;
    const char* suffix = get_suffix(lcd->filename);
    LSPID matching_lsp = shget(lsp_file_suffix_register, suffix);
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            LSPContext* ctx = ccode->lsp_ctxs[i];
            if(ctx->kind == matching_lsp){
                Cursor* cursor = lcd->cursor;

                uint64_t line = cursor->y;
                uint64_t character = cursor->x;

                JsonValue* params = make_definition_params(lcd->uri, line, character);
                JsonValue* message = make_definition_request(json_new_string("definition"), params);
                tiny_queue_push(ctx->sender_queue, message);
            }
        }
    }
}

#endif