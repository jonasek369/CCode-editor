#ifndef _H_SYNTAX_HIGHLIGHTING
#define _H_SYNTAX_HIGHLIGHTING

#define COLOR_GRAY       8
#define COLOR_ORANGE     9
#define COLOR_PURPLE     10
#define COLOR_BROWN_GRAY 11
#define COLOR_FULL_BLACK 12

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
#define COLOR_SYMLINK  12

#define COLOR_PAIR_INFORMATION 14
#define COLOR_PAIR_WARNING     15
#define COLOR_PAIR_ERROR       16

#define COLOR_PAIR_COMPLETION 20

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
        init_color(COLOR_FULL_BLACK, 0, 0, 0);
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
    init_pair(COLOR_SYMLINK, COLOR_RED,   COLOR_BLACK);

    init_pair(COLOR_PAIR_INFORMATION, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_PAIR_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);

    init_pair(COLOR_PAIR_COMPLETION, COLOR_WHITE, COLOR_FULL_BLACK);
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

/* static bool node_text_eq(TSNode node, const char* text,
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
}*/

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

static void collect_highlights_c(TSNode node, HSpan** spans, char** buf, int buf_size) {
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
// ── Python highlighter ────────────────────────────────────────────────────────

static int py_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    if (strcmp(type, "string")          == 0 ||
        strcmp(type, "string_content")  == 0 ||
        strcmp(type, "string_start")    == 0 ||
        strcmp(type, "string_end")      == 0 ||
        strcmp(type, "escape_sequence") == 0 ||
        strcmp(type, "interpolation")   == 0)
        return COLOR_PAIR_STRING;

    if (strcmp(type, "integer")         == 0 ||
        strcmp(type, "float")           == 0 ||
        strcmp(type, "complex")         == 0)
        return COLOR_PAIR_NUMBER;

    if (strcmp(type, "comment") == 0)
        return COLOR_PAIR_COMMENT;

    if (strcmp(type, "def")       == 0 ||
        strcmp(type, "class")     == 0 ||
        strcmp(type, "return")    == 0 ||
        strcmp(type, "if")        == 0 ||
        strcmp(type, "elif")      == 0 ||
        strcmp(type, "else")      == 0 ||
        strcmp(type, "for")       == 0 ||
        strcmp(type, "while")     == 0 ||
        strcmp(type, "in")        == 0 ||
        strcmp(type, "not")       == 0 ||
        strcmp(type, "and")       == 0 ||
        strcmp(type, "or")        == 0 ||
        strcmp(type, "is")        == 0 ||
        strcmp(type, "import")    == 0 ||
        strcmp(type, "from")      == 0 ||
        strcmp(type, "as")        == 0 ||
        strcmp(type, "with")      == 0 ||
        strcmp(type, "lambda")    == 0 ||
        strcmp(type, "yield")     == 0 ||
        strcmp(type, "pass")      == 0 ||
        strcmp(type, "break")     == 0 ||
        strcmp(type, "continue")  == 0 ||
        strcmp(type, "raise")     == 0 ||
        strcmp(type, "try")       == 0 ||
        strcmp(type, "except")    == 0 ||
        strcmp(type, "finally")   == 0 ||
        strcmp(type, "global")    == 0 ||
        strcmp(type, "nonlocal")  == 0 ||
        strcmp(type, "del")       == 0 ||
        strcmp(type, "assert")    == 0 ||
        strcmp(type, "async")     == 0 ||
        strcmp(type, "await")     == 0)
        return COLOR_PAIR_KEYWORD;

    return COLOR_PAIR_DEFAULT;
}

static bool py_is_span_container(const char* type) {
    return strcmp(type, "string")        == 0 ||
           strcmp(type, "comment")       == 0 ||
           strcmp(type, "concatenated_string") == 0;
}

static void collect_highlights_python(TSNode node, HSpan** spans, char** buf, int buf_size) {
    const char* type = ts_node_type(node);

    // function/method call — color only the name, descend normally
    if (strcmp(type, "call") == 0) {
        TSNode func = ts_node_child(node, 0);
        if (!ts_node_is_null(func)) {
            const char* ft = ts_node_type(func);
            TSNode ident = func;
            // obj.method(...) — highlight only the attribute (method name)
            if (strcmp(ft, "attribute") == 0) {
                uint32_t fc = ts_node_child_count(func);
                if (fc > 0) ident = ts_node_child(func, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size);
        return;
    }

    // function/class definition — color only the declared name
    if (strcmp(type, "function_definition") == 0 ||
        strcmp(type, "class_definition")    == 0) {
        // child 0 = "def"/"class" keyword, child 1 = name identifier
        if (ts_node_child_count(node) > 1) {
            TSNode name = ts_node_child(node, 1);
            if (!ts_node_is_null(name))
                push_span(spans, name, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size);
        return;
    }

    // boolean / None constants
    if (strcmp(type, "true")      == 0 ||
        strcmp(type, "false")     == 0 ||
        strcmp(type, "True")      == 0 ||
        strcmp(type, "False")     == 0)  {
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }
    if (strcmp(type, "none")      == 0 ||
        strcmp(type, "None")      == 0)  {
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }

    // decorator — color the '@' plus the name as a unit
    if (strcmp(type, "decorator") == 0) {
        push_span(spans, node, COLOR_PAIR_FUNCTION);
        return;
    }

    int pair          = py_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = py_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size);
}

// ── C# highlighter ───────────────────────────────────────────────────────────

static int cs_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    // Strings
    if (strcmp(type, "string_literal")          == 0 ||
        strcmp(type, "verbatim_string_literal")  == 0 ||
        strcmp(type, "interpolated_string_text") == 0 ||
        strcmp(type, "string_literal_fragment")  == 0 ||
        strcmp(type, "character_literal")        == 0 ||
        strcmp(type, "escape_sequence")          == 0 ||
        strcmp(type, "raw_string_literal")       == 0)
        return COLOR_PAIR_STRING;

    // Numbers
    if (strcmp(type, "integer_literal")          == 0 ||
        strcmp(type, "real_literal")             == 0 ||
        strcmp(type, "hex_integer_literal")      == 0 ||
        strcmp(type, "binary_integer_literal")   == 0)
        return COLOR_PAIR_NUMBER;

    // Comments
    if (strcmp(type, "comment")                  == 0 ||
        strcmp(type, "single_line_comment")      == 0 ||
        strcmp(type, "multiline_comment")        == 0)
        return COLOR_PAIR_COMMENT;

    // Operators and punctuation
    if (strcmp(type, "{")  == 0 || strcmp(type, "}")  == 0 ||
        strcmp(type, "(")  == 0 || strcmp(type, ")")  == 0 ||
        strcmp(type, "[")  == 0 || strcmp(type, "]")  == 0 ||
        strcmp(type, ";")  == 0 || strcmp(type, ",")  == 0 ||
        strcmp(type, ".")  == 0 || strcmp(type, "=>") == 0 ||
        strcmp(type, "=")  == 0 || strcmp(type, "+=") == 0 ||
        strcmp(type, "-=") == 0 || strcmp(type, "*=") == 0 ||
        strcmp(type, "/=") == 0 || strcmp(type, "??") == 0 ||
        strcmp(type, "?.") == 0 || strcmp(type, "::") == 0)
        return COLOR_PAIR_OPERATOR;

    // Keywords
    if (strcmp(type, "abstract")   == 0 ||
        strcmp(type, "as")         == 0 ||
        strcmp(type, "async")      == 0 ||
        strcmp(type, "await")      == 0 ||
        strcmp(type, "base")       == 0 ||
        strcmp(type, "break")      == 0 ||
        strcmp(type, "case")       == 0 ||
        strcmp(type, "catch")      == 0 ||
        strcmp(type, "checked")    == 0 ||
        strcmp(type, "class")      == 0 ||
        strcmp(type, "const")      == 0 ||
        strcmp(type, "continue")   == 0 ||
        strcmp(type, "default")    == 0 ||
        strcmp(type, "delegate")   == 0 ||
        strcmp(type, "do")         == 0 ||
        strcmp(type, "else")       == 0 ||
        strcmp(type, "enum")       == 0 ||
        strcmp(type, "event")      == 0 ||
        strcmp(type, "explicit")   == 0 ||
        strcmp(type, "extern")     == 0 ||
        strcmp(type, "finally")    == 0 ||
        strcmp(type, "fixed")      == 0 ||
        strcmp(type, "for")        == 0 ||
        strcmp(type, "foreach")    == 0 ||
        strcmp(type, "goto")       == 0 ||
        strcmp(type, "if")         == 0 ||
        strcmp(type, "implicit")   == 0 ||
        strcmp(type, "in")         == 0 ||
        strcmp(type, "interface")  == 0 ||
        strcmp(type, "internal")   == 0 ||
        strcmp(type, "is")         == 0 ||
        strcmp(type, "lock")       == 0 ||
        strcmp(type, "namespace")  == 0 ||
        strcmp(type, "new")        == 0 ||
        strcmp(type, "operator")   == 0 ||
        strcmp(type, "out")        == 0 ||
        strcmp(type, "override")   == 0 ||
        strcmp(type, "params")     == 0 ||
        strcmp(type, "private")    == 0 ||
        strcmp(type, "protected")  == 0 ||
        strcmp(type, "public")     == 0 ||
        strcmp(type, "readonly")   == 0 ||
        strcmp(type, "record")     == 0 ||
        strcmp(type, "ref")        == 0 ||
        strcmp(type, "return")     == 0 ||
        strcmp(type, "sealed")     == 0 ||
        strcmp(type, "sizeof")     == 0 ||
        strcmp(type, "stackalloc") == 0 ||
        strcmp(type, "static")     == 0 ||
        strcmp(type, "struct")     == 0 ||
        strcmp(type, "switch")     == 0 ||
        strcmp(type, "this")       == 0 ||
        strcmp(type, "throw")      == 0 ||
        strcmp(type, "try")        == 0 ||
        strcmp(type, "typeof")     == 0 ||
        strcmp(type, "unchecked")  == 0 ||
        strcmp(type, "unsafe")     == 0 ||
        strcmp(type, "using")      == 0 ||
        strcmp(type, "virtual")    == 0 ||
        strcmp(type, "void")       == 0 ||
        strcmp(type, "volatile")   == 0 ||
        strcmp(type, "where")      == 0 ||
        strcmp(type, "while")      == 0 ||
        strcmp(type, "with")       == 0 ||
        strcmp(type, "yield")      == 0 || 
        strcmp(type, "var")        == 0
        )
        return COLOR_PAIR_KEYWORD;

    return COLOR_PAIR_DEFAULT;
}

static bool cs_is_span_container(const char* type) {
    return strcmp(type, "string_literal")              == 0 ||
           strcmp(type, "verbatim_string_literal")     == 0 ||
           strcmp(type, "raw_string_literal")          == 0 ||
           strcmp(type, "interpolated_string_expression") == 0 ||
           strcmp(type, "comment")                     == 0 ||
           strcmp(type, "multiline_comment")           == 0;
}

static void collect_highlights_csharp(TSNode node, HSpan** spans) {
    const char* type = ts_node_type(node);

    // Method / constructor / local-function invocation — highlight name only
    if (strcmp(type, "invocation_expression") == 0) {
        TSNode func = ts_node_child(node, 0);
        if (!ts_node_is_null(func)) {
            const char* ft = ts_node_type(func);
            TSNode ident = func;
            // obj.Method(...) — highlight only the right-hand name
            if (strcmp(ft, "member_access_expression") == 0) {
                uint32_t fc = ts_node_child_count(func);
                if (fc > 0) ident = ts_node_child(func, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans);
        return;
    }

    // Method / constructor / local-function definition — highlight declared name
    if (strcmp(type, "method_declaration")            == 0 ||
        strcmp(type, "constructor_declaration")       == 0 ||
        strcmp(type, "local_function_statement")      == 0 ||
        strcmp(type, "operator_declaration")          == 0 ||
        strcmp(type, "conversion_operator_declaration") == 0) {
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++) {
            TSNode child = ts_node_child(node, i);
            const char* ct = ts_node_type(child);
            if (strcmp(ct, "identifier") == 0) {
                push_span(spans, child, COLOR_PAIR_FUNCTION);
                break;
            }
        }
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans);
        return;
    }

    // Class / struct / interface / enum / record declaration — highlight name
    if (strcmp(type, "class_declaration")     == 0 ||
        strcmp(type, "struct_declaration")    == 0 ||
        strcmp(type, "interface_declaration") == 0 ||
        strcmp(type, "enum_declaration")      == 0 ||
        strcmp(type, "record_declaration")    == 0 ||
        strcmp(type, "record_struct_declaration") == 0) {
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++) {
            TSNode child = ts_node_child(node, i);
            if (strcmp(ts_node_type(child), "identifier") == 0) {
                push_span(spans, child, COLOR_PAIR_FUNCTION);
                break;
            }
        }
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans);
        return;
    }

    // Attributes (e.g. [Serializable], [HttpGet]) — highlight as function/orange
    if (strcmp(type, "attribute") == 0) {
        push_span(spans, node, COLOR_PAIR_FUNCTION);
        return;
    }

    // Boolean / null literals
    if (strcmp(type, "true")  == 0 ||
        strcmp(type, "false") == 0) {
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }
    if (strcmp(type, "null")    == 0 ||
        strcmp(type, "default") == 0) {
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }

    // Preprocessor directives — treat as comments for visual grouping
    if (strcmp(type, "preprocessor_directive") == 0 ||
        strcmp(type, "if_directive")           == 0 ||
        strcmp(type, "elif_directive")         == 0 ||
        strcmp(type, "else_directive")         == 0 ||
        strcmp(type, "endif_directive")        == 0 ||
        strcmp(type, "define_directive")       == 0 ||
        strcmp(type, "undef_directive")        == 0 ||
        strcmp(type, "region_directive")       == 0 ||
        strcmp(type, "endregion_directive")    == 0 ||
        strcmp(type, "pragma_directive")       == 0 ||
        strcmp(type, "nullable_directive")     == 0) {
        push_span(spans, node, COLOR_PAIR_COMMENT);
        return;
    }

    int pair          = cs_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = cs_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_csharp(ts_node_child(node, i), spans);
}

// ── main render ──────────────────────────────────────────────────────────────


void init_syntax_highlighting(){
    init_c_type_map();
    init_syntax_colors();
}

void destroy_syntax_highlihting(){
    shfree(c_type_map);
}

typedef struct {
    int start_character;
    int start_line;

    int end_character;
    int end_line;
} LSPRange;

LSPRange get_range(JsonValue* diagnostic){
    LSPRange range = {0};
    range.start_character = -1;
    range.start_line = -1;

    range.end_character = -1;
    range.end_line = -1;

    JsonValue* range_json = shget(diagnostic->object, "range");
    if(!range_json){
        printf("Diagnostic does not have range!\n");
        return range;
    }
    JsonValue* start = shget(range_json->object, "start");
    JsonValue* end = shget(range_json->object, "end");
    if(!start || !end){
        printf("Diagnostic does not have start or end!\n");
        return range;
    }
    range.start_character = (int)(shget(start->object, "character")->number);
    range.start_line = (int)(shget(start->object, "line")->number);
    // seems like its offseted ?
    range.end_character = ((int)(shget(end->object, "character")->number)) - 1;
    range.end_line = (int)(shget(end->object, "line")->number);

    return range;
}

int get_diagnostic_color(JsonValue* diagnostic){
    JsonValue* severity = shget(diagnostic->object, "severity");
    if(!severity){
        printf("Diagnostic does not have severity!\n");
        return -1;
    }
    int severity_int = (int)severity->number;
    switch(severity_int){
        case(1): {
            return COLOR_PAIR_ERROR;
        }
        case(2): {
            return COLOR_PAIR_WARNING;
        }
        case(3): {
            return COLOR_PAIR_INFORMATION;;
        }
        case(4): {
            return -1;
        }
    }
    return -1;
}

void apply_tree_sitter_syntax_highlighting(LayerCodeData* lcd, SyntaxLanguage lang) {
    if (!lcd) return;

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
                collect_highlights_python(root, &spans, lcd->code_buffer, buffer_size);
                break;
            case LANG_C_SHARP:
                collect_highlights_csharp(root, &spans);
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
        const char* line = lcd->code_buffer[br];
        int line_len = (int)strlen(line);
        int sc = 0;
    
        while (sc < content_width) {
            int pair = colour_map[sr][sc];
            if (pair == 0) pair = COLOR_PAIR_DEFAULT;
            attr_t attrs = ts_color_pair_to_attrs(pair);
    
            int run_end = sc + 1;
            while (run_end < content_width && colour_map[sr][run_end] == pair)
                run_end++;
    
            int diag_color = -1;
            if (lcd->diagnostics) {
                JsonValue* params = shget(lcd->diagnostics->object, "params");
                JsonValue* diags  = shget(params->object, "diagnostics");
                for (size_t i = 0; i < arrlenu(diags->array); i++) {
                    LSPRange range = get_range(diags->array[i]);

                    if (br < range.start_line || br > range.end_line)
                        continue;

                    int run_bc_start = sc  + xoff;
                    int run_bc_end   = run_end - 1 + xoff;
                    int col_start = (br == range.start_line) ? range.start_character : 0;
                    int col_end   = (br == range.end_line)   ? range.end_character   : INT_MAX;
                    if (run_bc_end >= col_start && run_bc_start <= col_end) {
                        diag_color = get_diagnostic_color(diags->array[i]);
                        break;
                    }
                }
            }
    
            attr_t active_attrs;
            int    active_pair;
            if (diag_color >= 0) {
                active_pair  = diag_color;
                active_attrs = A_UNDERLINE;
            } else {
                active_pair  = pair;
                active_attrs = attrs;
            }
    
            attron(COLOR_PAIR(active_pair) | active_attrs);
            for (int c = sc; c < run_end; c++) {
                int bc = c + xoff;
                addch(bc < line_len ? (unsigned char)line[bc] : ' ');
            }
            attroff(COLOR_PAIR(active_pair) | active_attrs);
            sc = run_end;
        }
    }

    // 4. cleanup
    for (int r = 0; r < content_height; r++) free(colour_map[r]);
    free(colour_map);
}

#endif