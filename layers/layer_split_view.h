#ifndef _H_SPLIT_VIEW
#define _H_SPLIT_VIEW

void layer_split_view_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);
bool layer_split_view_update(CCode* ccode, Layer* layer, int chr);

Layer* new_layer_split_view(){
	Layer* split_view = malloc(sizeof(Layer));
	if(!split_view){
		return NULL;
	}
	split_view->type = LAYER_SPLIT_VIEW;
	split_view->consume_input = true;
    split_view->draws_fullscreen = true;
	split_view->update_function = &layer_split_view_update;
	split_view->handle_keypress_function = &layer_split_view_handle_keypress;

	LayerSplitViewData* lsvd = malloc(sizeof(LayerSplitViewData));
	lsvd->splitten_layers = NULL;
	lsvd->virtual_windows = NULL;
	lsvd->focused = -1;
	split_view->layer_data = lsvd;
	return split_view;
}

bool make_split_view(Layer* split_view, Layer* layer_a, Layer* layer_b){
	assert(split_view);
	assert(layer_a);
	assert(layer_b);
	assert(split_view->type == LAYER_SPLIT_VIEW);
	assert(layer_a->type == LAYER_CODE);
	assert(layer_b->type == LAYER_CODE);
	// TODO: Make this more dynamic and allow more than 2 windows open at once
	LayerSplitViewData* lsvd = split_view->layer_data;
	if(!lsvd){
		return false;
	}
    int y, x;
    getmaxyx(stdscr, y, x);

	int left_w = x / 2;
	int right_w = x - left_w;
	
	VirtualWindow* virt_win_a = malloc(sizeof(VirtualWindow));
	virt_win_a->x = 0;
	virt_win_a->y = 1;
	virt_win_a->width  = left_w;
	virt_win_a->height = y - 1;
	
	VirtualWindow* virt_win_b = malloc(sizeof(VirtualWindow));
	virt_win_b->x = left_w;
	virt_win_b->y = 1;
	virt_win_b->width  = right_w;
	virt_win_b->height = y - 1;

	arrput(lsvd->splitten_layers, layer_a);
	arrput(lsvd->splitten_layers, layer_b);


	arrput(lsvd->virtual_windows, virt_win_a);
	if(layer_a->layer_data) {((LayerCodeData*) layer_a->layer_data)->virtual_window = virt_win_a;}
	arrput(lsvd->virtual_windows, virt_win_b);
	if(layer_b->layer_data) {((LayerCodeData*) layer_b->layer_data)->virtual_window = virt_win_b;}

	lsvd->focused = 0;
	return true;
}

void layer_split_view_close(CCode* ccode, Layer* split_view){
	if(!ccode || !split_view){
		return;
	}
	LayerSplitViewData* lsvd = split_view->layer_data;
	if(!lsvd){
		return;
	}

	if(lsvd->splitten_layers != NULL && arrlenu(lsvd->splitten_layers) > 0){
		for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
			Layer* layer = lsvd->splitten_layers[i];
			if(layer && layer->layer_data != NULL){
				((LayerCodeData*)layer->layer_data)->virtual_window = NULL;
				if(i == lsvd->focused && ccode->config->close_code_layer_on_split_view_close){
					printf("freeing and closing layer\n");
					free_layer(layer);
					continue;
				}
				push_layer_to_top(ccode, layer);
			}
		}
		size_t zero = 0;
		arrsetlen(lsvd->splitten_layers, zero);
	}
	remove_layer(ccode, split_view);
	free_layer(split_view);
}

void layer_split_view_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_SPLIT_VIEW || layer->layer_data == NULL){
        return;
    }
    LayerSplitViewData* lsvd = layer->layer_data;
    if(arrlen(lsvd->splitten_layers) == 0){
        return;
    }

    for(size_t i = 0; i < arrlenu(lsvd->splitten_layers); i++){
    	layer_code_render(ccode, lsvd->splitten_layers[i]);
    }
}

bool layer_split_view_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_SPLIT_VIEW || layer->layer_data == NULL){
        return false;
    }

    LayerSplitViewData* lsvd = layer->layer_data;

    if(arrlen(lsvd->splitten_layers) == 0){
    	return false;
    }

    if (lsvd->focused > arrlen(lsvd->splitten_layers)){
    	lsvd->focused = 0;
    }

    if (chr == CTL_TAB){
    	lsvd->focused = (int)(!lsvd->focused);
    	chr = -1;
    }

    layer_code_update(ccode, lsvd->splitten_layers[lsvd->focused], chr);
    layer_code_update(ccode, lsvd->splitten_layers[(int)(!lsvd->focused)], -1);

	return true;
}

void layer_split_view_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    layer_split_view_update(ccode, layer, chr);
    END_PROFILING("layer_split_view_update");
    if(should_draw){
        START_PROFILING();
        layer_split_view_render(ccode, layer);
        END_PROFILING("layer_split_view_render");
    }
    END_PROFILING("layer_split_view");
}

#endif