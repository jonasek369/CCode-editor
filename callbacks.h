#ifndef _H_CALLBACKS
#define _H_CALLBACKS

void file_remove_callback(CCode* ccode, void* data){
	char* filename = (char*)data;
	if(!filename) return;
	char* cwd = strdup(nob_get_current_dir_temp());
	char* file_path = nob_temp_sprintf("%s/%s", cwd, filename);

	nob_delete_file(file_path);

	for(size_t i = 0; i < arrlenu(ccode->layers); i++){
		if(ccode->layers[i]->type == LAYER_DIR_WALK) refresh_tree_files(ccode->layers[i]);
	}

	free(cwd);
	free(filename);
	nob_temp_reset();
}

void file_not_remove_callback(CCode* ccode, void* data){
	(void) ccode;
	char* filename = (char*)data;
	if(!filename) return;
	free(filename);
}


void file_on_save_callback(CCode* ccode, void* data){
	if(!data) return;
	LayerCodeData* lcd = ((Layer*)data)->layer_data;

	if(is_lspkind_running(ccode, lang_to_lspkind[lcd->lang])){
        send_to_lsp(ccode, get_running_lsp(ccode, lang_to_lspkind[lcd->lang]));
    }

    char path[4096];
    char config_path[8096];

    get_config_directory(path, sizeof(path));
    snprintf(config_path, sizeof(config_path), "%s/config.json", path);
    if(strncmp(config_path, lcd->filename, strlen(config_path)) == 0){
		if(ccode->config->theme){
            free_theme(ccode->config->theme);
        }
        free(ccode->config);
        ccode->config = load_config(config_path);
        init_syntax_colors(ccode->config->theme);
    }
}

#endif