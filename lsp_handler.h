
typedef void (*LSPResponseHandler)(CCode* ccode, JsonValue* message);

typedef struct {
    char* key;
    LSPResponseHandler value;
} LSPResponseHandlerMap;

static LSPResponseHandlerMap* lsp_method_handler = NULL;
static LSPResponseHandlerMap* lsp_id_handler = NULL;


// Quite disgusting but I like it
#define TYPE_CHECK_FIELD(obj, key, expected_type) 				 \
do { 			 												 \
    key = shget((obj)->object, #key); 							 \
    if (!(key) || (key)->type != (expected_type)) { 		     \
        fprintf(stderr, "Invalid or missing field: %s\n", #key); \
        goto defer; 											 \
    }               											 \
} while(0) 													     \


//method: textDocument/publishDiagnostics
void handle_publishDiagnostics(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
		goto defer;
	}
	LayerCodeData* lcd = top_code_layer->layer_data;
	JsonValue* params = NULL;
	JsonValue* uri = NULL;
	JsonValue* version = NULL;

	// this create JsonValue* seconds_param = ...
	TYPE_CHECK_FIELD(message, params, JSON_OBJECT);
	TYPE_CHECK_FIELD(params, uri, JSON_STRING);
	TYPE_CHECK_FIELD(params, version, JSON_NUMBER);

	int version_int = (int)version->number;

	if(strcmp(lcd->uri, uri->string) == 0 && lcd->version == version_int){
		if(lcd->diagnostics != NULL){
			json_free(lcd->diagnostics);
		}
		// json_print(message, 4, 0);
		lcd->diagnostics = message;
		return;
	}

defer:
	json_free(message);
	return;
}

void handle_completion(CCode* ccode, JsonValue* message){
	Layer* top_code_layer = top_type_layer(ccode, LAYER_CODE);
	if(!top_code_layer){
		printf("CCode layers do not have any code layers!\n");
		goto defer;
	}
	LayerCodeData* lcd = top_code_layer->layer_data;
	JsonValue* result = NULL;
	JsonValue* items = NULL;

	TYPE_CHECK_FIELD(message, result, JSON_OBJECT);
	TYPE_CHECK_FIELD(result, items, JSON_ARRAY);
	
	if(lcd->completion){
		json_free(lcd->completion);
	}
	lcd->completion = message;
	return;

defer:
	json_free(message);
	return;
}

void init_lsp_handlers(){
	shput(lsp_method_handler, "textDocument/publishDiagnostics", handle_publishDiagnostics);

	shput(lsp_id_handler, "completion", handle_completion);
}

void destory_lsp_handlers(){
	shfree(lsp_method_handler);
	shfree(lsp_id_handler);
}

void handle_lsp(CCode* ccode){
	LSPContext* ctx = ccode->lsp_ctx;

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
		LSPResponseHandler handler = shget(lsp_method_handler, method->string);
		if(!handler){
			printf("Got message that dose not have handler %s\n", method->string);
			json_free(message);
		}else{
			handler(ccode, message);
		}
	}
	return;
}