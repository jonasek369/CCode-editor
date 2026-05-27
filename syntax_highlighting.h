
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

typedef struct {
    int32_t start_row, start_col;
    int32_t end_row,   end_col;
    int     color_pair;
} HSpan;

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

    range->start_character = (int)(shget(start->object, "character")->number);
    range->start_line      = (int)(shget(start->object, "line")->number);
    range->end_character   = (int)(shget(end->object,   "character")->number);
    range->end_line        = (int)(shget(end->object,   "line")->number);
    return range;
}

int get_diagnostic_color(JsonValue* diagnostic) {
    JsonValue* severity = shget(diagnostic->object, "severity");
    if (!severity) {
        printf("Diagnostic does not have severity!\n");
        return -1;
    }
    switch ((int)severity->number) {
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

static int pair_to_attrs(int pair_id){
    (void)pair_id;
    return A_NORMAL;
}

void init_syntax_highlighting(ColorTheme* theme) {
    init_type_map();
    init_syntax_colors(theme);
}

void destroy_syntax_highlighting(void) {
    shfree(type_map);
    type_map = NULL;
}

static int hspan_cmp(const void* a, const void* b) {
    const HSpan* x = (const HSpan*)a;
    const HSpan* y = (const HSpan*)b;
    if (x->start_row != y->start_row) return x->start_row - y->start_row;
    return x->start_col - y->start_col;
}

void syntax_highlighting_render(LayerCodeData* lcd) {
    if (!lcd || !lcd->query || !lcd->tree)
        return;
    START_PROFILING();

    Cursor* cur = lcd->cursor;
    VirtualWindow* virt_win = lcd->virtual_window;

    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    int win_x = virt_win ? virt_win->x      : 0;
    int win_y = virt_win ? virt_win->y      : 0;
    int win_w = virt_win ? virt_win->width  : screen_w;
    int win_h = virt_win ? virt_win->height : screen_h;

    int xoff = cur->xoff;
    int yoff = cur->yoff;

    HSpan* spans     = NULL;

    uint32_t capture_count = ts_query_capture_count(lcd->query);

    TSQueryCursor* ts_cursor = ts_query_cursor_new();
    TSNode root = ts_tree_root_node(lcd->tree);

    ts_query_cursor_set_point_range(ts_cursor,
        (TSPoint){ (uint32_t)yoff,           0 },
        (TSPoint){ (uint32_t)(yoff + win_h), UINT32_MAX });

    ts_query_cursor_exec(ts_cursor, lcd->query, root);
    START_PROFILING();
    TSQueryMatch match;
    while (ts_query_cursor_next_match(ts_cursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; i++) {
            TSQueryCapture capture = match.captures[i];
            if (capture.index >= capture_count) continue;

            uint32_t name_len = 0;
            const char* cap_name = ts_query_capture_name_for_id(
                lcd->query, capture.index, &name_len);

            int cp = name_to_color_pair(cap_name);
            if (cp < 0) {
                printf("unhandled name: %s\n", cap_name);
            };

            TSNode node = capture.node;
            HSpan sp = {
                .start_row  = (int32_t)ts_node_start_point(node).row,
                .start_col  = (int32_t)ts_node_start_point(node).column,
                .end_row    = (int32_t)ts_node_end_point(node).row,
                .end_col    = (int32_t)ts_node_end_point(node).column,
                .color_pair = cp,
            };

            arrput(spans, sp);
        }
    }
    END_PROFILING("collection");
    ts_query_cursor_delete(ts_cursor);
    int span_count = arrlen(spans);
    // not needed for now
    //if (span_count > 0)
    //    qsort(spans, span_count, sizeof(HSpan), hspan_cmp);

    int buffer_lines = (int)arrlenu(lcd->code_buffer);

/* Absolutely-positioned addch: always writes to the correct screen cell
   relative to the virtual window, never drifts into adjacent windows. */
#define MVADDCH(buf_row, buf_col, ch, pair, attrs)                        \
    do {                                                                   \
        int _sx = win_x + ((buf_col) - xoff);                             \
        int _sy = win_y + ((buf_row) - yoff);                             \
        if (_sx >= win_x && _sx < win_x + win_w &&                        \
            _sy >= win_y && _sy < win_y + win_h) {                        \
            attron(COLOR_PAIR(pair) | (attrs));                            \
            mvaddch(_sy, _sx, (ch));                                       \
            attroff(COLOR_PAIR(pair) | (attrs));                           \
        }                                                                  \
    } while (0)
    START_PROFILING();
    for (int sr = 0; sr < win_h; sr++) {
        int br = sr + yoff;

        if (br >= buffer_lines || !lcd->code_buffer[br]) {
            /* Empty line — fill the virtual window columns with default */
            for (int c = xoff; c < xoff + win_w; c++)
                MVADDCH(br, c, ' ', COLOR_PAIR_DEFAULT, A_NORMAL);
            continue;
        }

        const char* line     = lcd->code_buffer[br];
        int         line_len = (int)strlen(line);

        int span_base = 0;
        for (int s = 0; s < span_count; s++) {
            if (spans[s].end_row >= br) { span_base = s; break; }
        }

        int bc = xoff;   /* current buffer column */

        for (int si = span_base; si < span_count; si++) {
            HSpan* sp = &spans[si];
            if (sp->start_row > br) break;
            if (sp->end_row   < br) continue;

            int sp_col_start = (sp->start_row == br) ? sp->start_col : 0;
            int sp_col_end   = (sp->end_row   == br) ? sp->end_col   : line_len;

            /* Clamp span to visible column range */
            if (sp_col_end   <= xoff)          continue;
            if (sp_col_start >= xoff + win_w)  break;

            /* Gap before span */
            for (int c = bc; c < sp_col_start && c < xoff + win_w; c++) {
                chtype ch = c < line_len ? (unsigned char)line[c] : ' ';
                MVADDCH(br, c, ch, COLOR_PAIR_DEFAULT, A_NORMAL);
            }
            if (sp_col_start > bc) bc = sp_col_start;

            /* Span itself */
            attr_t attrs = pair_to_attrs(sp->color_pair);
            for (int c = bc; c < sp_col_end && c < xoff + win_w; c++) {
                chtype ch = c < line_len ? (unsigned char)line[c] : ' ';
                MVADDCH(br, c, ch, sp->color_pair, attrs);
            }
            bc = sp_col_end;
        }

        /* Remainder after all spans */
        for (int c = bc; c < xoff + win_w; c++) {
            chtype ch = c < line_len ? (unsigned char)line[c] : ' ';
            MVADDCH(br, c, ch, COLOR_PAIR_DEFAULT, A_NORMAL);
        }
    }
    END_PROFILING("render");
#undef MVADDCH

    move(win_y + (cur->y - yoff), win_x + (cur->x - xoff));
    END_PROFILING("syntax_highlighting_render");
    arrfree(spans);
}