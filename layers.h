#ifndef _H_LAYERS
#define _H_LAYERS

#include "defines.h"
#include "tokenizer.h"


/*
    Layer constructors
*/

Layer* new_layer_code();
Layer* new_layer_console();
Layer* new_layer_dir_walk(char* dir);
Layer* new_layer_theme_selector();

/*
    Layer management helpers
*/

void push_layer_to_top(CCode* ccode, Layer* to_top);
void push_layer_to_bot(CCode* ccode, Layer* to_bot);
Layer* top_layer(CCode* ccode);
void remove_layer(CCode* ccode, Layer* target);
Layer** all_type_layers(CCode* ccode, LayerType type);
Layer* top_type_layer(CCode* ccode, LayerType type);
Cursor* top_code_layer_cursor(CCode* ccode);
int contains_layer(CCode* ccode, Layer* layer);
Layer* layer_at_index(CCode* ccode, int index);
void free_layer(Layer* layer);
char* layertype_to_str(LayerType lt);
void print_layer(Layer* l);

/*
    File manipulation / console
*/

size_t buffer_byte_offset(LayerCodeData *code, int row, int col);
char* flatten_buffer(LayerCodeData *code);
void message_to_console(CCode* ccode, const char* message);
void console_execute_command(CCode* ccode, const char* buffer);
void change_tree_path(Layer* layer, char* new_path);
/*
    UI
*/

void draw_ui(CCode* ccode);

#include "./layers/layer_code.h"
#include "./layers/layer_console.h"
#include "./layers/layer_dir_walk.h"
#include "./layers/layer_theme_selector.h"
#include "./layers/layers_impl.h"

#endif