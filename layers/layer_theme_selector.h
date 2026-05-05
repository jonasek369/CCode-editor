#ifndef _H_LAYER_THEME_SELECTOR
#define _H_LAYER_THEME_SELECTOR

void layer_theme_selector_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);


Layer* new_layer_theme_selector(){
    Layer* tree = malloc(sizeof(Layer));
    if(!tree){
        return NULL;
    }
    tree->type = LAYER_THEME_SELECTOR;
    tree->consume_input = true;
    tree->draws_fullscreen = true;
    tree->handle_keypress_function = &layer_theme_selector_handle_keypress;

    LayerThemeSelectorData* ltsd = malloc(sizeof(LayerThemeSelectorData));
    if(!ltsd){
        free(tree);
        return NULL;
    }
    ltsd->current_dir_files = NULL;
    ltsd->selected = 0;
    ltsd->offset = 0;

    tree->layer_data = ltsd;
    return tree;
}


int load_themes(LayerThemeSelectorData* ltsd, const char* path){
    if(path == NULL){
        return -1;
    }
    Nob_File_Paths paths = {0};
    ltsd->selected = 0;
    ltsd->offset = 0;
    bool status = nob_read_entire_dir(path, &paths);
    if(!status){
        return -1;
    }
    if(paths.count > 0){
        qsort(paths.items, paths.count, sizeof(char*), pstrcmp);
    }
    char** add_to_top = NULL;
    for(size_t i = 0; i < paths.count; i++){
        if(strlen(paths.items[i]) <= 2 && (strncmp(paths.items[i], ".", 1) == 0 || strncmp(paths.items[i], "..", 2) == 0)){
            continue;
        }
        char* arr = str_to_arr(paths.items[i]);
        if(nob_get_file_type(nob_temp_sprintf("./themes/%s", arr)) == NOB_FILE_DIRECTORY){
            arrput(add_to_top, arr);
        }else{
            arrput(ltsd->current_dir_files, arr);
        }
    }
    if(add_to_top){
        for(size_t i = 0; i < arrlenu(add_to_top); i++){
            arrins(ltsd->current_dir_files, 0, add_to_top[i]);
        }
    }
    arrfree(add_to_top);
    free(paths.items);
    return 0;
}


bool layer_theme_selector_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_THEME_SELECTOR || layer->layer_data == NULL){
        return false;
    }

    LayerThemeSelectorData* ltsd = (LayerThemeSelectorData*) layer->layer_data;

    if(ltsd->current_dir_files == NULL){
        if(load_themes(ltsd, "./themes") == -1){
            printf("There was error reading the directory themes");
            return false;
        }
        char* theme_path = nob_temp_sprintf("./themes/%s", ltsd->current_dir_files[ltsd->selected]);
        ColorTheme* new_theme = load_theme(theme_path);
        if(new_theme){
            free_theme(ccode->config->theme);
            ccode->config->theme = new_theme;
            init_syntax_colors(ccode->config->theme);
        }
        nob_temp_reset();
    }

    int screen_y, screen_x;
    getmaxyx(stdscr, screen_y, screen_x);
    (void)screen_x;

    int list_height = screen_y - 1;

    if(chr == KEY_DOWN){
        if((ltsd->selected + 1) < arrlen(ltsd->current_dir_files)){
            ltsd->selected++;
            char* theme_path = nob_temp_sprintf("./themes/%s", ltsd->current_dir_files[ltsd->selected]);
            ColorTheme* new_theme = load_theme(theme_path);
            if(new_theme){
                free_theme(ccode->config->theme);
                ccode->config->theme = new_theme;
                init_syntax_colors(ccode->config->theme);
            }
            nob_temp_reset();
        }
    }

    if(chr == KEY_UP){
        if(ltsd->selected > 0){
            ltsd->selected--;
            char* theme_path = nob_temp_sprintf("./themes/%s", ltsd->current_dir_files[ltsd->selected]);
            ColorTheme* new_theme = load_theme(theme_path);
            if(new_theme){
                free_theme(ccode->config->theme);
                ccode->config->theme = new_theme;
                init_syntax_colors(ccode->config->theme);
            }
            nob_temp_reset();
        }
    }

    if(chr == CUSTOM_KEY_ENTER){
        remove_layer(ccode, layer);
        free_layer(layer);
        return true;
    }

    if(ltsd->selected < ltsd->offset){
        ltsd->offset = ltsd->selected;
    }

    if(ltsd->selected >= ltsd->offset + list_height){
        ltsd->offset = ltsd->selected - list_height + 1;
    }
    return false;
}


void layer_theme_selector_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_THEME_SELECTOR || layer->layer_data == NULL){
        return;
    }

    LayerThemeSelectorData* ltsd = (LayerThemeSelectorData*) layer->layer_data;

    int y, x;
    getmaxyx(stdscr, y, x);
    (void)x;

    int draw_y = 1;
    for(size_t i = ltsd->offset; i < arrlenu(ltsd->current_dir_files); i++){
        if(draw_y >= y) break;
        Nob_File_Type ft = dir_walk_get_file_type("./themes/", ltsd->current_dir_files[i]);
        if(ft == NOB_FILE_DIRECTORY){
            attron(COLOR_PAIR(COLOR_DIR));
        }else if (ft == NOB_FILE_SYMLINK){
            attron(COLOR_PAIR(COLOR_SYMLINK));
        }else{
            attron(COLOR_PAIR(COLOR_FILE));
        }

        if(i == ltsd->selected){
            attron(A_REVERSE);
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ltsd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ltsd->current_dir_files[i]);
            }
            attroff(A_REVERSE);
        } else {
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ltsd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ltsd->current_dir_files[i]);
            }
        }
        if(ft == NOB_FILE_DIRECTORY){
            attroff(COLOR_PAIR(COLOR_DIR));
        }else if (ft == NOB_FILE_SYMLINK){
            attroff(COLOR_PAIR(COLOR_SYMLINK));
        }else{
            attroff(COLOR_PAIR(COLOR_FILE));
        }

        draw_y++;
    }

    #define PREVIEW_X 36

    typedef struct { const char* text; int pair; } Seg;

    static const Seg preview[][16] = {
        { {"#include <stdio.h>", COLOR_PAIR_PREPROC}, {NULL,0} },
        { {NULL, 0} },
        { {"// Entry point", COLOR_PAIR_COMMENT}, {NULL,0} },
        { {"int", COLOR_PAIR_TYPE}, {" ", COLOR_PAIR_DEFAULT}, {"main", COLOR_PAIR_FUNCTION}, {"()", COLOR_PAIR_OPERATOR}, {" {", COLOR_PAIR_OPERATOR}, {NULL,0} },
        { {"    ", COLOR_PAIR_DEFAULT}, {"int", COLOR_PAIR_TYPE}, {" x ", COLOR_PAIR_DEFAULT}, {"=", COLOR_PAIR_OPERATOR}, {" ", COLOR_PAIR_DEFAULT}, {"42", COLOR_PAIR_NUMBER}, {";", COLOR_PAIR_OPERATOR}, {NULL,0} },
        { {"    ", COLOR_PAIR_DEFAULT}, {"const", COLOR_PAIR_KEYWORD}, {" char", COLOR_PAIR_TYPE}, {"* msg ", COLOR_PAIR_DEFAULT}, {"=", COLOR_PAIR_OPERATOR}, {" ", COLOR_PAIR_DEFAULT}, {"\"hello\"", COLOR_PAIR_STRING}, {";", COLOR_PAIR_OPERATOR}, {NULL,0} },
        { {"    ", COLOR_PAIR_DEFAULT}, {"printf", COLOR_PAIR_FUNCTION}, {"(", COLOR_PAIR_OPERATOR}, {"msg", COLOR_PAIR_DEFAULT}, {")", COLOR_PAIR_OPERATOR}, {";", COLOR_PAIR_OPERATOR}, {NULL,0} },
        { {"    ", COLOR_PAIR_DEFAULT}, {"return", COLOR_PAIR_KEYWORD}, {" ", COLOR_PAIR_DEFAULT}, {"0", COLOR_PAIR_NUMBER}, {";", COLOR_PAIR_OPERATOR}, {NULL,0} },
        { {"}", COLOR_PAIR_OPERATOR}, {NULL,0} },
    };

    int preview_lines = (int)(sizeof(preview) / sizeof(preview[0]));

    for (int pl = 0; pl < preview_lines && (pl + 1) < y; pl++) {
        int col = PREVIEW_X;
        for (int s = 0; preview[pl][s].text != NULL; s++) {
            const Seg* seg = &preview[pl][s];
            attron(COLOR_PAIR(seg->pair));
            mvprintw(pl + 1, col, "%s", seg->text);
            col += (int)strlen(seg->text);
            attroff(COLOR_PAIR(seg->pair));
        }
        move(pl + 1, col);
        clrtoeol();
    }

    #undef PREVIEW_X
}


void layer_theme_selector_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    bool closed = layer_theme_selector_update(ccode, layer, chr);
    END_PROFILING("layer_theme_selector_update");
    if(closed){
        END_PROFILING("layer_theme_selector");
        return;
    }
    if(should_draw){
        START_PROFILING();
        layer_theme_selector_render(ccode, layer);
        END_PROFILING("layer_theme_selector_render");
    }
    END_PROFILING("layer_theme_selector");
}

#endif // _H_LAYER_THEME_SELECTOR
