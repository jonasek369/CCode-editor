#ifndef _H_SYNTAX_HIGHLIGHTING
#define _H_SYNTAX_HIGHLIGHTING

// ═══════════════════════════════════════════════════════════════════════════════
// Color indices
// ═══════════════════════════════════════════════════════════════════════════════

#define COLOR_GRAY        8
#define COLOR_ORANGE      9
#define COLOR_PURPLE      10
#define COLOR_BROWN_GRAY  11
#define COLOR_FULL_BLACK  12

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

// ═══════════════════════════════════════════════════════════════════════════════
// Color / pair initialization
// ═══════════════════════════════════════════════════════════════════════════════

void init_syntax_colors(void) {
    start_color();
    if (can_change_color()) {
        init_color(COLOR_BLACK,      100, 100, 100);
        init_color(COLOR_WHITE,      700, 700, 700);
        init_color(COLOR_RED,        800, 120, 120);
        init_color(COLOR_GREEN,      120, 700, 300);
        init_color(COLOR_YELLOW,     800, 700, 300);
        init_color(COLOR_CYAN,       300, 600, 700);
        init_color(COLOR_MAGENTA,    700, 400, 700);
        init_color(COLOR_PURPLE,     500, 400, 700);
        init_color(COLOR_GRAY,       400, 400, 400);
        init_color(COLOR_ORANGE,     850, 500, 150);
        init_color(COLOR_FULL_BLACK,   0,   0,   0);
    }

    // Syntax
    init_pair(COLOR_PAIR_DEFAULT,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_KEYWORD,  COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_PAIR_TYPE,     COLOR_CYAN,    COLOR_BLACK);
    init_pair(COLOR_PAIR_STRING,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(COLOR_PAIR_NUMBER,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(COLOR_PAIR_COMMENT,  COLOR_GRAY,    COLOR_BLACK);
    init_pair(COLOR_PAIR_FUNCTION, COLOR_ORANGE,  COLOR_BLACK);
    init_pair(COLOR_PAIR_OPERATOR, COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_PREPROC,  COLOR_MAGENTA, COLOR_BLACK);

    // File browser
    init_pair(COLOR_DIR,     COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_FILE,    COLOR_CYAN,   COLOR_BLACK);
    init_pair(COLOR_SYMLINK, COLOR_RED,    COLOR_BLACK);

    // Diagnostics
    init_pair(COLOR_PAIR_INFORMATION, COLOR_BLUE,   COLOR_BLACK);
    init_pair(COLOR_PAIR_WARNING,     COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR,       COLOR_RED,    COLOR_BLACK);

    // Completion
    init_pair(COLOR_PAIR_COMPLETION, COLOR_FULL_BLACK, COLOR_WHITE);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Attribute helpers
// ═══════════════════════════════════════════════════════════════════════════════

static attr_t pair_to_attrs(int pair) {
    if (pair == COLOR_PAIR_KEYWORD ||
        pair == COLOR_PAIR_FUNCTION ||
        pair == COLOR_PAIR_TYPE)
        return A_BOLD;
    return A_NORMAL;
}

// ═══════════════════════════════════════════════════════════════════════════════
// HSpan — a colored source range
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    int start_row, start_col;
    int end_row,   end_col;
    int color_pair;
} HSpan;

static void push_span(HSpan** spans, TSNode node, int pair) {
    TSPoint s = ts_node_start_point(node);
    TSPoint e = ts_node_end_point(node);
    HSpan h = {
        (int)s.row, (int)s.column,
        (int)e.row, (int)e.column,
        pair
    };
    arrpush(*spans, h);
}

// Viewport guard — prune entire subtrees that don't touch visible rows.
static bool node_outside_viewport(TSNode node, int yoff, int vh) {
    TSPoint s = ts_node_start_point(node);
    TSPoint e = ts_node_end_point(node);
    return (int)e.row < yoff || (int)s.row >= yoff + vh;
}

// Sort comparator for spans (by start_row, then start_col).
static int hspan_cmp(const void* a, const void* b) {
    const HSpan* x = (const HSpan*)a;
    const HSpan* y = (const HSpan*)b;
    if (x->start_row != y->start_row) return x->start_row - y->start_row;
    return x->start_col - y->start_col;
}

// Binary search: first span whose end_row >= target_row.
static int first_span_for_row(HSpan* spans, int count, int row) {
    int lo = 0, hi = count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (spans[mid].end_row < row) lo = mid + 1;
        else                          hi = mid;
    }
    return lo;
}

// ═══════════════════════════════════════════════════════════════════════════════
// C highlighter
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct { char* key; int value; } map_entry;

static map_entry* c_type_map = NULL;

static void init_c_type_map(void) {
    if (c_type_map) return;

    // Keywords
    shput(c_type_map, "if",       COLOR_PAIR_KEYWORD);
    shput(c_type_map, "else",     COLOR_PAIR_KEYWORD);
    shput(c_type_map, "for",      COLOR_PAIR_KEYWORD);
    shput(c_type_map, "while",    COLOR_PAIR_KEYWORD);
    shput(c_type_map, "do",       COLOR_PAIR_KEYWORD);
    shput(c_type_map, "switch",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "case",     COLOR_PAIR_KEYWORD);
    shput(c_type_map, "default",  COLOR_PAIR_KEYWORD);
    shput(c_type_map, "break",    COLOR_PAIR_KEYWORD);
    shput(c_type_map, "continue", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "return",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "goto",     COLOR_PAIR_KEYWORD);
    shput(c_type_map, "typedef",  COLOR_PAIR_KEYWORD);
    shput(c_type_map, "enum",     COLOR_PAIR_KEYWORD);
    shput(c_type_map, "struct",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "union",    COLOR_PAIR_KEYWORD);
    shput(c_type_map, "static",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "extern",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "const",    COLOR_PAIR_KEYWORD);
    shput(c_type_map, "inline",   COLOR_PAIR_KEYWORD);
    shput(c_type_map, "volatile", COLOR_PAIR_KEYWORD);
    shput(c_type_map, "sizeof",   COLOR_PAIR_KEYWORD);

    // Types
    shput(c_type_map, "primitive_type", COLOR_PAIR_TYPE);
    shput(c_type_map, "type_qualifier", COLOR_PAIR_TYPE);
    shput(c_type_map, "type_identifier",COLOR_PAIR_TYPE);

    // Strings
    shput(c_type_map, "string_literal",  COLOR_PAIR_STRING);
    shput(c_type_map, "char_literal",    COLOR_PAIR_STRING);
    shput(c_type_map, "string_content",  COLOR_PAIR_STRING);
    shput(c_type_map, "escape_sequence", COLOR_PAIR_STRING);

    // Numbers
    shput(c_type_map, "number_literal",  COLOR_PAIR_NUMBER);
    shput(c_type_map, "integer_literal", COLOR_PAIR_NUMBER);
    shput(c_type_map, "float_literal",   COLOR_PAIR_NUMBER);

    // Comments
    shput(c_type_map, "comment",       COLOR_PAIR_COMMENT);
    shput(c_type_map, "line_comment",  COLOR_PAIR_COMMENT);
    shput(c_type_map, "block_comment", COLOR_PAIR_COMMENT);

    // Preprocessor
    shput(c_type_map, "preproc_include",   COLOR_PAIR_PREPROC);
    shput(c_type_map, "preproc_def",       COLOR_PAIR_PREPROC);
    shput(c_type_map, "preproc_directive", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#ifndef",  COLOR_PAIR_PREPROC);
    shput(c_type_map, "#if",      COLOR_PAIR_PREPROC);
    shput(c_type_map, "#elif",    COLOR_PAIR_PREPROC);
    shput(c_type_map, "#define",  COLOR_PAIR_PREPROC);
    shput(c_type_map, "#include", COLOR_PAIR_PREPROC);
    shput(c_type_map, "#error",   COLOR_PAIR_PREPROC);
    shput(c_type_map, "#ifdef",   COLOR_PAIR_PREPROC);
    shput(c_type_map, "#else",    COLOR_PAIR_PREPROC);
    shput(c_type_map, "#endif",   COLOR_PAIR_PREPROC);
    shput(c_type_map, "#undef",   COLOR_PAIR_PREPROC);
    shput(c_type_map, "#pragma",  COLOR_PAIR_PREPROC);

    // Operators / punctuation
    shput(c_type_map, "+",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "-",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "*",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "/",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "%",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "=",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "+=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "-=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "*=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "/=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "==", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "!=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">=", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "&&", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "||", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "!",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "&",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "|",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "^",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "~",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "<<", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ">>", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "++", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "--", COLOR_PAIR_OPERATOR);
    shput(c_type_map, "->", COLOR_PAIR_OPERATOR);
    shput(c_type_map, ".",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, ",",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, ";",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, ":",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "{",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "}",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "(",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, ")",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "[",  COLOR_PAIR_OPERATOR);
    shput(c_type_map, "]",  COLOR_PAIR_OPERATOR);
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

static void collect_highlights_c(TSNode node, HSpan** spans,
                                  char** buf, int buf_size,
                                  int yoff, int vh) {
    if (node_outside_viewport(node, yoff, vh)) return;

    const char* type = ts_node_type(node);

    // Function call — color only the callee name, descend normally
    if (strcmp(type, "call_expression") == 0) {
        TSNode callee = ts_node_child(node, 0);
        if (!ts_node_is_null(callee)) {
            const char* ct  = ts_node_type(callee);
            TSNode      ident = callee;
            if (strcmp(ct, "field_expression") == 0) {
                uint32_t fc = ts_node_child_count(callee);
                if (fc > 0) ident = ts_node_child(callee, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
        return;
    }

    // Function declaration — color only the declared name
    if (strcmp(type, "function_declarator") == 0) {
        TSNode name = ts_node_child(node, 0);
        if (!ts_node_is_null(name))
            push_span(spans, name, COLOR_PAIR_FUNCTION);
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
        return;
    }

    // Null pointer constant
    if (strcmp(type, "NULL") == 0 || strcmp(type, "nullptr") == 0) {
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }

    // Boolean constants
    if (strcmp(type, "TRUE")  == 0 || strcmp(type, "true")  == 0 ||
        strcmp(type, "FALSE") == 0 || strcmp(type, "false") == 0) {
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }

    int  pair         = c_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = c_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_c(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
}

// ═══════════════════════════════════════════════════════════════════════════════
// JSON highlighter
// ═══════════════════════════════════════════════════════════════════════════════

static int json_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    if (strcmp(type, "string")          == 0 ||
        strcmp(type, "string_content")  == 0 ||
        strcmp(type, "escape_sequence") == 0)
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

static void collect_highlights_json(TSNode node, HSpan** spans, int yoff, int vh) {
    if (node_outside_viewport(node, yoff, vh)) return;

    const char* type = ts_node_type(node);

    // Object key (first child of "pair") — orange so it differs from values
    if (strcmp(type, "pair") == 0) {
        TSNode key = ts_node_child(node, 0);
        if (!ts_node_is_null(key))
            push_span(spans, key, COLOR_PAIR_FUNCTION);
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 1; i < cc; i++)
            collect_highlights_json(ts_node_child(node, i), spans, yoff, vh);
        return;
    }

    int  pair         = json_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = strcmp(type, "string") == 0;

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_json(ts_node_child(node, i), spans, yoff, vh);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Python highlighter
// ═══════════════════════════════════════════════════════════════════════════════

static int py_node_to_pair(const char* type) {
    if (!type) return COLOR_PAIR_DEFAULT;

    if (strcmp(type, "string")          == 0 ||
        strcmp(type, "string_content")  == 0 ||
        strcmp(type, "string_start")    == 0 ||
        strcmp(type, "string_end")      == 0 ||
        strcmp(type, "escape_sequence") == 0 ||
        strcmp(type, "interpolation")   == 0)
        return COLOR_PAIR_STRING;

    if (strcmp(type, "integer") == 0 ||
        strcmp(type, "float")   == 0 ||
        strcmp(type, "complex") == 0)
        return COLOR_PAIR_NUMBER;

    if (strcmp(type, "comment") == 0)
        return COLOR_PAIR_COMMENT;

    if (strcmp(type, "def")      == 0 || strcmp(type, "class")    == 0 ||
        strcmp(type, "return")   == 0 || strcmp(type, "if")       == 0 ||
        strcmp(type, "elif")     == 0 || strcmp(type, "else")     == 0 ||
        strcmp(type, "for")      == 0 || strcmp(type, "while")    == 0 ||
        strcmp(type, "in")       == 0 || strcmp(type, "not")      == 0 ||
        strcmp(type, "and")      == 0 || strcmp(type, "or")       == 0 ||
        strcmp(type, "is")       == 0 || strcmp(type, "import")   == 0 ||
        strcmp(type, "from")     == 0 || strcmp(type, "as")       == 0 ||
        strcmp(type, "with")     == 0 || strcmp(type, "lambda")   == 0 ||
        strcmp(type, "yield")    == 0 || strcmp(type, "pass")     == 0 ||
        strcmp(type, "break")    == 0 || strcmp(type, "continue") == 0 ||
        strcmp(type, "raise")    == 0 || strcmp(type, "try")      == 0 ||
        strcmp(type, "except")   == 0 || strcmp(type, "finally")  == 0 ||
        strcmp(type, "global")   == 0 || strcmp(type, "nonlocal") == 0 ||
        strcmp(type, "del")      == 0 || strcmp(type, "assert")   == 0 ||
        strcmp(type, "async")    == 0 || strcmp(type, "await")    == 0)
        return COLOR_PAIR_KEYWORD;

    return COLOR_PAIR_DEFAULT;
}

static bool py_is_span_container(const char* type) {
    return strcmp(type, "string")              == 0 ||
           strcmp(type, "comment")             == 0 ||
           strcmp(type, "concatenated_string") == 0;
}

static void collect_highlights_python(TSNode node, HSpan** spans,
                                       char** buf, int buf_size,
                                       int yoff, int vh) {
    if (node_outside_viewport(node, yoff, vh)) return;

    const char* type = ts_node_type(node);

    // Function / method call — color only the name
    if (strcmp(type, "call") == 0) {
        TSNode func = ts_node_child(node, 0);
        if (!ts_node_is_null(func)) {
            const char* ft    = ts_node_type(func);
            TSNode      ident = func;
            if (strcmp(ft, "attribute") == 0) {
                uint32_t fc = ts_node_child_count(func);
                if (fc > 0) ident = ts_node_child(func, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
        return;
    }

    // Function / class definition — color the declared name (child 1)
    if (strcmp(type, "function_definition") == 0 ||
        strcmp(type, "class_definition")    == 0) {
        if (ts_node_child_count(node) > 1) {
            TSNode name = ts_node_child(node, 1);
            if (!ts_node_is_null(name))
                push_span(spans, name, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
        return;
    }

    // Decorator — highlight as a unit
    if (strcmp(type, "decorator") == 0) {
        push_span(spans, node, COLOR_PAIR_FUNCTION);
        return;
    }

    // Boolean constants
    if (strcmp(type, "True")  == 0 || strcmp(type, "true")  == 0 ||
        strcmp(type, "False") == 0 || strcmp(type, "false") == 0) {
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }

    // None constant
    if (strcmp(type, "None") == 0 || strcmp(type, "none") == 0) {
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }

    int  pair         = py_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = py_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_python(ts_node_child(node, i), spans, buf, buf_size, yoff, vh);
}

// ═══════════════════════════════════════════════════════════════════════════════
// C# highlighter
// ═══════════════════════════════════════════════════════════════════════════════

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
    if (strcmp(type, "integer_literal")        == 0 ||
        strcmp(type, "real_literal")           == 0 ||
        strcmp(type, "hex_integer_literal")    == 0 ||
        strcmp(type, "binary_integer_literal") == 0)
        return COLOR_PAIR_NUMBER;

    // Comments
    if (strcmp(type, "comment")           == 0 ||
        strcmp(type, "single_line_comment")== 0 ||
        strcmp(type, "multiline_comment")  == 0)
        return COLOR_PAIR_COMMENT;

    // Operators / punctuation
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
    if (strcmp(type, "abstract")   == 0 || strcmp(type, "as")        == 0 ||
        strcmp(type, "async")      == 0 || strcmp(type, "await")      == 0 ||
        strcmp(type, "base")       == 0 || strcmp(type, "break")      == 0 ||
        strcmp(type, "case")       == 0 || strcmp(type, "catch")      == 0 ||
        strcmp(type, "checked")    == 0 || strcmp(type, "class")      == 0 ||
        strcmp(type, "const")      == 0 || strcmp(type, "continue")   == 0 ||
        strcmp(type, "default")    == 0 || strcmp(type, "delegate")   == 0 ||
        strcmp(type, "do")         == 0 || strcmp(type, "else")       == 0 ||
        strcmp(type, "enum")       == 0 || strcmp(type, "event")      == 0 ||
        strcmp(type, "explicit")   == 0 || strcmp(type, "extern")     == 0 ||
        strcmp(type, "finally")    == 0 || strcmp(type, "fixed")      == 0 ||
        strcmp(type, "for")        == 0 || strcmp(type, "foreach")    == 0 ||
        strcmp(type, "goto")       == 0 || strcmp(type, "if")         == 0 ||
        strcmp(type, "implicit")   == 0 || strcmp(type, "in")         == 0 ||
        strcmp(type, "interface")  == 0 || strcmp(type, "internal")   == 0 ||
        strcmp(type, "is")         == 0 || strcmp(type, "lock")       == 0 ||
        strcmp(type, "namespace")  == 0 || strcmp(type, "new")        == 0 ||
        strcmp(type, "operator")   == 0 || strcmp(type, "out")        == 0 ||
        strcmp(type, "override")   == 0 || strcmp(type, "params")     == 0 ||
        strcmp(type, "private")    == 0 || strcmp(type, "protected")  == 0 ||
        strcmp(type, "public")     == 0 || strcmp(type, "readonly")   == 0 ||
        strcmp(type, "record")     == 0 || strcmp(type, "ref")        == 0 ||
        strcmp(type, "return")     == 0 || strcmp(type, "sealed")     == 0 ||
        strcmp(type, "sizeof")     == 0 || strcmp(type, "stackalloc") == 0 ||
        strcmp(type, "static")     == 0 || strcmp(type, "struct")     == 0 ||
        strcmp(type, "switch")     == 0 || strcmp(type, "this")       == 0 ||
        strcmp(type, "throw")      == 0 || strcmp(type, "try")        == 0 ||
        strcmp(type, "typeof")     == 0 || strcmp(type, "unchecked")  == 0 ||
        strcmp(type, "unsafe")     == 0 || strcmp(type, "using")      == 0 ||
        strcmp(type, "virtual")    == 0 || strcmp(type, "void")       == 0 ||
        strcmp(type, "volatile")   == 0 || strcmp(type, "where")      == 0 ||
        strcmp(type, "while")      == 0 || strcmp(type, "with")       == 0 ||
        strcmp(type, "yield")      == 0 || strcmp(type, "var")        == 0)
        return COLOR_PAIR_KEYWORD;

    return COLOR_PAIR_DEFAULT;
}

static bool cs_is_span_container(const char* type) {
    return strcmp(type, "string_literal")                 == 0 ||
           strcmp(type, "verbatim_string_literal")        == 0 ||
           strcmp(type, "raw_string_literal")             == 0 ||
           strcmp(type, "interpolated_string_expression") == 0 ||
           strcmp(type, "comment")                        == 0 ||
           strcmp(type, "multiline_comment")              == 0;
}

static void collect_highlights_csharp(TSNode node, HSpan** spans, int yoff, int vh) {
    if (node_outside_viewport(node, yoff, vh)) return;

    const char* type = ts_node_type(node);

    // Method invocation — color name only
    if (strcmp(type, "invocation_expression") == 0) {
        TSNode func = ts_node_child(node, 0);
        if (!ts_node_is_null(func)) {
            const char* ft    = ts_node_type(func);
            TSNode      ident = func;
            if (strcmp(ft, "member_access_expression") == 0) {
                uint32_t fc = ts_node_child_count(func);
                if (fc > 0) ident = ts_node_child(func, fc - 1);
            }
            if (!ts_node_is_null(ident))
                push_span(spans, ident, COLOR_PAIR_FUNCTION);
        }
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans, yoff, vh);
        return;
    }

    // Method / constructor / local-function definition — color declared name
    if (strcmp(type, "method_declaration")              == 0 ||
        strcmp(type, "constructor_declaration")         == 0 ||
        strcmp(type, "local_function_statement")        == 0 ||
        strcmp(type, "operator_declaration")            == 0 ||
        strcmp(type, "conversion_operator_declaration") == 0) {
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++) {
            TSNode child = ts_node_child(node, i);
            if (strcmp(ts_node_type(child), "identifier") == 0) {
                push_span(spans, child, COLOR_PAIR_FUNCTION);
                break;
            }
        }
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans, yoff, vh);
        return;
    }

    // Type declaration — color declared name
    if (strcmp(type, "class_declaration")        == 0 ||
        strcmp(type, "struct_declaration")       == 0 ||
        strcmp(type, "interface_declaration")    == 0 ||
        strcmp(type, "enum_declaration")         == 0 ||
        strcmp(type, "record_declaration")       == 0 ||
        strcmp(type, "record_struct_declaration")== 0) {
        uint32_t cc = ts_node_child_count(node);
        for (uint32_t i = 0; i < cc; i++) {
            TSNode child = ts_node_child(node, i);
            if (strcmp(ts_node_type(child), "identifier") == 0) {
                push_span(spans, child, COLOR_PAIR_FUNCTION);
                break;
            }
        }
        for (uint32_t i = 0; i < cc; i++)
            collect_highlights_csharp(ts_node_child(node, i), spans, yoff, vh);
        return;
    }

    // Attributes — highlight as orange
    if (strcmp(type, "attribute") == 0) {
        push_span(spans, node, COLOR_PAIR_FUNCTION);
        return;
    }

    // Boolean literals
    if (strcmp(type, "true")  == 0 || strcmp(type, "false") == 0) {
        push_span(spans, node, COLOR_PAIR_KEYWORD);
        return;
    }

    // Null / default literals
    if (strcmp(type, "null") == 0 || strcmp(type, "default") == 0) {
        push_span(spans, node, COLOR_PAIR_NUMBER);
        return;
    }

    // Preprocessor directives — gray like comments
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

    int  pair         = cs_node_to_pair(type);
    bool is_leaf      = ts_node_child_count(node) == 0;
    bool is_container = cs_is_span_container(type);

    if (pair != COLOR_PAIR_DEFAULT && (is_leaf || is_container)) {
        push_span(spans, node, pair);
        if (is_container) return;
    }

    uint32_t cc = ts_node_child_count(node);
    for (uint32_t i = 0; i < cc; i++)
        collect_highlights_csharp(ts_node_child(node, i), spans, yoff, vh);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Diagnostic helpers
// ═══════════════════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════════

void init_syntax_highlighting(void) {
    init_c_type_map();
    init_syntax_colors();
}

void destroy_syntax_highlighting(void) {
    shfree(c_type_map);
    c_type_map = NULL;
}

// ── Main render ───────────────────────────────────────────────────────────────
//
// Pipeline:
//   1. Walk only nodes that overlap the visible viewport  (viewport pruning)
//   2. Sort spans once by position                        (O(n log n), n small)
//   3. Render line-by-line in a single forward pass       (no colour_map alloc)
//
void apply_tree_sitter_syntax_highlighting(LayerCodeData* lcd, SyntaxLanguage lang) {
    if (!lcd) return;

    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols);
    int vh = screen_rows - 1;   // visible height (content rows)
    int vw = screen_cols;       // visible width

    int buffer_size = arrlen(lcd->code_buffer);
    int yoff        = lcd->cursor->yoff;
    int xoff        = lcd->cursor->xoff;

    // 1. Collect spans — viewport-pruned walk
    HSpan* spans = NULL;
    if (lcd->parser && lcd->tree) {
        TSNode root = ts_tree_root_node(lcd->tree);
        switch (lang) {
            case LANG_C:
                collect_highlights_c(root, &spans,
                                     lcd->code_buffer, buffer_size,
                                     yoff, vh);
                break;
            case LANG_JSON:
                collect_highlights_json(root, &spans, yoff, vh);
                break;
            case LANG_PYTHON:
                collect_highlights_python(root, &spans,
                                          lcd->code_buffer, buffer_size,
                                          yoff, vh);
                break;
            case LANG_C_SHARP:
                collect_highlights_csharp(root, &spans, yoff, vh);
                break;
            default:
                break;
        }
    }

    // 2. Sort spans by (start_row, start_col) for the forward-pass renderer
    int span_count = arrlen(spans);
    if (span_count > 0)
        qsort(spans, span_count, sizeof(HSpan), hspan_cmp);

    // 3. Render — one forward pass per screen row, no 2-D colour_map
    for (int sr = 0; sr < vh; sr++) {
        int br = sr + yoff;
        move(sr + 1, 0);

        if (br >= buffer_size || !lcd->code_buffer[br]) {
            clrtoeol();
            continue;
        }

        const char* line     = lcd->code_buffer[br];
        int         line_len = (int)strlen(line);   // cached once per line

        // Binary-search to the first span whose end_row >= br
        int span_base = first_span_for_row(spans, span_count, br);

        // bc = current buffer column (absolute, not screen-relative)
        int bc = xoff;

        for (int si = span_base; si < span_count; si++) {
            HSpan* sp = &spans[si];
            if (sp->start_row > br) break;  // spans are sorted; no more for this line
            if (sp->end_row   < br) continue;

            int sp_col_start = (sp->start_row == br) ? sp->start_col : 0;
            int sp_col_end   = (sp->end_row   == br) ? sp->end_col   : line_len;

            // ── Gap before this span: render in default ──────────────────────
            if (bc < sp_col_start) {
                attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));
                for (int c = bc; c < sp_col_start && (c - xoff) < vw; c++)
                    addch(c < line_len ? (unsigned char)line[c] : ' ');
                attroff(COLOR_PAIR(COLOR_PAIR_DEFAULT));
                bc = sp_col_start;
            }

            if (bc >= sp_col_end || (bc - xoff) >= vw) continue;

            // ── Diagnostic overlay: check if this span is under a diagnostic ─
            int diag_color = -1;
            if (lcd->diagnostics) {
                JsonValue* params = shget(lcd->diagnostics->object, "params");
                JsonValue* diags  = shget(params->object, "diagnostics");
                size_t     nd     = arrlenu(diags->array);

                for (size_t di = 0; di < nd; di++) {
                    LSPRange* range = lcd->ranges[di];
                    if (!range) {
                        lcd->ranges[di] = get_range(diags->array[di]);
                        range = lcd->ranges[di];
                    }
                    if (br < range->start_line || br > range->end_line) continue;

                    int col_start = (br == range->start_line) ? range->start_character : 0;
                    int col_end   = (br == range->end_line)   ? range->end_character   : INT_MAX;

                    if (sp_col_end - 1 >= col_start && bc <= col_end) {
                        diag_color = get_diagnostic_color(diags->array[di]);
                        break;
                    }
                }
            }

            // ── Render the span ──────────────────────────────────────────────
            int    active_pair;
            attr_t active_attrs;
            if (diag_color >= 0) {
                active_pair  = diag_color;
                active_attrs = A_UNDERLINE;
            } else {
                active_pair  = sp->color_pair;
                active_attrs = pair_to_attrs(sp->color_pair);
            }

            attron(COLOR_PAIR(active_pair) | active_attrs);
            for (int c = bc; c < sp_col_end && (c - xoff) < vw; c++)
                addch(c < line_len ? (unsigned char)line[c] : ' ');
            attroff(COLOR_PAIR(active_pair) | active_attrs);

            bc = sp_col_end;
        }

        // ── Remainder of line in default ─────────────────────────────────────
        if ((bc - xoff) < vw) {
            attron(COLOR_PAIR(COLOR_PAIR_DEFAULT));
            for (; (bc - xoff) < vw; bc++)
                addch(bc < line_len ? (unsigned char)line[bc] : ' ');
            attroff(COLOR_PAIR(COLOR_PAIR_DEFAULT));
        }
    }

    arrfree(spans);
}

#endif // _H_SYNTAX_HIGHIGHTINGL