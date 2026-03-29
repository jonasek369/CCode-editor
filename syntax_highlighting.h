#ifndef _H_SYNTAX_HIGHLIGHTING
#define _H_SYNTAX_HIGHLIGHTING

#define COLOR_GRAY       8
#define COLOR_ORANGE     9
#define COLOR_PURPLE     10
#define COLOR_BROWN_GRAY 11

#define COLOR_PAIR_DEFAULT  1
#define COLOR_PAIR_KEYWORD  2
#define COLOR_PAIR_TYPE     3
#define COLOR_PAIR_STRING   4
#define COLOR_PAIR_NUMBER   5
#define COLOR_PAIR_COMMENT  6
#define COLOR_PAIR_FUNCTION 7
#define COLOR_PAIR_OPERATOR 8
#define COLOR_PAIR_PREPROC  9

#define COLOR_FILE 10
#define COLOR_DIR  11

void init_syntax_colors() {
    start_color();
    if (can_change_color()) {
        init_color(COLOR_BLACK,   100, 100, 100);
        init_color(COLOR_WHITE,   700, 700, 700);
        init_color(COLOR_RED,     800, 120, 120);
        init_color(COLOR_GREEN,   120, 700, 300);
        init_color(COLOR_YELLOW,  800, 700, 300);
        init_color(COLOR_CYAN,    300, 600, 700);
        init_color(COLOR_MAGENTA, 700, 400, 700);
        init_color(COLOR_PURPLE,  500, 400, 700);
        init_color(COLOR_GRAY,    400, 400, 400);
        init_color(COLOR_ORANGE,  850, 500, 150);
    }

    init_pair(COLOR_PAIR_DEFAULT,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_KEYWORD,  COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_PAIR_TYPE,     COLOR_CYAN,    COLOR_BLACK);
    init_pair(COLOR_PAIR_STRING,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(COLOR_PAIR_NUMBER,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(COLOR_PAIR_COMMENT,  COLOR_GRAY,    COLOR_BLACK);
    init_pair(COLOR_PAIR_FUNCTION, COLOR_ORANGE,  COLOR_BLACK);
    init_pair(COLOR_PAIR_OPERATOR, COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_PREPROC,  COLOR_MAGENTA, COLOR_BLACK);

    init_pair(COLOR_DIR,  COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_FILE, COLOR_CYAN,   COLOR_BLACK);
}

// ── attrs ────────────────────────────────────────────────────────────────────

static attr_t ts_color_pair_to_attrs(int pair) {
    if (pair == COLOR_PAIR_KEYWORD)  return A_BOLD;
    if (pair == COLOR_PAIR_FUNCTION) return A_BOLD;
    if (pair == COLOR_PAIR_TYPE)     return A_BOLD;
    return A_NORMAL;
}

// ── HSpan ────────────────────────────────────────────────────────────────────

typedef struct {
    int start_row, start_col;
    int end_row,   end_col;
    int color_pair;
} HSpan;

static void push_span(HSpan** spans, TSNode node, int pair) {
    TSPoint s = ts_node_start_point(node);
    TSPoint e = ts_node_end_point(node);
    HSpan h = { (int)s.row, (int)s.column, (int)e.row, (int)e.column, pair };
    arrpush(*spans, h);
}

// ── helpers ──────────────────────────────────────────────────────────────────

// read the actual source text of a node (single-line nodes only)
static bool node_text_eq(TSNode node, const char* text,
                         char** buf, int buf_size) {
    TSPoint s = ts_node_start_point(node);
    TSPoint e = ts_node_end_point(node);
    if ((int)s.row != (int)e.row)  return false;
    if ((int)s.row >= buf_size)    return false;
    const char* line = buf[(int)s.row];
    if (!line) return false;
    int len = (int)(e.column - s.column);
    return (int)strlen(text) == len &&
           strncmp(line + s.column, text, len) == 0;
}

// ── C highlighter ────────────────────────────────────────────────────────────

typedef struct {
    char *key;
    int value;
} map_entry;

static map_entry* c_type_map = NULL;

static void init_c_type_map(){
    if(c_type_map){
        return;
    }
    shput(c_type_map, "if", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "while", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "do", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "switch", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "break", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "goto", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "typedef", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "enum", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "static", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "const", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "inline", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "else", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "for", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "return", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "case", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "continue", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "sizeof", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "struct", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "union", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "extern", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "volatile", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "default", COLOR_PAIR_KEYWORD);

    shput(c_type_map, "primitive_type", COLOR_PAIR_TYPE);
    shput(c_type_map, "type_qualifier", COLOR_PAIR_TYPE);
    shput(c_type_map, "type_identifier", COLOR_PAIR_TYPE);

    shput(c_type_map, "string_literal", COLOR_PAIR_STRING);
    shput(c_type_map, "char_literal", COLOR_PAIR_STRING);
    shput(c_type_map, "string_content", COLOR_PAIR_STRING);
    shput(c_type_map, "escape_sequence", COLOR_PAIR_STRING);

    shput(c_type_map, "number_literal", COLOR_PAIR_NUMBER);
    shput(c_type_map, "integer_literal", COLOR_PAIR_NUMBER);
    shput(c_type_map, "float_literal", COLOR_PAIR_NUMBER);

    shput(c_type_map, "comment", COLOR_PAIR_COMMENT);
    shput(c_type_map, "line_comment", COLOR_PAIR_COMMENT);
    shput(c_type_map, "block_comment", COLOR_PAIR_COMMENT);

    shput(c_type_map, "preproc_include", COLOR_PAIR_PREPROC);
    shput(c_type_map, "preproc_def", COLOR_PAIR_PREPROC);
    shput(c_type_map, "preproc_directive", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#ifndef", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#if", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#elif", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#define", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#include", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#error", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#ifdef", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#else", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#endif", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#undef", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#pragma", COLOR_PAIR_PREPROC);

    shput(c_type_map, "+", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "*", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "%", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "==", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "&&", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "!", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "|", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "~", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">>", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "-=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "/=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "--", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ".", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ";", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "{", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "(", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "[", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "-", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "/", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "!=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "||", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "&", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "^", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<<", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "+=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "*=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "++", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "->", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ",", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ":", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "}", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ")", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "]", COLOR_PAIR_OPERATOR);
}

static int c_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    int val = shget(c_type_map, type);
    return val ? val : COLOR_PAIR_DEFAULT;
}

static bool c_is_span_container(const char* type) {
    return strcmp(type, "string_literal")  == 0 ||
           strcmp(type, "char_literal")    == 0 ||
           strcmp(type, "comment")         == 0 ||
           strcmp(type, "line_comment")    == 0 ||
           strcmp(type, "block_comment")   == 0 ||
           strcmp(type, "preproc_include") == 0 ||
           strcmp(type, "preproc_def")     == 0;
}

static void collect_highlights_c(TSNode node, HSpan** spans,char** buf, int buf_size) {
    const char* type = ts_node_type(node);

    // function call — color only the name, descend into arguments normally
    if (strcmp(type, "call_expression") == 0) {
        TSNode name = ts_node_child(node, 0);
        if (!ts_node_is_null(name)) {
            const char* nt = ts_node_type(name);
            TSNode ident = name;
            if (strcmp(nt, "field_expression") == 0) {
                uint32_t fc = ts_node_child_count(name);
                if (fc > 0) ident = ts_node_child(name, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size);
        return;
    }

    // function declaration — color only the declared name
    if (strcmp(type, "function_declarator") == 0) {
        TSNode name = ts_node_child(node, 0);
        if (!ts_node_is_null(name))
            push_span(spans, name, COLOR_PAIR_FUNCTION);
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size);
        return;
    }
    // identifier: check for NULL / true / false constants
    if (strcmp(type, "NULL") == 0 ||
        strcmp(type, "nullptr") == 0){
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }else if (strcmp(type, "TRUE")  == 0 ||
              strcmp(type, "FALSE") == 0 ||
              strcmp(type, "true")  == 0 ||
              strcmp(type, "false") == 0){
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }

    int pair = c_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = c_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size);
}

// ── JSON highlighter ─────────────────────────────────────────────────────────

static int json_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    if (strcmp(type, "string")         == 0 ||
        strcmp(type, "string_content") == 0 ||
        strcmp(type, "escape_sequence")== 0)
        return COLOR_PAIR_STRING;

    if (strcmp(type, "number") == 0)
        return COLOR_PAIR_NUMBER;

    if (strcmp(type, "true")  == 0 ||
        strcmp(type, "false") == 0)
        return COLOR_PAIR_KEYWORD;

    if (strcmp(type, "null") == 0)
        return COLOR_PAIR_NUMBER;

    if (strcmp(type, "{") == 0 || strcmp(type, "}") == 0 ||
        strcmp(type, "[") == 0 || strcmp(type, "]") == 0 ||
        strcmp(type, ":") == 0 || strcmp(type, ",") == 0)
        return COLOR_PAIR_OPERATOR;

    return COLOR_PAIR_DEFAULT;
}

static void collect_highlights_json(TSNode node, HSpan** spans) {
    const char* type = ts_node_type(node);

    // object key (first child of "pair") → function/orange so it differs
    // from string values
    if (strcmp(type, "pair") == 0) {
        TSNode key = ts_node_child(node, 0);
        if (!ts_node_is_null(key))
            push_span(spans, key, COLOR_PAIR_FUNCTION);
        // descend remaining children (colon + value) normally
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 1; i < cc; i++)
            collect_highlights_json(ts_node_child(node, i), spans);
        return;
    }

    int pair     = json_node_to_pair(type);
    bool is_leaf = ts_node_child_count(node) == 0;
    // treat "string" as a container so the whole quoted span gets one color
    bool is_container = (strcmp(type, "string") == 0);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_json(ts_node_child(node, i), spans);
}

// ── main render ──────────────────────────────────────────────────────────────


void init_syntax_highlighting(){
    init_c_type_map();
    init_syntax_colors();
}

void apply_tree_sitter_syntax_highlighting(LayerCodeData* lcd, SyntaxLanguage lang) {
    if (!lcd) return;  // only guard against null lcd itself

    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols);
    int content_height = screen_rows - 1;
    int content_width  = screen_cols;

    int buffer_size = arrlen(lcd->code_buffer);
    int yoff        = lcd->cursor->yoff;
    int xoff        = lcd->cursor->xoff;

    // 1. collect spans — skip if no tree available
    HSpan*  spans = NULL;
    if (lcd->parser && lcd->tree) {
        TSNode root = ts_tree_root_node(lcd->tree);
        switch (lang) {
            case LANG_C:
                collect_highlights_c(root, &spans,
                                     lcd->code_buffer, buffer_size);
                break;
            case LANG_JSON:
                collect_highlights_json(root, &spans);
                break;
            case LANG_PYTHON:
                // TODO: Make more
            default:
                break;
        }
    }

    // 2. build colour map
    int** colour_map = calloc(content_height, sizeof(int*));
    for (int r = 0; r < content_height; r++)
        colour_map[r] = calloc(content_width, sizeof(int));

    int span_count = arrlen(spans);
    for (int i = 0; i < span_count; i++) {
        HSpan* sp = &spans[i];
        if (sp->end_row < yoff || sp->start_row >= yoff + content_height)
            continue;

        for (int br = sp->start_row; br <= sp->end_row; br++) {
            int sr = br - yoff;
            if (sr < 0 || sr >= content_height) continue;

            int line_len = (br < buffer_size && lcd->code_buffer[br])
                           ? (int)strlen(lcd->code_buffer[br]) : 0;

            int col_start = (br == sp->start_row) ? sp->start_col : 0;
            int col_end   = (br == sp->end_row)   ? sp->end_col   : line_len;

            int sc_start = col_start - xoff;
            int sc_end   = col_end   - xoff;
            if (sc_end   > content_width) sc_end   = content_width;
            if (sc_start < 0)            sc_start  = 0;

            for (int sc = sc_start; sc < sc_end; sc++)
                colour_map[sr][sc] = sp->color_pair;
        }
    }

    arrfree(spans);

    // 3. render — colour_map is all zeros when no spans, falls back to DEFAULT
    for (int sr = 0; sr < content_height; sr++) {
        int br = sr + yoff;
        move(sr + 1, 0);

        if (br >= buffer_size || !lcd->code_buffer[br]) {
            clrtoeol();
            continue;
        }

        const char* line     = lcd->code_buffer[br];
        int         line_len = (int)strlen(line);
        int         sc       = 0;

        while (sc < content_width) {
            int pair = colour_map[sr][sc];
            if (pair == 0) pair = COLOR_PAIR_DEFAULT;

            attr_t attrs   = ts_color_pair_to_attrs(pair);
            int    run_end = sc + 1;
            while (run_end < content_width && colour_map[sr][run_end] == pair)
                run_end++;

            attron(COLOR_PAIR(pair) | attrs);
            for (int c = sc; c < run_end; c++) {
                int bc = c + xoff;
                addch(bc < line_len ? (unsigned char)line[bc] : ' ');
            }
            attroff(COLOR_PAIR(pair) | attrs);
            sc = run_end;
        }
    }

    // 4. cleanup
    for (int r = 0; r < content_height; r++) free(colour_map[r]);
    free(colour_map);
}

#endif