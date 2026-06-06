#ifndef _H_FLOATING_DIALOG
#define _H_FLOATING_DIALOG

void layer_floating_dialog_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw);
bool layer_floating_dialog_update(CCode* ccode, Layer* layer, int chr);


Layer* new_layer_floating_dialog(const char* message, void (*on_yes)(CCode*, void*), void (*on_no)(CCode*, void*), void* userdata){
    Layer* dialog = malloc(sizeof(Layer));
    if(!dialog){
        return NULL;
    }
    dialog->type = LAYER_FLOATING_DIALOG;
    dialog->consume_input = true;
    dialog->draws_fullscreen = false;
    dialog->handle_keypress_function = &layer_floating_dialog_handle_keypress;
    dialog->update_function = &layer_floating_dialog_update;

    LayerFloatingDialogData* lfd = malloc(sizeof(LayerFloatingDialogData));
    if(!lfd){
        free(dialog);
        return NULL;
    }

    int x, y;
    getmaxyx(stdscr, y, x);

    int msg_len = (int)strlen(message);
    int min_width = 18;
    int width = msg_len + 4;
    if(width < min_width) width = min_width;
    if(width > x)         width = x;

    int height = 5;
    lfd->virtual_window = malloc(sizeof(VirtualWindow));
    if(!lfd->virtual_window){
        free(dialog);
        free(lfd);
        return NULL;
    }
    lfd->virtual_window->x      = (x / 2) - width / 2;
    lfd->virtual_window->y      = (y / 2) - height / 2;
    lfd->virtual_window->width  = width;
    lfd->virtual_window->height = height;

    lfd->message  = str_to_arr(message);
    lfd->selected = 0;    /* 0 = Yes, 1 = No */
    lfd->on_yes   = on_yes;
    lfd->on_no    = on_no;
    lfd->userdata = userdata;

    dialog->layer_data = lfd;
    return dialog;
}


void layer_floating_dialog_close(CCode* ccode, Layer* layer){
    remove_layer(ccode, layer);
    free_layer(layer);
}


bool layer_floating_dialog_update(CCode* ccode, Layer* layer, int chr){
    if(!ccode || !layer || layer->type != LAYER_FLOATING_DIALOG || layer->layer_data == NULL){
        return false;
    }
    LayerFloatingDialogData* lfd = (LayerFloatingDialogData*) layer->layer_data;

    if(chr == KEY_LEFT || chr == KEY_RIGHT){
        lfd->selected = !lfd->selected;
    } else if(chr == 'y' || chr == 'Y'){
        if(lfd->on_yes) lfd->on_yes(ccode, lfd->userdata);
        layer_floating_dialog_close(ccode, layer);
        return true;
    } else if(chr == 'n' || chr == 'N' || chr == CUSTOM_KEY_ESCAPE){
        if(lfd->on_no) lfd->on_no(ccode, lfd->userdata);
        layer_floating_dialog_close(ccode, layer);
        return true;
    } else if(chr == CUSTOM_KEY_ENTER){
        if(lfd->selected == 0){
            if(lfd->on_yes) lfd->on_yes(ccode, lfd->userdata);
        } else {
            if(lfd->on_no)  lfd->on_no(ccode, lfd->userdata);
        }
        layer_floating_dialog_close(ccode, layer);
        return true;
    }

    return false;
}


void layer_floating_dialog_render(CCode* ccode, Layer* layer){
    if(!ccode || !layer || layer->type != LAYER_FLOATING_DIALOG || layer->layer_data == NULL){
        return;
    }
    LayerFloatingDialogData* lfd = (LayerFloatingDialogData*) layer->layer_data;
    VirtualWindow* virt_win = lfd->virtual_window;

    attron(COLOR_PAIR(COLOR_PAIR_COMPLETION));

    for(int j = 0; j < virt_win->width; j++){
        mvaddch(virt_win->y, virt_win->x + j, '-');
    }

    for(int j = 0; j < virt_win->width; j++){
        mvaddch(virt_win->y + 1, virt_win->x + j, ' ');
    }
    {
        int msg_len = (int)strlen(lfd->message);
        int msg_col = virt_win->x + (virt_win->width - msg_len) / 2;
        if(msg_col < virt_win->x) msg_col = virt_win->x;
        mvprintw(virt_win->y + 1, msg_col, "%s", lfd->message);
    }

    for(int j = 0; j < virt_win->width; j++){
        mvaddch(virt_win->y + 2, virt_win->x + j, ' ');
    }

    for(int j = 0; j < virt_win->width; j++){
        mvaddch(virt_win->y + 3, virt_win->x + j, ' ');
    }
    int btn_row_start = virt_win->x + (virt_win->width - 11) / 2;

    if(lfd->selected == 0) attron(A_REVERSE);
    mvprintw(virt_win->y + 3, btn_row_start, "[Y]es");
    if(lfd->selected == 0) attroff(A_REVERSE);

    if(lfd->selected == 1) attron(A_REVERSE);
    mvprintw(virt_win->y + 3, btn_row_start + 7, "[N]o");
    if(lfd->selected == 1) attroff(A_REVERSE);

    for(int j = 0; j < virt_win->width; j++){
        mvaddch(virt_win->y + 4, virt_win->x + j, '-');
    }

    attroff(COLOR_PAIR(COLOR_PAIR_COMPLETION));
}


void layer_floating_dialog_handle_keypress(CCode* ccode, Layer* layer, int chr, bool should_draw){
    START_PROFILING();
    START_PROFILING();
    bool redirected = layer_floating_dialog_update(ccode, layer, chr);
    if(redirected){
        END_PROFILING("layer_floating_dialog");
        return;
    }
    END_PROFILING("layer_floating_dialog_update");
    if(should_draw){
        START_PROFILING();
        layer_floating_dialog_render(ccode, layer);
        END_PROFILING("layer_floating_dialog_render");
    }
    END_PROFILING("layer_floating_dialog");
}


#endif