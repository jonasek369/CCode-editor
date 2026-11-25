#ifndef _H_CODE_VIEW
#define _H_CODE_VIEW

/*
    Geometry of a code layer on screen

    Buffer positions (cursor->x) are byte offsets into a line. What the user sees are
    display columns: a tab is as wide as it takes to reach the next tab stop, a UTF-8
    sequence is one character and CJK/emoji take two cells. Scrolling (cursor->xoff)
    is kept in display columns, everything else in bytes.

    A code layer draws into its view (virtual window, or the whole screen between the
    tab bar and the status bar). The left part of the view is the gutter (diagnostic
    sign + line number), the rest is the text area.
*/

// Set in main, lets drawing code read the current config (it is replaced on reload)
static CCode* global_ccode = NULL;

static inline CCodeConfig* view_config(void){
    return global_ccode ? global_ccode->config : NULL;
}

static inline int view_tab_size(void){
    CCodeConfig* c = view_config();
    return (c && c->tab_size > 0) ? (int)c->tab_size : 4;
}

/*
    UTF-8
*/

static inline int u8_seq_len(unsigned char c){
    if(c < 0x80) return 1;
    if((c & 0xE0) == 0xC0) return 2;
    if((c & 0xF0) == 0xE0) return 3;
    if((c & 0xF8) == 0xF0) return 4;
    return 1; // stray continuation byte or invalid lead, shown as one cell
}

static inline bool u8_is_cont(unsigned char c){
    return (c & 0xC0) == 0x80;
}

// Decodes the character at s (at most n bytes), returns its byte length.
// Invalid sequences decode as one byte with codepoint 0xFFFD
static int u8_decode(const char* s, int n, uint32_t* cp){
    unsigned char c = (unsigned char)s[0];
    int len = u8_seq_len(c);
    if(len == 1){
        *cp = c < 0x80 ? c : 0xFFFD;
        return 1;
    }
    if(len > n){
        *cp = 0xFFFD;
        return 1;
    }
    uint32_t v = c & (0x7F >> len);
    for(int i = 1; i < len; i++){
        unsigned char cc = (unsigned char)s[i];
        if(!u8_is_cont(cc)){
            *cp = 0xFFFD;
            return 1;
        }
        v = (v << 6) | (cc & 0x3F);
    }
    *cp = v;
    return len;
}

// Cell width of a codepoint, a small wcwidth that also works on Windows
static int u8_cp_width(uint32_t cp){
    if(cp == 0) return 1;
    if(cp < 0x20 || cp == 0x7F) return 2;          // drawn as ^X
    if(cp < 0x300) return 1;
    // combining marks
    if((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
       (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
       (cp >= 0xFE20 && cp <= 0xFE2F) || cp == 0x200B || cp == 0x200C ||
       cp == 0x200D || (cp >= 0xFE00 && cp <= 0xFE0F)){
        return 0;
    }
    // wide (East Asian, emoji)
    if((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E) ||
       (cp >= 0x3041 && cp <= 0x33FF) || (cp >= 0x3400 && cp <= 0x4DBF) ||
       (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xA000 && cp <= 0xA4CF) ||
       (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) ||
       (cp >= 0xFE30 && cp <= 0xFE4F) || (cp >= 0xFF00 && cp <= 0xFF60) ||
       (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x1F300 && cp <= 0x1F64F) ||
       (cp >= 0x1F900 && cp <= 0x1F9FF) || (cp >= 0x20000 && cp <= 0x3FFFD)){
        return 2;
    }
    return 1;
}

// Byte index of the character before byte x
static inline int u8_prev(const char* line, int x){
    if(x <= 0) return 0;
    x--;
    while(x > 0 && u8_is_cont((unsigned char)line[x])) x--;
    return x;
}

// Byte index of the character after byte x, never past len
static inline int u8_next(const char* line, int x, int len){
    if(x >= len) return len;
    uint32_t cp;
    x += u8_decode(line + x, len - x, &cp);
    return x > len ? len : x;
}

// Width of the character at byte x when it starts at display column col
static inline int view_char_width(const char* line, int x, int len, int col, int* bytes){
    if(line[x] == '\t'){
        *bytes = 1;
        int ts = view_tab_size();
        return ts - (col % ts);
    }
    uint32_t cp;
    *bytes = u8_decode(line + x, len - x, &cp);
    return u8_cp_width(cp);
}

// Display column of byte x
static int view_display_col(const char* line, int x){
    int len = (int)line_content_length(line);
    if(x > len) x = len;
    int col = 0;
    int i = 0;
    while(i < x){
        int bytes;
        col += view_char_width(line, i, len, col, &bytes);
        i += bytes;
    }
    return col;
}

// Byte index of the character covering display column target (clamped to the line end)
static int view_byte_at_col(const char* line, int target){
    int len = (int)line_content_length(line);
    int col = 0;
    int i = 0;
    while(i < len){
        int bytes;
        int w = view_char_width(line, i, len, col, &bytes);
        if(col + w > target){
            // pick the nearer edge of a wide character
            if(w > 1 && target - col >= (w + 1) / 2) return i + bytes;
            return i;
        }
        col += w;
        i += bytes;
    }
    return len;
}

// Snaps byte x onto a character boundary (and into the line)
static inline int view_clamp_x(const char* line, int x){
    int len = (int)line_content_length(line);
    if(x > len) x = len;
    if(x < 0) x = 0;
    while(x > 0 && x < len && u8_is_cont((unsigned char)line[x])) x--;
    return x;
}

/*
    Layout
*/

static inline int view_digits(int n){
    int d = 1;
    while(n >= 10){ n /= 10; d++; }
    return d;
}

int view_gutter_width(LayerCodeData* lcd){
    CCodeConfig* c = view_config();
    if(c && !c->line_numbers) return 0;
    int lines = (int)arrlen(lcd->code_buffer);
    int d = view_digits(lines < 1 ? 1 : lines);
    if(d < 3) d = 3;
    // sign column + number + space
    return 1 + d + 1;
}

// Region the layer draws into, gutter included
VirtualWindow view_area(LayerCodeData* lcd){
    if(lcd->virtual_window){
        VirtualWindow w = *lcd->virtual_window;
        // a pane right of another one leaves a column for the separator
        if(w.x > 0 && w.width > 1){
            w.x++;
            w.width--;
        }
        return w;
    }
    return default_virtual_window;
}

// Region the text is drawn in
VirtualWindow view_text_area(LayerCodeData* lcd){
    VirtualWindow w = view_area(lcd);
    int g = view_gutter_width(lcd);
    if(g > w.width - 4) g = 0; // too narrow, drop the gutter
    w.x += g;
    w.width -= g;
    if(w.width < 1) w.width = 1;
    if(w.height < 1) w.height = 1;
    return w;
}

// Screen position of the cursor, false if it is scrolled out of view
bool view_cursor_screen(LayerCodeData* lcd, int* sy, int* sx){
    VirtualWindow w = view_text_area(lcd);
    Cursor* c = lcd->cursor;
    if(c->y < 0 || c->y >= arrlen(lcd->code_buffer)) return false;
    int col = view_display_col(lcd->code_buffer[c->y], c->x);
    *sy = w.y + c->y - c->yoff;
    *sx = w.x + col - c->xoff;
    return *sy >= w.y && *sy < w.y + w.height && *sx >= w.x && *sx <= w.x + w.width;
}

// Converts a screen cell into a buffer position. Rows outside the buffer are clamped
// when clamp is set, otherwise false is returned for them
bool view_screen_to_pos(LayerCodeData* lcd, int sy, int sx, int* row, int* x, bool clamp){
    VirtualWindow w = view_text_area(lcd);
    int r = lcd->cursor->yoff + sy - w.y;
    int n = (int)arrlen(lcd->code_buffer);
    if(n == 0) return false;
    if(r < 0 || r >= n){
        if(!clamp) return false;
        r = r < 0 ? 0 : n - 1;
    }
    int col = lcd->cursor->xoff + sx - w.x;
    if(col < 0) col = 0;
    *row = r;
    *x = view_byte_at_col(lcd->code_buffer[r], col);
    return true;
}

// Scrolls so the cursor is visible, keeping scroll_margin lines around it
void view_scroll_to_cursor(LayerCodeData* lcd){
    if(!lcd->code_buffer) return;
    VirtualWindow w = view_text_area(lcd);
    Cursor* c = lcd->cursor;
    int n = (int)arrlen(lcd->code_buffer);
    if(c->y >= n) c->y = n - 1;
    if(c->y < 0) c->y = 0;

    CCodeConfig* cfg = view_config();
    int margin = cfg ? cfg->scroll_margin : 0;
    if(margin > (w.height - 1) / 2) margin = (w.height - 1) / 2;

    if(c->y - margin < c->yoff){
        c->yoff = c->y - margin;
    }else if(c->y + margin >= c->yoff + w.height){
        c->yoff = c->y + margin - w.height + 1;
    }
    int max_yoff = n - 1;
    if(c->yoff > max_yoff) c->yoff = max_yoff;
    if(c->yoff < 0) c->yoff = 0;

    int col = view_display_col(lcd->code_buffer[c->y], c->x);
    if(col < c->xoff){
        c->xoff = col;
    }else if(col >= c->xoff + w.width){
        c->xoff = col - w.width + 1;
    }
    if(c->xoff < 0) c->xoff = 0;
}

#endif // _H_CODE_VIEW
