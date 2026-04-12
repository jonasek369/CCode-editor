typedef void (*LSPResponseHandler)(CCode* ccode, JsonValue* message);

typedef struct {
    char* key;
    LSPResponseHandler value;
} LSPResponseHandlerMap;

typedef struct {
	SyntaxLanguage key;
	LSPKind value;
} LangToLSP;

static LSPResponseHandlerMap* lsp_method_handler = NULL;
static LSPResponseHandlerMap* lsp_id_handler = NULL;
static LSPKind lang_to_lspkind[64];
static char* lsp_to_cstr_lang[64] = {
    [LSPKIND_CLANGD] = "c",
    [LSPKIND_PYLSP]   = "python",
    [LSPKIND_RUST_ANALYZER] = "rust"
};

// Quite disgusting but I like it
#define TYPE_CHECK_FIELD(obj, key, expected_type) 				 \
do { 			 												 \
    key = shget((obj)->object, #key); 							 \
    if (!(key) || (key)->type != (expected_type)) { 		     \
        fprintf(stderr, "Invalid or missing field: %s\n", #key); \
        goto defer; 											 \
    }               											 \
} while(0) 													     \

JsonValue* make_ccode_initialize_params(const char* root_uri) {
    JsonValue* params = json_new_object();

    // processId (null is fine)
    json_add_child(params, "processId", json_new_null());

    // rootUri
    if (root_uri) {
        json_add_child(params, "rootUri", json_new_string(root_uri));
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

    json_add_child(capabilities, "textDocument", textDocument);
    json_add_child(params, "capabilities", capabilities);

    JsonValue* clientInfo = json_new_object();
    json_add_child(clientInfo, "name", json_new_string("C-LSP-Client"));
    json_add_child(clientInfo, "version", json_new_string("0.1.0"));
    json_add_child(params, "clientInfo", clientInfo);

    return params;
}

//method: textDocument/publishDiagnostics
void handle_publishDiagnostics(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = NULL;
	for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE){
        	top_code_layer = ccode->layers[i];
        	break;
        }
    }
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
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
		int version_int = (int)version->number;

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

void handle_completion(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = NULL;
	for(int i = 0; i < arrlen(ccode->layers); i++){
        if(ccode->layers[i]->type == LAYER_CODE){
        	top_code_layer = ccode->layers[i];
        	break;
        }
    }
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
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
	}
	CompletionWindow* win = malloc(sizeof(CompletionWindow));
	win->completion = message;
	win->selected = 0;
	win->items_count = 6;
	if(arrlenu(items->array) < 6){
		win->items_count = arrlenu(items->array);
	} 

	lcd->completion_window = win;
	return;

defer:
	json_free(message);
	return;
}

void init_lsp_handler(){
	shput(lsp_method_handler, "textDocument/publishDiagnostics", handle_publishDiagnostics);

	shput(lsp_id_handler, "completion", handle_completion);

	for (int i = 0; i < 64; i++)
        lang_to_lspkind[i] = LSPKIND_UNKNOWN;

    lang_to_lspkind[LANG_C] = LSPKIND_CLANGD;
    lang_to_lspkind[LANG_PYTHON] = LSPKIND_PYLSP;
    lang_to_lspkind[LANG_RUST] = LSPKIND_RUST_ANALYZER;

    get_installed_lsps();
}

bool is_lspkind_running(CCode* ccode, LSPKind lsp_kind){
	if(ccode->lsp_ctxs == NULL){
		return false;
	}
	for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
		if(ccode->lsp_ctxs[i]->kind == lsp_kind){
			return true;
		}
	}
	return false;
}

LSPContext* get_running_lsp(CCode* ccode, LSPKind lsp_kind){
	if(ccode->lsp_ctxs == NULL){
		return NULL;
	}
	for(size_t i = 0; i < arrlenu(ccode->lsp_ctxs); i++){
		if(ccode->lsp_ctxs[i]->kind == lsp_kind){
			return ccode->lsp_ctxs[i];
		}
	}
	return NULL;
}

void destory_lsp_handlers(){
	shfree(lsp_method_handler);
	shfree(lsp_id_handler);
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