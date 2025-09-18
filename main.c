#include <ctype.h>
#include <stdbool.h>

#include "curses.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"


#include "layers.h"


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
}

int main(int argc, char** argv) {
    NOB_UNUSED(argc);
    NOB_UNUSED(argv);

    Cursor cursor = {0};
    cursor.x      = 0;
    cursor.y      = 0;
    cursor.xoff   = 0;
    cursor.yoff   = 0;

    CCode ccode   = {0};
    ccode.cursor  = &cursor;
    ccode.layers  = NULL;

    // keep console in memory and reuse it 
    Layer* console_layer = new_layer_console();

    arrpush(ccode.layers, new_layer_code());

    int ch;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();

    move(1, 0);

    while(RUNNING) {
        ch = getch();
        // Code switch
        // FIXME: When switching codes the layers change in memory but the buffer isnt rendered right
        if(ch == CTL_TAB){
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
            int index = contains_layer(&ccode, console_layer);
            if(index == -1 && !CLOSE_CONSOLE){
                push_layer_to_top(&ccode, console_layer);
            }else{
                LayerConsoleData* lcd = (LayerConsoleData*)console_layer->layer_data;
                size_t zero = 0; // using variable to circumvent -Wtype-limits
                arrsetlen(lcd->console_buffer, zero);
                arrput(lcd->console_buffer, '\0');
                arrdel(ccode.layers, index);
                lcd->console_buffer_x = 0;
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
        int propagated_ch = ch;
        clear();
        if(ch != -1){
            print_layers(&ccode);
        }
        for (int i = arrlen(ccode.layers) - 1; i >= 0; i--){
            Layer* layer = ccode.layers[i];
            propagated_ch = ch;
            if(stop_input_propagation < i){
                propagated_ch = -1;
            }
            switch(layer->type){
                case LAYER_CODE: {
                    layer_code_handle_keypress(&ccode, layer, propagated_ch);
                    break;
                }
                case LAYER_CONSOLE: {
                    layer_console_handle_keypress(&ccode, layer, propagated_ch);
                    break;
                }
                default: {
                    fprintf(stderr, "Unknown Layer type %d\n", layer->type);
                    break;
                }
            }
        }
        refresh();
    }

    if(contains_layer(&ccode, console_layer) == -1){
        free_layer(console_layer);
    }
    free_ccode(&ccode);

    // Clean up
    endwin();   // End curses mode
    return 0;
}


