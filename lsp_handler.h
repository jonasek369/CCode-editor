typedef void (*LSPResponseHandler)(CCode* ccode, JsonValue* message);

typedef struct {
    char* key;
    LSPResponseHandler value;
} LSPResponseHandlerMap;

typedef struct {
    char* key;
    LSPID value;
} LSPFileSuffix;

typedef struct {
	LSPID key;
	char* value;
} LSPLanguageid;

static LSPResponseHandlerMap* lsp_method_handler = NULL;
static LSPResponseHandlerMap* lsp_id_handler = NULL;

static LSPFileSuffix* lsp_file_suffix_register = NULL;
static LSPLanguageid* lsp_language_id_register = NULL;

// Quite disgusting but I like it
#define TYPE_CHECK_FIELD(obj, key, expected_type) 				 \
do { 			 												 \
    key = shget((obj)->object, #key); 							 \
    if (!(key) || (key)->type != (expected_type)) { 		     \
        fprintf(stderr, "Invalid or missing field: %s\n", #key); \
        goto defer; 											 \
    }               											 \
} while(0) 													     \

JsonValue* make_ccode_initialize_params(const char* file_uri) {
    JsonValue* params = json_new_object();

    // processId (null is fine)
    json_add_child(params, "processId", json_new_null());

    // rootUri
    if (file_uri) {
    	char* uri = project_uri_from_file_uri(file_uri); 
        json_add_child(params, "rootUri", json_new_string(uri));
        arrfree(uri);
    } else {
        json_add_child(params, "rootUri", json_new_null());
    }

    JsonValue* capabilities = json_new_object();
    JsonValue* textDocument = json_new_object();


    JsonValue* sync = json_new_object();
    json_add_child(sync, "didOpen", json_new_bool(true));
    json_add_child(sync, "didChange", json_new_bool(true));
    json_add_child(sync, "willSave", json_new_bool(false));
    json_add_child(sync, "didSave", json_new_bool(false));
    json_add_child(textDocument, "synchronization", sync);

    JsonValue* completion = json_new_object();
    json_add_child(completion, "dynamicRegistration", json_new_bool(false));

    JsonValue* completionItem = json_new_object();
    json_add_child(completionItem, "snippetSupport", json_new_bool(true));

    JsonValue* docFormats = json_new_array();
    json_add_child(docFormats, NULL, json_new_string("markdown"));
    json_add_child(docFormats, NULL, json_new_string("plaintext"));

    json_add_child(completionItem, "documentationFormat", docFormats);
    json_add_child(completion, "completionItem", completionItem);

    json_add_child(textDocument, "completion", completion);
    json_add_child(textDocument, "versionSupport", json_new_bool(true));
    json_add_child(textDocument, "linkSupport", json_new_bool(true));	

    if(file_uri){
        JsonValue* workspace_c   = json_new_object();
    	json_add_child(workspace_c, "workspaceFolders", json_new_bool(true));
    	json_add_child(capabilities, "workspace", workspace_c);	
    }

    json_add_child(capabilities, "textDocument", textDocument);
    json_add_child(params, "capabilities", capabilities);

    JsonValue* clientInfo = json_new_object();
    json_add_child(clientInfo, "name", json_new_string("C-LSP-Client"));
    json_add_child(clientInfo, "version", json_new_string("0.1.0"));
    json_add_child(params, "clientInfo", clientInfo);

    if(file_uri){
		JsonValue* workspaces = json_new_array();

    	JsonValue* workspace = json_new_object();
    	char* uri = project_uri_from_file_uri(file_uri); 
    	json_add_child(workspace, "uri", json_new_string(uri));
    	json_add_child(workspace, "name", json_new_string("project"));
    	arrfree(uri);
    	json_add_child(workspaces, NULL, workspace);

    	json_add_child(params, "workspaceFolders", workspaces);
    }

    return params;
}

//method: textDocument/publishDiagnostics
void handle_publishDiagnostics(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = NULL;
	printf("recieve handle of diagnostics\n");
	for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE || ccode->layers[i]->type == LAYER_SPLIT_VIEW){
        	top_code_layer = ccode->layers[i];
        	break;
        }
    }
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
		goto defer;
	}
	if(top_code_layer->type == LAYER_SPLIT_VIEW){
    	LayerSplitViewData* lsvd = top_code_layer->layer_data;
    	if(lsvd->splitten_layers != NULL && arrlen(lsvd->splitten_layers) > 0){
    		top_code_layer = lsvd->splitten_layers[lsvd->focused];
    	}	
    }
    if(top_code_layer->type != LAYER_CODE){
    	goto defer;
    }
	LayerCodeData* lcd = top_code_layer->layer_data;
	JsonValue* params = NULL;
	JsonValue* uri = NULL;
	JsonValue* version = NULL;

	TYPE_CHECK_FIELD(message, params, JSON_OBJECT);
	TYPE_CHECK_FIELD(params, uri, JSON_STRING);
	version = shget(params->object, "version");
	// pylsp dose not return version
	if(version){
		int version_int = json_get_integer(version);

		if(strcmp(lcd->uri, uri->string) == 0 && lcd->version == version_int){
			if(lcd->diagnostics != NULL){
				json_free(lcd->diagnostics);
			}
			if(lcd->ranges){
				for(size_t i = 0; i < arrlenu(lcd->ranges); i++){
					if(lcd->ranges[i])
						free(lcd->ranges[i]);
				}
				arrfree(lcd->ranges);
				lcd->ranges = NULL;
			}
			lcd->diagnostics = message;
			printf("setting diagnostics!\n");
			JsonValue* params = shget(lcd->diagnostics->object, "params");
            JsonValue* diags  = shget(params->object, "diagnostics");
			arrsetlen(lcd->ranges, arrlenu(diags->array));
			for(size_t i = 0; i < arrlenu(lcd->ranges); i++){
				lcd->ranges[i] = NULL;
			}
			return;
		}
	}else{
		if(strcmp(lcd->uri, uri->string) == 0){
			if(lcd->diagnostics != NULL){
				json_free(lcd->diagnostics);
			}
			if(lcd->ranges){
				for(size_t i = 0; i < arrlenu(lcd->ranges); i++){
					if(lcd->ranges[i])
						free(lcd->ranges[i]);
				}
				arrfree(lcd->ranges);
				lcd->ranges = NULL;
			}
			lcd->diagnostics = message;
			JsonValue* params = shget(lcd->diagnostics->object, "params");
            JsonValue* diags  = shget(params->object, "diagnostics");
			arrsetlen(lcd->ranges, arrlenu(diags->array));
			for(size_t i = 0; i < arrlenu(lcd->ranges); i++){
				lcd->ranges[i] = NULL;
			}
			return;
		}
	}


defer:
	json_free(message);
	return;
}


void handle_definition(CCode* ccode, JsonValue* message){
	JsonValue* result = NULL;
	JsonValue* uri = NULL;
	LSPRange*  range = NULL;
	TYPE_CHECK_FIELD(message, result, JSON_ARRAY);
	if(arrlenu(result->array) > 1){
		printf("Found multiple definitions opening only first!\n");
	}
	if(arrlenu(result->array) == 0){
		goto defer;
	}
	JsonValue* first_result = result->array[0];
	TYPE_CHECK_FIELD(first_result, uri, JSON_STRING);
	range = get_range(first_result);
	for(size_t i = 0; i < arrlenu(ccode->layers); i++){
		if(ccode->layers[i]->type == LAYER_CODE){
			LayerCodeData* lcd = ccode->layers[i]->layer_data;
			if(!lcd->uri) continue;
			if(strcmp(uri->string, lcd->uri) == 0){
				push_layer_to_top(ccode, ccode->layers[i]);
				lcd->cursor->x = range->start_character;
				lcd->cursor->y = range->start_line;
				goto defer;
			} 
		}else if(ccode->layers[i]->type == LAYER_SPLIT_VIEW){
			LayerSplitViewData* lsvd = ccode->layers[i]->layer_data;
			LayerCodeData* focused = lsvd->splitten_layers[(int)lsvd->focused]->layer_data;
			LayerCodeData* unfocused = lsvd->splitten_layers[(int)(!lsvd->focused)]->layer_data;

			if(focused->uri && strcmp(focused->uri, uri->string) == 0){
				focused->cursor->x = range->start_character;
				focused->cursor->y = range->start_line;
				goto defer;
			}else if(unfocused->uri && strcmp(unfocused->uri, uri->string) == 0){
				lsvd->focused = !lsvd->focused;
				unfocused->cursor->x = range->start_character;
				unfocused->cursor->y = range->start_line;
				goto defer;
			}
		}
	}
	Layer* top_code_layer = NULL;
	for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE || ccode->layers[i]->type == LAYER_SPLIT_VIEW){
        	top_code_layer = ccode->layers[i];
        	break;
        }
    }
    if(top_code_layer == NULL || top_code_layer == NULL){
    	Layer* layer = read_file_to_code_layer(ccode, uri->string + 6, strlen(uri->string) - 6, true);
		LayerCodeData* lcd = layer->layer_data;
		lcd->cursor->x = range->start_character;
		lcd->cursor->y = range->start_line;
		goto defer;
    }

	Layer* layer = read_file_to_code_layer(ccode, uri->string + 6, strlen(uri->string) - 6, false);
	LayerCodeData* lcd = layer->layer_data;
	lcd->cursor->x = range->start_character;
	lcd->cursor->y = range->start_line;
	
	make_parser(layer, get_filename(lcd->uri));
	file_on_open_callback(ccode, (void*)layer);

	remove_layer(ccode, top_code_layer);

	Layer* split = new_layer_split_view();
	LayerSplitViewData* lsvd = split->layer_data;
	make_split_view(split, top_code_layer, layer);
	lsvd->focused = 1;

	push_layer_to_top(ccode, split);
 

defer:
	if(range){
		free(range);
	}
	json_free(message);
	return;
}


void handle_completion(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = NULL;
	for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE || ccode->layers[i]->type == LAYER_SPLIT_VIEW){
        	top_code_layer = ccode->layers[i];
        	break;
        }
    }
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
		goto defer;
	}
	if(top_code_layer->type == LAYER_SPLIT_VIEW){
    	LayerSplitViewData* lsvd = top_code_layer->layer_data;
    	if(lsvd->splitten_layers != NULL && arrlen(lsvd->splitten_layers) > 0){
    		top_code_layer = lsvd->splitten_layers[lsvd->focused];
    	}	
    }
    if(top_code_layer->type != LAYER_CODE){
    	goto defer;
    }
	LayerCodeData* lcd = top_code_layer->layer_data;
	JsonValue* result = NULL;
	JsonValue* items = NULL;

	TYPE_CHECK_FIELD(message, result, JSON_OBJECT);
	TYPE_CHECK_FIELD(result, items, JSON_ARRAY);
	
	if(lcd->completion_window){
		json_free(lcd->completion_window->completion);
		free(lcd->completion_window);
		lcd->completion_window = NULL;
	}
	if(arrlenu(items->array) == 0){
		goto defer;
	}
	CompletionWindow* win = malloc(sizeof(CompletionWindow));
	win->completion = message;
	win->selected = 0;
	win->items_count = arrlenu(items->array) < 6 ? arrlenu(items->array) : 6;  

	lcd->completion_window = win;
	return;

defer:
	json_free(message);
	return;
}

void init_lsp_handler(){
	shput(lsp_method_handler, "textDocument/publishDiagnostics", handle_publishDiagnostics);

	shput(lsp_id_handler, "completion", handle_completion);
	shput(lsp_id_handler, "definition", handle_definition);
}

void register_lsp_from_config(CCode* ccode){
	CCodeConfig* config = ccode->config;

	if(!config->lsps) return;
	printf("loading lsps\n");

	if(lsp_file_suffix_register){
		shfree(lsp_file_suffix_register);
		lsp_file_suffix_register = NULL;
	}
	if(lsp_language_id_register){
		hmfree(lsp_language_id_register);
		lsp_language_id_register = NULL;
	}

	clear_lsp_registry();

	// Stop all lps when reloading
    if(ccode->lsp_ctxs){
        for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
            destroy_lsp(ccode->lsp_ctxs[i]);
        }
        arrfree(ccode->lsp_ctxs);
        ccode->lsp_ctxs = NULL;
    }

	for(size_t i = 0; i < arrlenu(config->lsps); i++){
		ConfigLSP* clsp = config->lsps[i];
		LSPID lsp_id = register_lsp(clsp->name, clsp->cmd);
		if(lsp_id > 0){
			for(size_t j = 0; j < arrlenu(clsp->file_suffixes); j++){
				shput(lsp_file_suffix_register, clsp->file_suffixes[j], lsp_id);
			}
			hmput(lsp_language_id_register, lsp_id, clsp->lsp_language_id);
			printf("Registered lsp %s as %d\n", clsp->name, lsp_id);
		}else{
			printf("Could not register %s\n", clsp->name);
		}
	}
}

void destory_lsp_handlers(){
	shfree(lsp_method_handler);
	shfree(lsp_id_handler);

	shfree(lsp_file_suffix_register);
	hmfree(lsp_language_id_register);
}

void handle_lsp(CCode* ccode, LSPContext* ctx){
	while(1){
		JsonValue* message = tiny_queue_pop_nowait(ctx->receiver_queue);
		if(!message){
			break;
		}
		JsonValue* method = shget(message->object, "method");
		if(!method || method->type != JSON_STRING){
			JsonValue* id = shget(message->object, "id");
			if(id->type == JSON_STRING){
				LSPResponseHandler handler = shget(lsp_id_handler, id->string);
				if(!handler){
					printf("id: '%s' dose not have handler", id->string);
					json_free(message);
					continue;
				}
				handler(ccode, message);
				continue;
			}
			printf("Error: server returned message without method or method isnt string or id isnt string!\n");
			json_print(message, 4, 0);
			json_free(message);
			continue;
		}
		char* unescaped_method = json_unescape(method->string);
		LSPResponseHandler handler = shget(lsp_method_handler, unescaped_method);
		if(!handler){
			printf("Got message that dose not have handler %s\n", method->string);
			json_free(message);
		}else{
			handler(ccode, message);
		}
		free(unescaped_method);
	}
	return;
}