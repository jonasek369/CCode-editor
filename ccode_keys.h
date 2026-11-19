#ifndef _H_CCODE_KEYS
#define _H_CCODE_KEYS

/*
    Keyboard input normalization

    ncurses reports some keys differently than PDCurses did, read_key()
    translates them so the rest of the editor sees the codes from defines.h
*/

typedef struct {
    const char* name; // terminfo extended capability name, as keyname() returns it
    const char* seq;  // xterm sequence, bound with define_key() if the terminfo lacks it
    int code;
} CCodeKeyDef;

static const CCodeKeyDef ccode_key_defs[] = {
    {"kUP5",  "\033[1;5A",    CTL_UP},
    {"kDN5",  "\033[1;5B",    CTL_DOWN},
    {"kRIT5", "\033[1;5C",    CTL_RIGHT},
    {"kLFT5", "\033[1;5D",    CTL_LEFT},
    {"kPRV5", "\033[5;5~",    CTL_PGUP},
    {"kNXT5", "\033[6;5~",    CTL_PGDN},
    {NULL,    "\033[1;2A",    KEY_SUP},
    {NULL,    "\033[1;2B",    KEY_SDOWN},
    // Ctrl+Tab, only sent by terminals with modifyOtherKeys / CSI u enabled
    {NULL,    "\033[27;5;9~", CTL_TAB},
    {NULL,    "\033[9;5u",    CTL_TAB},
};

#define CCODE_KEY_DEFS_COUNT (sizeof(ccode_key_defs) / sizeof(ccode_key_defs[0]))

// extended ncurses key code -> editor key code, 0 if untranslated
static int ccode_key_translation[CCODE_KEY_BASE];

void init_keys(void){
    // ncurses assigns extended key codes above KEY_MAX at runtime
    for(int code = KEY_MAX + 1; code < CCODE_KEY_BASE; code++){
        const char* name = keyname(code);
        if(name == NULL) continue;
        for(size_t i = 0; i < CCODE_KEY_DEFS_COUNT; i++){
            if(ccode_key_defs[i].name && strcmp(name, ccode_key_defs[i].name) == 0){
                ccode_key_translation[code] = ccode_key_defs[i].code;
            }
        }
    }
    for(size_t i = 0; i < CCODE_KEY_DEFS_COUNT; i++){
        if(key_defined(ccode_key_defs[i].seq) == 0){
            define_key(ccode_key_defs[i].seq, ccode_key_defs[i].code);
        }
    }
}

int read_key(void){
    int ch = getch();
    switch(ch){
        case KEY_BACKSPACE:
        case 127: {
            return CUSTOM_KEY_BACKSPACE;
        }
        // Backspace is reported above, so a raw ^H is Ctrl+Backspace
        case CUSTOM_KEY_BACKSPACE: {
            return CTL_BKSP;
        }
        case KEY_ENTER:
        case '\n': {
            return CUSTOM_KEY_ENTER;
        }
        case CUSTOM_KEY_ESCAPE: {
            // Alt+key arrives as ESC immediately followed by the key
            int next = getch();
            if(next == 'i') return ALT_I;
            if(next != ERR) ungetch(next);
            return CUSTOM_KEY_ESCAPE;
        }
        default: {
            break;
        }
    }
    if(ch > KEY_MAX && ch < CCODE_KEY_BASE && ccode_key_translation[ch]){
        return ccode_key_translation[ch];
    }
    return ch;
}

#endif // _H_CCODE_KEYS
