// ── Syntax color pairs ────────────────────────────────────────────────────────

#define COLOR_PAIR_DEFAULT   1
#define COLOR_PAIR_KEYWORD   2
#define COLOR_PAIR_TYPE      3
#define COLOR_PAIR_STRING    4
#define COLOR_PAIR_NUMBER    5
#define COLOR_PAIR_COMMENT   6
#define COLOR_PAIR_FUNCTION  7
#define COLOR_PAIR_OPERATOR  8
#define COLOR_PAIR_PREPROC   9

// ── File-browser color pairs ──────────────────────────────────────────────────

#define COLOR_FILE    10
#define COLOR_DIR     11
#define COLOR_SYMLINK 12

// ── Diagnostic color pairs ────────────────────────────────────────────────────

#define COLOR_PAIR_INFORMATION 14
#define COLOR_PAIR_WARNING     15
#define COLOR_PAIR_ERROR       16

// ── Completion color pairs ────────────────────────────────────────────────────

#define COLOR_PAIR_COMPLETION 20

// ── Completion color pairs ────────────────────────────────────────────────────

#define COLOR_PAIR_CONSOLE_TYPE_HINT 30



LSPRange* get_range(JsonValue* diagnostic) {
    LSPRange* range = malloc(sizeof(LSPRange));
    range->start_character = -1;
    range->start_line      = -1;
    range->end_character   = -1;
    range->end_line        = -1;

    JsonValue* range_json = shget(diagnostic->object, "range");
    if (!range_json) {
        printf("Diagnostic does not have range!\n");
        return range;
    }

    JsonValue* start = shget(range_json->object, "start");
    JsonValue* end   = shget(range_json->object, "end");
    if (!start || !end) {
        printf("Diagnostic does not have start or end!\n");
        return range;
    }

    range->start_character = (int)(shget(start->object, "character")->integer);
    range->start_line      = (int)(shget(start->object, "line")->integer);
    range->end_character   = (int)(shget(end->object,   "character")->integer);
    range->end_line        = (int)(shget(end->object,   "line")->integer);
    return range;
}

int get_diagnostic_color(JsonValue* diagnostic) {
    JsonValue* severity = shget(diagnostic->object, "severity");
    if (!severity) {
        printf("Diagnostic does not have severity!\n");
        return -1;
    }
    switch (json_get_integer(severity)) {
        case 1: return COLOR_PAIR_ERROR;
        case 2: return COLOR_PAIR_WARNING;
        case 3: return COLOR_PAIR_INFORMATION;
        default: return -1;
    }
}


int get_pair_id(const char* name) {
    if(name == NULL){ return COLOR_PAIR_DEFAULT;}

    if (strcmp(name, "default") == 0)    return COLOR_PAIR_DEFAULT;
    if (strcmp(name, "keyword") == 0)    return COLOR_PAIR_KEYWORD;
    if (strcmp(name, "type") == 0)       return COLOR_PAIR_TYPE;
    if (strcmp(name, "string") == 0)     return COLOR_PAIR_STRING;
    if (strcmp(name, "number") == 0)     return COLOR_PAIR_NUMBER;
    if (strcmp(name, "comment") == 0)    return COLOR_PAIR_COMMENT;
    if (strcmp(name, "function") == 0)   return COLOR_PAIR_FUNCTION;
    if (strcmp(name, "operator") == 0)   return COLOR_PAIR_OPERATOR;
    if (strcmp(name, "preproc") == 0)    return COLOR_PAIR_PREPROC;
    // File browser
    if (strcmp(name, "dir") == 0)        return COLOR_DIR;
    if (strcmp(name, "file") == 0)       return COLOR_FILE;
    if (strcmp(name, "symlink") == 0)    return COLOR_SYMLINK;
    // Diagnostics
    if (strcmp(name, "info") == 0)       return COLOR_PAIR_INFORMATION;
    if (strcmp(name, "warning") == 0)    return COLOR_PAIR_WARNING;
    if (strcmp(name, "error") == 0)      return COLOR_PAIR_ERROR;
    // Completion
    if (strcmp(name, "completion") == 0) return COLOR_PAIR_COMPLETION;
    return -1;
}


Color* get_color_from_key(ColorTheme* theme, const char* key){
    if(theme == NULL || key == NULL){
        return NULL;
    }
    for (size_t i = 0; i < arrlenu(theme->colors); i++) {
        Color* e = theme->colors[i];
        if(strcmp(e->name, key) == 0) return e;
    }
    return NULL;
}


void init_syntax_colors(ColorTheme* theme) {
    start_color();

    if (can_change_color()) {
        uint16_t r, g, b;
        unpack_rgb16(theme->background->color, &r, &g, &b);
        init_color(COLOR_BLACK, r, g, b);

        for (size_t i = 0; i < arrlenu(theme->colors); i++) {
            Color* e = theme->colors[i];
            uint16_t r, g, b;
            unpack_rgb16(e->color, &r, &g, &b);
            init_color(e->id, r, g, b);
        }
    }

    for (size_t i = 0; i < arrlenu(theme->pairs); i++) {
        ColorPair* p = theme->pairs[i];
        Color* fg = get_color_from_key(theme, p->foreground_key);
        Color* bg = get_color_from_key(theme, p->background_key);
        int pair_id = get_pair_id(p->name);
        if (pair_id == -1) continue;
        if (fg == NULL || bg == NULL) { init_pair((short)pair_id, 1, 0); }
        else{init_pair((short)pair_id, fg->id, bg->id);}
    }

    // Blank cells (erase, clrtoeol, the '\n' at line ends) use the window background.
    // Left at pair 0 ncurses assumes they're already on screen after its initial clear,
    // never sends them, and the terminal shows its own unthemed background there
    bkgd(COLOR_PAIR(COLOR_PAIR_DEFAULT));
}


typedef struct { char* key; int value; } map_entry;


static map_entry* type_map = NULL;


void init_type_map(){
    // --- existing ---
    shput(type_map, "keyword",   COLOR_PAIR_KEYWORD);
    shput(type_map, "operator",  COLOR_PAIR_OPERATOR);
    shput(type_map, "function",  COLOR_PAIR_FUNCTION);
    shput(type_map, "boolean",   COLOR_PAIR_KEYWORD);
    shput(type_map, "number",    COLOR_PAIR_NUMBER);
    shput(type_map, "type",      COLOR_PAIR_TYPE);

    // --- keyword family ---
    shput(type_map, "keyword.type",                  COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.operator",              COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.return",                COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.repeat",                COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.conditional",           COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.conditional.ternary",   COLOR_PAIR_KEYWORD);
    shput(type_map, "keyword.directive",             COLOR_PAIR_PREPROC);
    shput(type_map, "keyword.directive.define",      COLOR_PAIR_PREPROC);
    shput(type_map, "keyword.import",                COLOR_PAIR_PREPROC);
    shput(type_map, "keyword.modifier",              COLOR_PAIR_KEYWORD);

    // --- type family ---
    shput(type_map, "type.builtin",    COLOR_PAIR_TYPE);
    shput(type_map, "type.definition", COLOR_PAIR_TYPE);

    // --- string / character ---
    shput(type_map, "string",          COLOR_PAIR_STRING);
    shput(type_map, "string.escape",   COLOR_PAIR_STRING);
    shput(type_map, "character",       COLOR_PAIR_STRING);

    // --- number ---
    shput(type_map, "number.float",    COLOR_PAIR_NUMBER);  // not in SCM but conventional

    // --- function family ---
    shput(type_map, "function.call",    COLOR_PAIR_FUNCTION);
    shput(type_map, "function.macro",   COLOR_PAIR_PREPROC);
    shput(type_map, "function.builtin", COLOR_PAIR_FUNCTION);

    // --- comment ---
    shput(type_map, "comment",               COLOR_PAIR_COMMENT);
    shput(type_map, "comment.documentation", COLOR_PAIR_COMMENT);

    // --- constants (preproc macros / builtins) ---
    shput(type_map, "constant",         COLOR_PAIR_PREPROC);
    shput(type_map, "constant.builtin", COLOR_PAIR_PREPROC);
    shput(type_map, "constant.macro",   COLOR_PAIR_PREPROC);

    // --- punctuation / attribute / misc → default ---
    shput(type_map, "punctuation.delimiter", COLOR_PAIR_DEFAULT);
    shput(type_map, "punctuation.bracket",   COLOR_PAIR_DEFAULT);
    shput(type_map, "punctuation.special",   COLOR_PAIR_DEFAULT);
    shput(type_map, "variable",              COLOR_PAIR_DEFAULT);
    shput(type_map, "variable.parameter",    COLOR_PAIR_DEFAULT);
    shput(type_map, "variable.builtin",      COLOR_PAIR_DEFAULT);
    shput(type_map, "property",              COLOR_PAIR_DEFAULT);
    shput(type_map, "label",                 COLOR_PAIR_DEFAULT);
    shput(type_map, "attribute",             COLOR_PAIR_DEFAULT);

    //  --- python specific ---
    shput(type_map, "constructor",     COLOR_PAIR_FUNCTION);
    shput(type_map, "embedded",        COLOR_PAIR_PREPROC);
    shput(type_map, "function.method", COLOR_PAIR_FUNCTION);
    shput(type_map, "escape",          COLOR_PAIR_DEFAULT);
}


int name_to_color_pair(const char* name){
    if (!name) return COLOR_PAIR_DEFAULT;
    int val = shget(type_map, name);
    return val ? val : COLOR_PAIR_DEFAULT;
}


void init_syntax_highlighting(ColorTheme* theme) {
    init_type_map();
    init_syntax_colors(theme);
}

void destroy_syntax_highlighting(void) {
    shfree(type_map);
    type_map = NULL;
}


