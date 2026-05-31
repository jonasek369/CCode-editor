#ifndef _H_FLOATING_TREE
#define _H_FLOATING_TREE

void layer_floating_tree_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);
bool layer_floating_tree_update(CCode* ccode, Layer* layer, int chr);


Layer* new_layer_floting_tree(){
	Layer* float_tree = malloc(sizeof(Layer));
    if(!float_tree){
        return NULL;
    }
    float_tree->type = LAYER_FLOATING_TREE;
    float_tree->consume_input = true;
    float_tree->draws_fullscreen = false;
    float_tree->handle_keypress_function = &layer_floating_tree_handle_keypress;
    float_tree->update_function = &layer_floating_tree_update;

    LayerFloatingTreeData* lftd = malloc(sizeof(LayerFloatingTreeData));
    if(!lftd){
    	free(float_tree);
    	return NULL;
    }
   
    int x, y;
    getmaxyx(stdscr, y, x);
    int width = x/3;
    int height = y/4;
    lftd->selected = 0;
    lftd->offset = 0;
    lftd->virtual_window = malloc(sizeof(VirtualWindow));
    if(!lftd->virtual_window){
    	free(float_tree);
    	free(lftd);
    	return NULL;
    }
    lftd->virtual_window->x = (x/2) - width/2;
    lftd->virtual_window->y = 2;
    lftd->virtual_window->width = width;
    lftd->virtual_window->height = height;
    lftd->search_buffer = NULL;
    arrput(lftd->search_buffer, '\0');
    lftd->search_buffer_x = 0;
    const char* temp_cwd = nob_get_current_dir_temp();
    lftd->cwd = str_to_arr(temp_cwd);
    nob_temp_reset();
    lftd->files = NULL;
    lftd->filtered_files = NULL;

    float_tree->layer_data = lftd;
    return float_tree;
}


void layer_floating_tree_close(CCode* ccode, Layer* layer){
	remove_layer(ccode, layer);
	free_layer(layer);
}

bool layer_floating_tree_load_dir(LayerFloatingTreeData* float_tree_data){
	NOB_ASSERT(float_tree_data->cwd != NULL);
	Nob_File_Paths paths = {0};
    bool status = nob_read_entire_dir(float_tree_data->cwd, &paths);
    if(!status){
        return false;
    }
    if(paths.count > 0){
        qsort(paths.items, paths.count, sizeof(char*), pstrcmp);
    }

    for(size_t i = 0; i < paths.count; i++){
    	if(strlen(paths.items[i]) <= 2 && (strncmp(paths.items[i], ".", 1) == 0 || strncmp(paths.items[i], "..", 2) == 0)){
            continue;
        }
        char* arr = str_to_arr(paths.items[i]);
        if(nob_get_file_type(nob_temp_sprintf("%s/%s", float_tree_data->cwd, arr)) == NOB_FILE_DIRECTORY){
        	arrfree(arr);
        	continue;
        }
		char* copy = str_to_arr(paths.items[i]);
    	arrput(float_tree_data->files, arr);
    	arrput(float_tree_data->filtered_files, copy);
    }
    free(paths.items);
    return true;
}


void layer_floating_tree_load_file(CCode* ccode, LayerFloatingTreeData* float_tree_data){
	if(float_tree_data->filtered_files == NULL){
		return;
	}
	if(arrlenu(float_tree_data->filtered_files) < float_tree_data->selected){
		return;
	}
	char* dir = nob_temp_sprintf("%s/%s",
        float_tree_data->cwd,
        float_tree_data->filtered_files[float_tree_data->selected]);
    char resolved_path[MAX_PATH] = {0};
    if(resolve_path(dir, resolved_path) == NULL){
        return;
    }
	char* arr = str_to_arr(resolved_path);
	read_file_to_code_layer(ccode, arr, arrlen(arr)-1);
	arrfree(arr);
}


bool contains_substr(const char* a, const char* b){
	return strstr(a, b) != NULL;
}


void layer_floating_tree_filter_files(LayerFloatingTreeData* float_tree_data){
	// filter from files to filtered_files based on search_buffer
	for(size_t i = 0; i < arrlenu(float_tree_data->filtered_files); i++){
		arrfree(float_tree_data->filtered_files[i]);
	}
	arrfree(float_tree_data->filtered_files);
	float_tree_data->filtered_files = NULL;
	for(size_t i = 0; i < arrlenu(float_tree_data->files); i++){
		if(contains_substr(float_tree_data->files[i], float_tree_data->search_buffer)){
			char* copy = str_to_arr(float_tree_data->files[i]);
			arrput(float_tree_data->filtered_files, copy);
		}
	}
	if(float_tree_data->filtered_files == NULL){
		size_t zero = 0;
		arrsetlen(float_tree_data->filtered_files, zero);
	}
	float_tree_data->selected = 0;
	float_tree_data->offset = 0;
}


bool layer_floating_tree_update(CCode* ccode, Layer* layer, int chr){
	if(!ccode || !layer || layer->type != LAYER_FLOATING_TREE || layer->layer_data == NULL){
        return false;
    }
    LayerFloatingTreeData* float_tree_data = (LayerFloatingTreeData*) layer->layer_data;
    VirtualWindow* virt_win = float_tree_data->virtual_window;
    if(float_tree_data->files == NULL){
    	if(!layer_floating_tree_load_dir(float_tree_data)){
    		fprintf(stderr, "Floating tree could not load current dir!\n");
    	}
    }

    if((chr >= 0 && chr <= 255) && isprint(chr)){
    	if(arrlen(float_tree_data->search_buffer) >= virt_win->width) return false;
    	int line_len = arrlen(float_tree_data->search_buffer);

        if(float_tree_data->search_buffer_x > line_len - 1){
            float_tree_data->search_buffer_x = line_len - 1;
        }

        if(line_len > 0){
            (void) arrpop(float_tree_data->search_buffer);
        }

        if(float_tree_data->search_buffer_x >= 0 && float_tree_data->search_buffer_x <= arrlen(float_tree_data->search_buffer)){
            arrins(float_tree_data->search_buffer, float_tree_data->search_buffer_x, (char)chr);
        } else {
            arrput(float_tree_data->search_buffer, (char)chr);
        }
        arrput(float_tree_data->search_buffer, '\0');

        if(float_tree_data->search_buffer_x + 1 < virt_win->width){
            float_tree_data->search_buffer_x++;
        }
        layer_floating_tree_filter_files(float_tree_data);
    } else if(chr == CUSTOM_KEY_ENTER){
    	layer_floating_tree_load_file(ccode, float_tree_data);
    	layer_floating_tree_close(ccode, layer);
    	return true;
    } else if(chr == CUSTOM_KEY_BACKSPACE){
        if(float_tree_data->search_buffer_x > 0 && arrlen(float_tree_data->search_buffer) > 1){
            (void) arrpop(float_tree_data->search_buffer);
            arrdel(float_tree_data->search_buffer, float_tree_data->search_buffer_x - 1);
            arrput(float_tree_data->search_buffer, '\0');

            float_tree_data->search_buffer_x--;
        }
        layer_floating_tree_filter_files(float_tree_data);
    } else if(chr == KEY_LEFT){
        if(float_tree_data->search_buffer_x != 0){
            float_tree_data->search_buffer_x--;
        }
    } else if(chr == KEY_RIGHT){
        if(float_tree_data->search_buffer_x < arrlen(float_tree_data->search_buffer)-1){
            float_tree_data->search_buffer_x++;
        }
    }

    int list_height = virt_win->height;

    if(chr == KEY_DOWN){
        if((float_tree_data->selected + 1) < arrlen(float_tree_data->filtered_files)){
            float_tree_data->selected++;
        }
    }

    if(chr == KEY_UP){
        if(float_tree_data->selected > 0){
            float_tree_data->selected--;
        }
    }

    if(float_tree_data->selected < float_tree_data->offset){
        float_tree_data->offset = float_tree_data->selected;
    }

    if(float_tree_data->selected >= float_tree_data->offset + list_height){
        float_tree_data->offset = float_tree_data->selected - list_height;
    }

    return false;
}


void layer_floating_tree_render(CCode* ccode, Layer* layer){
	if(!ccode || !layer || layer->type != LAYER_FLOATING_TREE || layer->layer_data == NULL){
        return;
    }
    LayerFloatingTreeData* float_tree_data = layer->layer_data;
    VirtualWindow* virt_win = float_tree_data->virtual_window;

    attron(COLOR_PAIR(COLOR_PAIR_COMPLETION));
    for(size_t i = 0; i < arrlenu(float_tree_data->search_buffer); i++){
    	if(float_tree_data->search_buffer[i] == '\0') continue;
    	mvaddch(virt_win->y, virt_win->x+i, float_tree_data->search_buffer[i]);
    }
    for(int i = 0; i < virt_win->width - (arrlen(float_tree_data->search_buffer)-1); i++){
    	mvaddch(virt_win->y, virt_win->x + arrlen(float_tree_data->search_buffer) + i -1, ' ');
    }
    int draw_y = 0;
    for(size_t i = float_tree_data->offset; i < arrlenu(float_tree_data->filtered_files); i++){
    	if(draw_y > virt_win->height) break;
    	if(i == float_tree_data->selected){
    		attron(A_REVERSE);
    	}
    	char output[1024] = {0};
    	snprintf(output, 1024, "%s", float_tree_data->filtered_files[i]);
    	size_t width = virt_win->width;
		if (width >= sizeof(output))
		    width = sizeof(output) - 1;
		
		for (size_t j = strlen(output); j < width; j++) {
		    output[j] = ' ';
		}
		output[width] = '\0';
    	if(strlen(output) > virt_win->width){
    		output[virt_win->width] = '\0';
    	}
    	mvprintw(virt_win->y+draw_y+1, virt_win->x, "%s", output);
    	if(i == float_tree_data->selected){
    		attroff(A_REVERSE);
    	}
    	draw_y++;
    }
    attroff(COLOR_PAIR(COLOR_PAIR_COMPLETION));
}


void layer_floating_tree_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    bool redirected = layer_floating_tree_update(ccode, layer, chr);
    if(redirected){
        END_PROFILING("layer_floating_tree");
        return;
    }
    END_PROFILING("layer_floating_tree_update");
    if(should_draw){
        START_PROFILING();
        layer_floating_tree_render(ccode, layer);
        END_PROFILING("layer_floating_tree_render");
    }
    END_PROFILING("layer_floating_tree");
}


#endif