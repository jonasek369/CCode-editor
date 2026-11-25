#ifndef _H_CODE_RENDER
#define _H_CODE_RENDER

/*
    Code layer drawing

    Layers of information are applied in order: syntax highlighting, diagnostics, search matches, matching
    bracket and selection
*/

// Attributes of the visible lines, reused between frames
static chtype** render_rows = NULL;

static void render_rows_prepare(LayerCodeData* lcd, int yoff, int rows){
    while(arrlen(render_rows) < rows) arrput(render_rows, NULL);
    for(int r = 0; r < rows; r++){
        int len = (int)line_content_length(lcd->code_buffer[yoff + r]);
        arrsetlen(render_rows[r], len + 1);
        memset(render_rows[r], 0, (len + 1) * sizeof(chtype));
    }
}


// Sets attr on [c0, c1) of a visible row, only where nothing was set before
static inline void render_fill(int r, int c0, int c1, chtype attr, bool overwrite){
    chtype* a = render_rows[r];
    int len = (int)arrlen(a) - 1;
    if(c0 < 0) c0 = 0;
    if(c1 > len) c1 = len;
    for(int c = c0; c < c1; c++){
        if(overwrite || a[c] == 0) a[c] = attr;
    }
}

// Changes only the color of [c0, c1), keeps other attributes
static inline void render_recolor(int r, int c0, int c1, int pair, attr_t add){
    chtype* a = render_rows[r];
    int len = (int)arrlen(a) - 1;
    if(c0 < 0) c0 = 0;
    if(c1 > len) c1 = len;
    for(int c = c0; c < c1; c++){
        chtype v = a[c] ? a[c] : COLOR_PAIR(COLOR_PAIR_DEFAULT);
        a[c] = (v & ~A_COLOR & ~A_CHARTEXT) | COLOR_PAIR(pair) | add;
    }
}

static inline void render_add_attr(int r, int c0, int c1, attr_t add){
    chtype* a = render_rows[r];
    int len = (int)arrlen(a) - 1;
    if(c0 < 0) c0 = 0;
    if(c1 > len + 1) c1 = len + 1; // the extra cell marks a selected line break
    for(int c = c0; c < c1; c++){
        chtype v = a[c] ? a[c] : COLOR_PAIR(COLOR_PAIR_DEFAULT);
        a[c] = v | add;
    }
}

#include "query_predicates.h"

static TSQueryCursor* render_query_cursor = NULL;

typedef struct {
    uint32_t start_byte, end_byte;
    uint32_t pattern;
    TSPoint start, end;
    int pair;
} RenderCapture;

static RenderCapture* render_captures = NULL;

// Bigger nodes first so the nodes inside them override them, then later patterns
// override earlier ones (queries list general patterns before specific ones)
static int render_capture_cmp(const void* a, const void* b){
    const RenderCapture* x = a;
    const RenderCapture* y = b;
    if(x->start_byte != y->start_byte) return x->start_byte < y->start_byte ? -1 : 1;
    if(x->end_byte != y->end_byte) return x->end_byte > y->end_byte ? -1 : 1;
    if(x->pattern != y->pattern) return x->pattern < y->pattern ? -1 : 1;
    return 0;
}

static void render_syntax_tree_sitter(LayerCodeData* lcd, int yoff, int rows){
    if(!render_query_cursor) render_query_cursor = ts_query_cursor_new();
    TSQueryCursor* qc = render_query_cursor;
    TSNode root = ts_tree_root_node(lcd->tree);
    ts_query_cursor_set_point_range(qc,
        (TSPoint){ (uint32_t)yoff, 0 },
        (TSPoint){ (uint32_t)(yoff + rows), 0 });
    ts_query_cursor_exec(qc, lcd->query, root);

    arrsetlen(render_captures, 0);
    uint32_t capture_count = ts_query_capture_count(lcd->query);
    TSQueryMatch match;
    while(ts_query_cursor_next_match(qc, &match)){
        if(!query_match_passes(lcd, lcd->query, &match)) continue;
        for(uint32_t i = 0; i < match.capture_count; i++){
            TSQueryCapture capture = match.captures[i];
            if(capture.index >= capture_count) continue;
            uint32_t name_len = 0;
            const char* cap_name = ts_query_capture_name_for_id(lcd->query, capture.index, &name_len);
            // captures starting with '_' are helpers of the query, not highlights
            if(cap_name[0] == '_') continue;
            RenderCapture rc = {
                .start_byte = ts_node_start_byte(capture.node),
                .end_byte   = ts_node_end_byte(capture.node),
                .pattern    = match.pattern_index,
                .start      = ts_node_start_point(capture.node),
                .end        = ts_node_end_point(capture.node),
                .pair       = name_to_color_pair(cap_name),
            };
            arrput(render_captures, rc);
        }
    }
    size_t count = arrlenu(render_captures);
    if(count > 1) qsort(render_captures, count, sizeof(RenderCapture), render_capture_cmp);

    for(size_t k = 0; k < count; k++){
        RenderCapture* rc = &render_captures[k];
        if(rc->pair <= 0) continue;
        int r0 = (int)rc->start.row < yoff ? yoff : (int)rc->start.row;
        int r1 = (int)rc->end.row >= yoff + rows ? yoff + rows - 1 : (int)rc->end.row;
        for(int br = r0; br <= r1; br++){
            int c0 = (br == (int)rc->start.row) ? (int)rc->start.column : 0;
            int c1 = (br == (int)rc->end.row) ? (int)rc->end.column : INT_MAX;
            render_fill(br - yoff, c0, c1, COLOR_PAIR(rc->pair), true);
        }
    }
}

/*
    Fallback highlighting for languages without a tree-sitter grammar:
    comments, strings, numbers and common keywords
*/

static const char* generic_keywords[] = {
    "if", "else", "elif", "for", "while", "do", "switch", "case", "default", "break", "continue", "return",
    "goto", "try", "catch", "finally", "throw", "throws", "raise", "except", "with", "as", "in", "is", "not",
    "and", "or", "new", "delete", "import", "export", "from", "package", "module", "use", "using", "namespace",
    "class", "struct", "enum", "union", "interface", "trait", "impl", "type", "typedef", "extends", "implements",
    "public", "private", "protected", "static", "const", "let", "var", "val", "mut", "fn", "func", "function",
    "def", "lambda", "async", "await", "yield", "go", "defer", "chan", "select", "match", "when", "then", "fi",
    "esac", "done", "end", "begin", "local", "export", "unset", "readonly", "true", "false", "null", "nil",
    "None", "True", "False", "undefined", "this", "self", "super", "void", "int", "float", "double", "char",
    "bool", "string", "auto", "virtual", "override", "final", "abstract", "template", "typename", "where",
    "include", "define", "ifdef", "ifndef", "endif", "pragma", NULL
};

static bool generic_is_keyword(const char* s, int len){
    for(size_t i = 0; generic_keywords[i]; i++){
        if((int)strlen(generic_keywords[i]) == len && strncmp(generic_keywords[i], s, len) == 0) return true;
    }
    return false;
}

static void render_syntax_generic(LayerCodeData* lcd, int yoff, int rows){
    const char* lc = language_line_comment(lcd->filename);
    int lcl = (int)strlen(lc);
    bool c_like = strcmp(lc, "//") == 0;
    const char* name = language_display_name(lcd->filename);
    if(strcmp(name, "Text") == 0 || strcmp(name, "Markdown") == 0) return;

    // block comment state carried in from the lines above the view
    bool in_block = false;
    if(c_like){
        int from = yoff - 2000;
        if(from < 0) from = 0;
        for(int br = from; br < yoff; br++){
            char* line = lcd->code_buffer[br];
            int len = (int)line_content_length(line);
            for(int i = 0; i + 1 < len; i++){
                if(!in_block && line[i] == '/' && line[i+1] == '*'){ in_block = true; i++; }
                else if(in_block && line[i] == '*' && line[i+1] == '/'){ in_block = false; i++; }
                else if(!in_block && line[i] == '/' && line[i+1] == '/') break;
            }
        }
    }

    for(int r = 0; r < rows; r++){
        char* line = lcd->code_buffer[yoff + r];
        int len = (int)line_content_length(line);
        int i = 0;
        while(i < len){
            if(in_block){
                int s = i;
                while(i < len && !(i + 1 < len && line[i] == '*' && line[i+1] == '/')) i++;
                if(i < len){ i += 2; in_block = false; }
                render_fill(r, s, i, COLOR_PAIR(COLOR_PAIR_COMMENT), false);
                continue;
            }
            char c = line[i];
            if(c_like && c == '/' && i + 1 < len && line[i+1] == '*'){
                in_block = true;
                continue;
            }
            if(strncmp(line + i, lc, lcl) == 0 && (lcl > 1 || c != '#' || i == 0 || isspace((unsigned char)line[i-1]))){
                // "#include" style directives in C like files are not comments
                render_fill(r, i, len, COLOR_PAIR(COLOR_PAIR_COMMENT), false);
                break;
            }
            if(c == '"' || c == '\'' || c == '`'){
                int s = i++;
                while(i < len && line[i] != c){
                    if(line[i] == '\\') i++;
                    i++;
                }
                if(i < len) i++;
                render_fill(r, s, i, COLOR_PAIR(COLOR_PAIR_STRING), false);
                continue;
            }
            if(isdigit((unsigned char)c) && (i == 0 || !is_word_byte(line[i-1]))){
                int s = i;
                while(i < len && (isalnum((unsigned char)line[i]) || line[i] == '.' || line[i] == '_')) i++;
                render_fill(r, s, i, COLOR_PAIR(COLOR_PAIR_NUMBER), false);
                continue;
            }
            if(is_word_byte(c)){
                int s = i;
                while(i < len && is_word_byte(line[i])) i++;
                if(generic_is_keyword(line + s, i - s)){
                    render_fill(r, s, i, COLOR_PAIR(COLOR_PAIR_KEYWORD), false);
                }else if(i < len && line[i] == '('){
                    render_fill(r, s, i, COLOR_PAIR(COLOR_PAIR_FUNCTION), false);
                }
                continue;
            }
            i++;
        }
    }
}


// Worst diagnostic severity per visible row (1 error .. 4 hint), 0 when none
static int* render_row_severity = NULL;

static void render_diagnostics(LayerCodeData* lcd, int yoff, int rows){
    arrsetlen(render_row_severity, rows);
    for(int r = 0; r < rows; r++) render_row_severity[r] = 0;
    if(!lcd->diagnostics || !lcd->allow_diagnostics) return;
    JsonValue* params = shget(lcd->diagnostics->object, "params");
    if(!params || params->type != JSON_OBJECT) return;
    JsonValue* diags = shget(params->object, "diagnostics");
    if(!diags || diags->type != JSON_ARRAY) return;
    size_t count = arrlenu(diags->array);
    if(arrlenu(lcd->ranges) < count) return;

    for(size_t i = 0; i < count; i++){
        if(!lcd->ranges[i]) lcd->ranges[i] = get_range(diags->array[i]);
        LSPRange* range = lcd->ranges[i];
        if(range->start_line < 0) continue;
        int color = get_diagnostic_color(diags->array[i]);
        JsonValue* sev = shget(diags->array[i]->object, "severity");
        int severity = sev ? (int)json_get_integer(sev) : 1;
        if(color < 0) continue;
        int r0 = range->start_line < yoff ? yoff : range->start_line;
        int r1 = range->end_line >= yoff + rows ? yoff + rows - 1 : range->end_line;
        for(int br = r0; br <= r1; br++){
            int r = br - yoff;
            int len = (int)line_content_length(lcd->code_buffer[br]);
            int c0 = (br == range->start_line) ? range->start_character : 0;
            int c1 = (br == range->end_line) ? range->end_character : len;
            // zero width diagnostics mark the character they point at
            if(c0 >= c1){
                c1 = c0 + 1;
                if(c0 >= len && len > 0){ c0 = len - 1; c1 = len; }
            }
            render_recolor(r, c0, c1, color, A_UNDERLINE);
            if(render_row_severity[r] == 0 || severity < render_row_severity[r]) render_row_severity[r] = severity;
        }
    }
}

static void render_search_matches(LayerCodeData* lcd, int yoff, int rows){
    if(!lcd->finding_substr) return;
    const char* s = lcd->finding_substr->substr;
    int sl = (int)strlen(s);
    if(sl == 0) return;
    bool ic = search_ignore_case(s);
    for(int r = 0; r < rows; r++){
        char* line = lcd->code_buffer[yoff + r];
        char* p = line;
        while((p = find_case_strstr(p, s, ic)) != NULL){
            int c = (int)(p - line);
            bool current = (yoff + r == lcd->cursor->y && c == lcd->cursor->x);
            render_recolor(r, c, c + sl, COLOR_PAIR_WARNING, A_REVERSE | (current ? A_BOLD | A_UNDERLINE : 0));
            p += sl;
        }
    }
}

static void render_selection(LayerCodeData* lcd, int yoff, int rows){
    int sr, sc, er, ec;
    if(!sel_range(lcd, &sr, &sc, &er, &ec)) return;
    for(int br = sr; br <= er; br++){
        if(br < yoff || br >= yoff + rows) continue;
        int len = (int)line_content_length(lcd->code_buffer[br]);
        int c0 = br == sr ? sc : 0;
        // selected line breaks are shown as one selected cell behind the text
        int c1 = br == er ? ec : len + 1;
        render_add_attr(br - yoff, c0, c1, A_REVERSE);
    }
}

static void render_brackets(LayerCodeData* lcd, int yoff, int rows){
    CCodeConfig* cfg = view_config();
    if(cfg && !cfg->highlight_matching_bracket) return;
    int br, bx, mr, mx;
    if(!bracket_at_cursor(lcd, &br, &bx)) return;
    if(!find_matching_bracket(lcd, br, bx, &mr, &mx)) return;
    if(br >= yoff && br < yoff + rows) render_add_attr(br - yoff, bx, bx + 1, A_BOLD | A_UNDERLINE);
    if(mr >= yoff && mr < yoff + rows) render_add_attr(mr - yoff, mx, mx + 1, A_BOLD | A_UNDERLINE);
}

static void render_gutter(LayerCodeData* lcd, VirtualWindow* area, int gutter, int r, int br, int n){
    int sy = area->y + r;
    int digits = gutter - 2;
    CCodeConfig* cfg = view_config();
    if(br >= n){
        attrset(COLOR_PAIR(COLOR_PAIR_COMMENT));
        mvaddch(sy, area->x, ' ');
        mvprintw(sy, area->x + 1, "%*s ", digits, "~");
        attrset(A_NORMAL);
        return;
    }
    // diagnostic sign
    int sev = render_row_severity && r < arrlen(render_row_severity) ? render_row_severity[r] : 0;
    if(sev == 1){ attrset(COLOR_PAIR(COLOR_PAIR_ERROR) | A_BOLD); mvaddch(sy, area->x, 'E'); }
    else if(sev == 2){ attrset(COLOR_PAIR(COLOR_PAIR_WARNING) | A_BOLD); mvaddch(sy, area->x, 'W'); }
    else if(sev >= 3){ attrset(COLOR_PAIR(COLOR_PAIR_INFORMATION)); mvaddch(sy, area->x, 'i'); }
    else { attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT)); mvaddch(sy, area->x, ' '); }

    bool current = br == lcd->cursor->y;
    int number = br + 1;
    if(cfg && cfg->relative_line_numbers && !current){
        number = br > lcd->cursor->y ? br - lcd->cursor->y : lcd->cursor->y - br;
    }
    if(current && (!cfg || cfg->highlight_current_line)){
        attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_BOLD);
    }else{
        attrset(COLOR_PAIR(COLOR_PAIR_COMMENT));
    }
    if(current && cfg && cfg->relative_line_numbers){
        mvprintw(sy, area->x + 1, "%-*d ", digits, number);
    }else{
        mvprintw(sy, area->x + 1, "%*d ", digits, number);
    }
    attrset(A_NORMAL);
}

static void render_line(LayerCodeData* lcd, VirtualWindow* text, int r, int br){
    char* line = lcd->code_buffer[br];
    int len = (int)line_content_length(line);
    chtype* attrs = render_rows[r];
    int xoff = lcd->cursor->xoff;
    int right = xoff + text->width;
    int sy = text->y + r;
    CCodeConfig* cfg = view_config();
    bool show_ws = cfg && cfg->show_whitespace;

    int trailing = len;
    if(show_ws){
        while(trailing > 0 && (line[trailing-1] == ' ' || line[trailing-1] == '\t')) trailing--;
    }

    int col = 0;
    int i = 0;
    bool drawn_prev = false;
    while(i < len && col < right){
        int bytes;
        int w = view_char_width(line, i, len, col, &bytes);
        chtype attr = attrs[i] ? attrs[i] : COLOR_PAIR(COLOR_PAIR_DEFAULT);
        int sx = text->x + col - xoff;
        unsigned char c = (unsigned char)line[i];

        if(w == 0){
            // combining character, attaches to the previous cell
            if(drawn_prev){
                attrset(attr);
                addnstr(line + i, bytes);
            }
        }else if(col < xoff || col + w > right){
            // wide character cut by the view edge
            attrset(attr);
            for(int k = 0; k < w; k++){
                int cc = col + k;
                if(cc >= xoff && cc < right) mvaddch(sy, text->x + cc - xoff, ' ');
            }
            drawn_prev = false;
        }else if(c == '\t'){
            if(show_ws){
                attrset((attr & ~A_COLOR) | COLOR_PAIR(COLOR_PAIR_COMMENT));
                mvaddstr(sy, sx, "\xe2\x86\x92"); // →
                attrset(attr);
                for(int k = 1; k < w; k++) mvaddch(sy, sx + k, ' ');
            }else{
                attrset(attr);
                for(int k = 0; k < w; k++) mvaddch(sy, sx + k, ' ');
            }
            drawn_prev = false;
        }else if(c < 0x20 || c == 0x7F){
            attrset(attr | A_DIM);
            mvaddch(sy, sx, '^');
            mvaddch(sy, sx + 1, c == 0x7F ? '?' : c + '@');
            drawn_prev = false;
        }else if(c == ' ' && show_ws && i >= trailing){
            attrset((attr & ~A_COLOR) | COLOR_PAIR(COLOR_PAIR_COMMENT));
            mvaddstr(sy, sx, "\xc2\xb7"); // ·
            drawn_prev = false;
        }else if(c >= 0x80){
            uint32_t cp;
            u8_decode(line + i, len - i, &cp);
            attrset(attr);
            if(cp == 0xFFFD && bytes == 1){
                mvaddch(sy, sx, '?');
            }else{
                mvaddnstr(sy, sx, line + i, bytes);
            }
            drawn_prev = true;
        }else{
            attrset(attr);
            mvaddch(sy, sx, c);
            drawn_prev = true;
        }
        col += w;
        i += bytes;
    }
    // selected line break
    if(i >= len && col >= xoff && col < right && (attrs[len] & A_REVERSE)){
        attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT) | A_REVERSE);
        mvaddch(sy, text->x + col - xoff, ' ');
    }
    attrset(A_NORMAL);
}


void layer_code_render_completion_window(CCode* ccode, Layer* layer){
    (void) ccode;
    LayerCodeData* code_data = (LayerCodeData*) layer->layer_data;
    if(!code_data->completion_window || !code_data->completion_window->completion) return;

    int sy, sx;
    if(!view_cursor_screen(code_data, &sy, &sx)) return;

    JsonValue* result = shget(code_data->completion_window->completion->object, "result");
    if(!result || result->type != JSON_OBJECT) return;
    JsonValue* items = shget(result->object, "items");
    if(!items || items->type != JSON_ARRAY) return;

    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    CompletionWindow* win = code_data->completion_window;
    size_t count = win->items_count;
    if(count == 0) return;

    // show the popup below the cursor, or above it when there is no room
    int max_rows = 10;
    int below = screen_h - 2 - sy;
    int above = sy - 1;
    bool down = below >= max_rows || below >= above;
    int rows = down ? below : above;
    if(rows > max_rows) rows = max_rows;
    if(rows > (int)count) rows = (int)count;
    if(rows <= 0) return;

    size_t selected = code_data->completion_window->selected;
    size_t first = 0;
    if(selected >= (size_t)rows) first = selected - rows + 1;

    int width = 0;
    for(size_t i = first; i < first + rows && i < count; i++){
        JsonValue* text = shget(items->array[win->visible[i]]->object, "label");
        if(!text || text->type != JSON_STRING) continue;
        int l = (int)strlen(text->string);
        if(l > width) width = l;
    }
    width += 2;
    if(width > screen_w / 2) width = screen_w / 2;
    if(sx + width > screen_w) sx = screen_w - width;
    if(sx < 0) sx = 0;

    for(int k = 0; k < rows; k++){
        size_t i = first + k;
        JsonValue* text = shget(items->array[win->visible[i]]->object, "label");
        const char* label = (text && text->type == JSON_STRING) ? text->string : "";
        // clangd prefixes labels with a space or a bullet
        while(*label == ' ') label++;
        if((unsigned char)*label == 0xE2) label += u8_seq_len((unsigned char)*label);
        int y = down ? sy + 1 + k : sy - rows + k;
        attrset(COLOR_PAIR(COLOR_PAIR_COMPLETION) | (i == selected ? A_REVERSE : 0));
        mvprintw(y, sx, " %-*.*s", width - 1, width - 1, label);
    }
    // scroll indicator
    if(count > (size_t)rows){
        attrset(COLOR_PAIR(COLOR_PAIR_COMMENT));
        int y = down ? sy + rows : sy - rows - 1;
        if(y > 0 && y < screen_h - 1) mvprintw(y + (down ? 1 : 0), sx, " %d/%d", (int)(selected + 1), (int)count);
    }
    attrset(A_NORMAL);
}


void layer_code_render(CCode* ccode, Layer* layer) {
    if (!ccode || !layer || layer->type != LAYER_CODE || layer->layer_data == NULL) {
        return;
    }
    START_PROFILING();
    LayerCodeData* lcd = (LayerCodeData*) layer->layer_data;
    if(!lcd->code_buffer){
        char* line = NULL;
        arrput(line, '\n');
        arrput(line, '\0');
        arrput(lcd->code_buffer, line);
    }
    int n = (int)arrlen(lcd->code_buffer);
    if(lcd->cursor->y >= n) lcd->cursor->y = n - 1;

    // follow the cursor when something moved it
    if(lcd->cursor->x != lcd->last_cx || lcd->cursor->y != lcd->last_cy){
        view_scroll_to_cursor(lcd);
        lcd->last_cx = lcd->cursor->x;
        lcd->last_cy = lcd->cursor->y;
    }
    if(lcd->cursor->yoff > n - 1) lcd->cursor->yoff = n - 1;
    if(lcd->cursor->yoff < 0) lcd->cursor->yoff = 0;

    VirtualWindow area = view_area(lcd);
    VirtualWindow text = view_text_area(lcd);
    int gutter = text.x - area.x;
    int yoff = lcd->cursor->yoff;
    int rows = n - yoff;
    if(rows > text.height) rows = text.height;
    if(rows < 0) rows = 0;

    // clear the area, split views draw next to each other
    attrset(COLOR_PAIR(COLOR_PAIR_DEFAULT));
    for(int r = 0; r < area.height; r++){
        mvhline(area.y + r, area.x, ' ', area.width);
    }

    render_rows_prepare(lcd, yoff, rows);
    START_PROFILING();
    if(lcd->query && lcd->tree){
        render_syntax_tree_sitter(lcd, yoff, rows);
    }else{
        render_syntax_generic(lcd, yoff, rows);
    }
    END_PROFILING("syntax");
    render_diagnostics(lcd, yoff, rows);
    render_search_matches(lcd, yoff, rows);
    render_brackets(lcd, yoff, rows);
    render_selection(lcd, yoff, rows);

    for(int r = 0; r < text.height; r++){
        int br = yoff + r;
        if(gutter > 0) render_gutter(lcd, &area, gutter, r, br, n);
        if(r < rows) render_line(lcd, &text, r, br);
    }

    layer_code_render_completion_window(ccode, layer);
    END_PROFILING("rendering");
}

void destroy_code_render(void){
    for(size_t i = 0; i < arrlenu(render_rows); i++) arrfree(render_rows[i]);
    arrfree(render_rows);
    render_rows = NULL;
    arrfree(render_captures);
    arrfree(render_row_severity);
    if(render_query_cursor){
        ts_query_cursor_delete(render_query_cursor);
        render_query_cursor = NULL;
    }
}

#endif // _H_CODE_RENDER
