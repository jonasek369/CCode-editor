#ifndef _H_LAYER_DIR_WALK
#define _H_LAYER_DIR_WALK


void layer_dir_walk_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);

Layer* new_layer_dir_walk(char* dir){
    Layer* tree = malloc(sizeof(Layer));
    if(!tree){
        return NULL;
    }
    tree->type = LAYER_DIR_WALK;
    tree->consume_input = true;
    tree->draws_fullscreen = true;
    tree->handle_keypress_function = &layer_dir_walk_handle_keypress;

    LayerDirWalkData* ldwd = malloc(sizeof(LayerDirWalkData));
    if(!ldwd){
        free(tree);
        return NULL;
    }
    ldwd->current_dir_path = dir;
    ldwd->current_dir_files = NULL;
    ldwd->selected = 0;
    ldwd->offset = 0;

    tree->layer_data = ldwd;
    return tree;
}


Nob_File_Type dir_walk_get_file_type(char* current_dir, char* filename){
    Nob_File_Type file_type = nob_get_file_type(nob_temp_sprintf("%s/%s", current_dir, filename));
    nob_temp_reset();
    return file_type;
}


int load_dir(LayerDirWalkData* ldwd){
    if(ldwd->current_dir_path == NULL){
        return -1;
    }
    Nob_File_Paths paths = {0};
    ldwd->selected = 0;
    ldwd->offset = 0;
    bool status = nob_read_entire_dir(ldwd->current_dir_path, &paths);
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
        if(nob_get_file_type(nob_temp_sprintf("%s/%s", ldwd->current_dir_path, arr)) == NOB_FILE_DIRECTORY){
            arrput(add_to_top, arr);
        }else{
            arrput(ldwd->current_dir_files, arr);
        }
    }
    if(add_to_top){
        for(size_t i = 0; i < arrlenu(add_to_top); i++){
            arrins(ldwd->current_dir_files, 0, add_to_top[i]);
        }
    }
    char* go_back_dir = NULL;
    arrput(go_back_dir, '.');
    arrput(go_back_dir, '.');
    arrput(go_back_dir, '\0');

    arrins(ldwd->current_dir_files, 0, go_back_dir);

    arrfree(add_to_top);
    free(paths.items);
    return 0;
}


bool layer_dir_walk_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_DIR_WALK || layer->layer_data == NULL){
        return false;
    }

    LayerDirWalkData* ldwd = (LayerDirWalkData*) layer->layer_data;

    if(ldwd->current_dir_files == NULL){
        if(load_dir(ldwd) == -1){
            printf("There was error reading the directory %s", ldwd->current_dir_path);
            return false;
        }
    }

    int screen_y, screen_x;
    getmaxyx(stdscr, screen_y, screen_x);
    (void)screen_x;

    int list_height = screen_y - 3;

    if(chr == KEY_DOWN){
        if((ldwd->selected + 1) < arrlen(ldwd->current_dir_files)){
            ldwd->selected++;
        }
    }

    if(chr == KEY_UP){
        if(ldwd->selected > 0){
            ldwd->selected--;
        }
    }

    if(ldwd->selected < ldwd->offset){
        ldwd->offset = ldwd->selected;
    }

    if(ldwd->selected >= ldwd->offset + list_height){
        ldwd->offset = ldwd->selected - list_height + 1;
    }

    if(chr == CUSTOM_KEY_ENTER){
        char* dir = nob_temp_sprintf("%s/%s",
            ldwd->current_dir_path,
            ldwd->current_dir_files[ldwd->selected]);
        char resolved_path[MAX_PATH] = {0};

        if(resolve_path(dir, resolved_path) == NULL){
            return false;
        }

        switch(nob_get_file_type(resolved_path)){
            case NOB_FILE_DIRECTORY: {
                char* arr = str_to_arr(resolved_path);

                change_tree_path(layer, arr);

                load_dir(ldwd);

                nob_temp_reset();
                break;
            }

            default: {
                char* arr = str_to_arr(resolved_path);
                read_file_to_code_layer(ccode, arr, arrlen(arr)-1);
                arrfree(arr);
                remove_layer(ccode, layer);
                free_layer(layer);
                return true;
            }
        }
    }
    return false;
}


void layer_dir_walk_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_DIR_WALK || layer->layer_data == NULL){
        return;
    }

    LayerDirWalkData* ldwd = (LayerDirWalkData*) layer->layer_data;

    mvprintw(1, 0, "Current dir: %s", ldwd->current_dir_path);
    int y, x;
    getmaxyx(stdscr, y, x);
    (void)x;

    int draw_y = 3;
    for(size_t i = ldwd->offset; i < arrlenu(ldwd->current_dir_files); i++){
        if(draw_y >= y) break;
        Nob_File_Type ft = dir_walk_get_file_type(ldwd->current_dir_path, ldwd->current_dir_files[i]);
        if(ft == NOB_FILE_DIRECTORY){
            attron(COLOR_PAIR(COLOR_DIR));
        }else if (ft == NOB_FILE_SYMLINK){
            attron(COLOR_PAIR(COLOR_SYMLINK));
        }else{
            attron(COLOR_PAIR(COLOR_FILE));
        }

        if(i == ldwd->selected){
            attron(A_REVERSE);
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ldwd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ldwd->current_dir_files[i]);
            }
            attroff(A_REVERSE);
        } else {
            if(ft == NOB_FILE_DIRECTORY){
                mvprintw(draw_y, 0,"/%s", ldwd->current_dir_files[i]);
            }else{
                mvprintw(draw_y, 0,"%s", ldwd->current_dir_files[i]);
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
}


void layer_dir_walk_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    bool redirected = layer_dir_walk_update(ccode, layer, chr);
    END_PROFILING("layer_dir_walk_update");
    if(redirected){
        END_PROFILING("layer_dir_walk");
        return;
    }
    if(should_draw){
        START_PROFILING();
        layer_dir_walk_render(ccode, layer);
        END_PROFILING("layer_dir_walk_render");
    }
    END_PROFILING("layer_dir_walk");
}

#endif // _H_LAYER_DIR_WALK
