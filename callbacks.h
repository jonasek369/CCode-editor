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


#endif