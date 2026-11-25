#ifndef _H_CLIPBOARD
#define _H_CLIPBOARD

#include <assert.h>
#include <stdbool.h>

static bool clipboard_suport = false;

// The terminal, used to copy through OSC 52 when no clipboard program exists
// (works over ssh in most terminal emulators)
static FILE* tty_output = NULL;

static inline void osc52_copy(const char* text){
    if(!tty_output) return;
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = strlen(text);
    if(len > 100000) return; // terminals limit the sequence length
    fputs("\033]52;c;", tty_output);
    for(size_t i = 0; i < len; i += 3){
        unsigned v = (unsigned char)text[i] << 16;
        if(i + 1 < len) v |= (unsigned char)text[i+1] << 8;
        if(i + 2 < len) v |= (unsigned char)text[i+2];
        fputc(b64[(v >> 18) & 63], tty_output);
        fputc(b64[(v >> 12) & 63], tty_output);
        fputc(i + 1 < len ? b64[(v >> 6) & 63] : '=', tty_output);
        fputc(i + 2 < len ? b64[v & 63] : '=', tty_output);
    }
    fputs("\a", tty_output);
    fflush(tty_output);
}

#ifdef _WIN32
    #include <windows.h>
    #include <winuser.h>

    void detect_clipboard_support(){
        clipboard_suport = true;
    }

    // Returns the clipboard as an stb array ('\0' terminated, UTF-8), NULL when empty
    char* get_clipboard_content(void) {
        if (!clipboard_suport || !OpenClipboard(NULL)) {
            return NULL;
        }
        char* out = NULL;
        HANDLE cb = GetClipboardData(CF_UNICODETEXT);
        if (cb) {
            const wchar_t* wide = (const wchar_t*)GlobalLock(cb);
            if (wide) {
                int bytes = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
                if (bytes > 0) {
                    arrsetlen(out, bytes);
                    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, bytes, NULL, NULL);
                }
                GlobalUnlock(cb);
            }
        }
        CloseClipboard();
        return out;
    }

    void set_clipboard_content(const char* data) {
        if (!clipboard_suport || !data) {
            return;
        }
        // windows programs expect \r\n line breaks
        size_t len = strlen(data);
        char* crlf = malloc(len * 2 + 1);
        size_t w = 0;
        for (size_t i = 0; i < len; i++) {
            if (data[i] == '\n' && (i == 0 || data[i-1] != '\r')) crlf[w++] = '\r';
            crlf[w++] = data[i];
        }
        crlf[w] = '\0';

        int wide_len = MultiByteToWideChar(CP_UTF8, 0, crlf, -1, NULL, 0);
        if (wide_len <= 0 || !OpenClipboard(NULL)) {
            free(crlf);
            return;
        }
        EmptyClipboard();
        HGLOBAL cb = GlobalAlloc(GMEM_MOVEABLE, wide_len * sizeof(wchar_t));
        if (cb) {
            wchar_t* buffer = (wchar_t*)GlobalLock(cb);
            if (buffer) {
                MultiByteToWideChar(CP_UTF8, 0, crlf, -1, buffer, wide_len);
                GlobalUnlock(cb);
                if (!SetClipboardData(CF_UNICODETEXT, cb)) {
                    GlobalFree(cb);
                }
            } else {
                GlobalFree(cb);
            }
        }
        CloseClipboard();
        free(crlf);
    }

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CLIPBOARD_NONE = 0,
    CLIPBOARD_WAYLAND,
    CLIPBOARD_XCLIP,
    CLIPBOARD_XSEL,
    CLIPBOARD_PBCOPY,
} ClipboardTool;

static ClipboardTool clipboard_tool = CLIPBOARD_NONE;

static bool clipboard_has_program(const char* name){
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s > /dev/null 2>&1", name);
    return system(cmd) == 0;
}

void detect_clipboard_support(){
    if (getenv("WAYLAND_DISPLAY") && clipboard_has_program("wl-copy") && clipboard_has_program("wl-paste")) {
        clipboard_tool = CLIPBOARD_WAYLAND;
    } else if (getenv("DISPLAY") && clipboard_has_program("xclip")) {
        clipboard_tool = CLIPBOARD_XCLIP;
    } else if (getenv("DISPLAY") && clipboard_has_program("xsel")) {
        clipboard_tool = CLIPBOARD_XSEL;
    } else if (clipboard_has_program("pbcopy") && clipboard_has_program("pbpaste")) {
        clipboard_tool = CLIPBOARD_PBCOPY;
    } else {
        fprintf(stderr, "No clipboard program found (wl-clipboard, xclip, xsel). "
                        "Copying uses the terminal (OSC 52), pasting only works inside the editor "
                        "or with the terminal's paste.\n");
    }
    clipboard_suport = clipboard_tool != CLIPBOARD_NONE;
}

// Returns the clipboard as an stb array ('\0' terminated), NULL when unavailable
char* get_clipboard_content(void){
    const char *cmd;
    switch (clipboard_tool) {
        case CLIPBOARD_WAYLAND: cmd = "wl-paste -n 2>/dev/null"; break;
        case CLIPBOARD_XCLIP:   cmd = "xclip -selection clipboard -o 2>/dev/null"; break;
        case CLIPBOARD_XSEL:    cmd = "xsel --clipboard --output 2>/dev/null"; break;
        case CLIPBOARD_PBCOPY:  cmd = "pbpaste"; break;
        default: return NULL;
    }

    FILE *fp = popen(cmd, "r");
    if (!fp)
        return NULL;

    char *buf = NULL;
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        memcpy(arraddnptr(buf, n), chunk, n);
    }
    arrput(buf, '\0');

    pclose(fp);
    return buf;
}

void set_clipboard_content(const char *text){
    const char *cmd;
    switch (clipboard_tool) {
        case CLIPBOARD_WAYLAND: cmd = "wl-copy 2>/dev/null"; break;
        case CLIPBOARD_XCLIP:   cmd = "xclip -selection clipboard 2>/dev/null"; break;
        case CLIPBOARD_XSEL:    cmd = "xsel --clipboard --input 2>/dev/null"; break;
        case CLIPBOARD_PBCOPY:  cmd = "pbcopy"; break;
        default:
            osc52_copy(text);
            return;
    }

    FILE *fp = popen(cmd, "w");
    if (!fp){
        osc52_copy(text);
        return;
    }

    fputs(text, fp);
    pclose(fp);
}

#endif

#endif
