#ifndef _H_CONFIG
#define _H_CONFIG


typedef struct {
    short       id;   // ncurses color slot
    char* name;
    uint64_t    color;
} Color;


typedef struct {
	char* name;
	char* foreground_key;
	char* background_key;
} ColorPair;

typedef struct {
    char* path;
    Color* background;
	Color** colors;
	ColorPair** pairs;
} ColorTheme;





Color* make_color(short id, const char* name, uint64_t color) {
    Color* c = malloc(sizeof(Color));
    c->id = id;
    if(name){
        c->name = strdup(name);
    }else{
        c->name = NULL;
    }
    c->color = color;
    return c;
}

ColorPair* make_pair(const char* name, const char* fg, const char* bg) {
    ColorPair* p = malloc(sizeof(ColorPair));
    p->name = strdup(name);
    p->foreground_key = strdup(fg);
    p->background_key = strdup(bg);
    return p;
}

uint64_t pack_rgb16(uint16_t r, uint16_t g, uint16_t b) {
    return ((uint64_t)r << 32) |
           ((uint64_t)g << 16) |
           (uint64_t)b;
}

void unpack_rgb16(uint64_t packed, uint16_t *r, uint16_t *g, uint16_t *b) {
    *r = (packed >> 32) & 0xFFFFULL;
    *g = (packed >> 16) & 0xFFFFULL;
    *b = packed & 0xFFFFULL;
}

const char *default_theme_json =
"{\n"
"    \"background\": {\"r\": 130, \"g\": 110, \"b\": 90},\n"
"    \"colors\": [\n"
"        { \"name\": \"black\",      \"r\": 130, \"g\": 110, \"b\": 90  },\n"
"        { \"name\": \"white\",      \"r\": 800, \"g\": 780, \"b\": 700 },\n"
"        { \"name\": \"red\",        \"r\": 950, \"g\": 100, \"b\": 100 },\n"
"        { \"name\": \"green\",      \"r\": 100, \"g\": 850, \"b\": 200 },\n"
"        { \"name\": \"yellow\",     \"r\": 950, \"g\": 800, \"b\": 50  },\n"
"        { \"name\": \"cyan\",       \"r\": 50,  \"g\": 750, \"b\": 750 },\n"
"        { \"name\": \"magenta\",    \"r\": 900, \"g\": 100, \"b\": 600 },\n"
"        { \"name\": \"purple\",     \"r\": 550, \"g\": 150, \"b\": 950 },\n"
"        { \"name\": \"gray\",       \"r\": 350, \"g\": 330, \"b\": 300 },\n"
"        { \"name\": \"orange\",     \"r\": 980, \"g\": 500, \"b\": 50  },\n"
"        { \"name\": \"full_black\", \"r\": 0,   \"g\": 0,   \"b\": 0   }\n"
"    ],\n"
"    \"pairs\": [\n"
"        { \"name\": \"default\",    \"fg\": \"white\",      \"bg\": \"black\" },\n"
"        { \"name\": \"keyword\",    \"fg\": \"magenta\",    \"bg\": \"black\" },\n"
"        { \"name\": \"type\",       \"fg\": \"cyan\",       \"bg\": \"black\" },\n"
"        { \"name\": \"string\",     \"fg\": \"green\",      \"bg\": \"black\" },\n"
"        { \"name\": \"number\",     \"fg\": \"orange\",     \"bg\": \"black\" },\n"
"        { \"name\": \"comment\",    \"fg\": \"gray\",       \"bg\": \"black\" },\n"
"        { \"name\": \"function\",   \"fg\": \"yellow\",     \"bg\": \"black\" },\n"
"        { \"name\": \"operator\",   \"fg\": \"white\",      \"bg\": \"black\" },\n"
"        { \"name\": \"preproc\",    \"fg\": \"purple\",     \"bg\": \"black\" },\n"
"        { \"name\": \"dir\",        \"fg\": \"yellow\",     \"bg\": \"black\" },\n"
"        { \"name\": \"file\",       \"fg\": \"cyan\",       \"bg\": \"black\" },\n"
"        { \"name\": \"symlink\",    \"fg\": \"red\",        \"bg\": \"black\" },\n"
"        { \"name\": \"info\",       \"fg\": \"cyan\",       \"bg\": \"black\" },\n"
"        { \"name\": \"warning\",    \"fg\": \"orange\",     \"bg\": \"black\" },\n"
"        { \"name\": \"error\",      \"fg\": \"red\",        \"bg\": \"black\" },\n"
"        { \"name\": \"completion\", \"fg\": \"full_black\", \"bg\": \"white\" }\n"
"    ]\n"
"}";

typedef struct {
    bool PrivateRunning;
    bool PrivateCloseConsole;
    bool profiling;
    ColorTheme* theme;
    size_t tab_size;
} CCodeConfig;

ColorTheme* load_theme(const char* path){
    JsonValue* json_theme = malloc(sizeof(JsonValue));
    if(!json_theme){
        return NULL;
    }
    if(!path){
        jsonStringLoad((char*)default_theme_json, json_theme);
    }else{
        jsonFileLoad(path, json_theme);
    }

    if(json_theme->type != JSON_OBJECT){
        return NULL;
    }

    JsonValue* colors = shget(json_theme->object, "colors");
    JsonValue* pairs = shget(json_theme->object, "pairs");
    JsonValue* background = shget(json_theme->object, "background");
    if(!colors || !pairs || !background){
        return NULL;
    }

    ColorTheme* theme = malloc(sizeof(ColorTheme));
    theme->colors = NULL;
    theme->pairs  = NULL;
    for(size_t i = 0; i < arrlenu(colors->array); i++){
        JsonValue* color = colors->array[i];
        char* name = shget(color->object, "name")->string;
        uint16_t r = (uint16_t)shget(color->object, "r")->number;
        uint16_t g = (uint16_t)shget(color->object, "g")->number;
        uint16_t b = (uint16_t)shget(color->object, "b")->number;
        arrput(theme->colors, make_color(64+i, name, pack_rgb16(r, g, b)));
    }

    for(size_t i = 0; i < arrlenu(pairs->array); i++){
        JsonValue* pair = pairs->array[i];
        char* name = shget(pair->object, "name")->string;
        char* fg = shget(pair->object, "fg")->string;
        char* bg = shget(pair->object, "bg")->string;
        arrput(theme->pairs, make_pair(name, fg, bg));
    }

    uint16_t r = (uint16_t)shget(background->object, "r")->number;
    uint16_t g = (uint16_t)shget(background->object, "g")->number;
    uint16_t b = (uint16_t)shget(background->object, "b")->number;
    theme->background = make_color(-1, NULL, pack_rgb16(r, g, b));
    if(json_theme){
        json_free(json_theme);
    }
    theme->path = strdup(path);
    return theme;
}


CCodeConfig* load_config(const char* path){
    if(!path){
        return NULL;
    }
    CCodeConfig* config = malloc(sizeof(CCodeConfig));
    config->PrivateRunning = true;
    config->PrivateCloseConsole = false;
    JsonValue* json_config = malloc(sizeof(JsonValue));
    if(!json_config){
        return NULL;
    }
    jsonFileLoad(path, json_config);
    if(json_config->type != JSON_OBJECT){
        free(json_config);
        return NULL;
    }
    JsonValue* profiling = shget(json_config->object, "profiling");
    if(!profiling || profiling->type != JSON_BOOL){
        config->profiling = false; // defualt
    }else{
        config->profiling = profiling->boolean;
    }
    JsonValue* theme = shget(json_config->object, "theme");
    if(!theme || theme->type != JSON_STRING){
        config->theme = load_theme(NULL); // defualt
    }else{
        config->theme = load_theme(theme->string);
        if(config->theme == NULL){
            config->theme = load_theme(NULL);
        }
    }
    JsonValue* tab_size = shget(json_config->object, "tab_size");
    if(!tab_size || tab_size->type != JSON_NUMBER){
        config->tab_size = 4; // defualt
    }else{
        config->tab_size = (size_t)tab_size->number;
    }
    json_free(json_config);
    return config;
}

void save_config(CCodeConfig* config){
    if(!config)
        return;
    JsonValue* json_config = json_new_object();
    json_add_child(json_config, "theme", json_new_string(config->theme->path));
    json_add_child(json_config, "profiling", json_new_bool(config->profiling));
    json_add_child(json_config, "tab_size", json_new_number(config->tab_size));
    char* out = NULL;
    json_dump(json_config, &out);
    arrput(out, NULL);
    bool saved = nob_write_entire_file("config.json", out, arrlen(out)-1);
    if(!saved){
        printf("failed to save config!\n");
    }
    arrfree(out);
    json_free(json_config);
}

CCodeConfig* make_default_config(){
	CCodeConfig* conf = malloc(sizeof(CCodeConfig));
	if(!conf){
		return NULL;
	}
	conf->PrivateRunning = true;
	conf->PrivateCloseConsole = false;
	conf->profiling = false;
    conf->theme = load_theme(NULL);
    conf->tab_size = 4;
	return conf;
}


void free_color(Color* color){
    if (color->name) {
        free(color->name);
    }
    free(color);
}

void free_color_pair(ColorPair* pair){
    if (pair->name) {
        free(pair->name);
    }
    if (pair->foreground_key) {
        free(pair->foreground_key);
    }
    if (pair->background_key) {
        free(pair->background_key);
    }
    free(pair);
}

void free_theme(ColorTheme* theme){
    if(theme->path)
        free(theme->path);
    if(theme->background)
        free_color(theme->background);
    if(theme->pairs){
        for (size_t i = 0; i < arrlenu(theme->pairs); i++) {
            free_color_pair(theme->pairs[i]);
        }
        arrfree(theme->pairs);
    }
    if(theme->colors){
        for (size_t i = 0; i < arrlenu(theme->colors); i++) {
            free_color(theme->colors[i]);
        }
        arrfree(theme->colors);
    }
    free(theme);
}



#endif