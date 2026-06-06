#ifndef _H_CLIPBOARD
#define _H_CLIPBOARD

#include <assert.h>

static bool clipboard_suport = false;

#ifdef _WIN32
	assert(false && "Not implemented!");
#endif

#ifdef __linux__
	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void detect_clipboard_support(){
    if (getenv("WAYLAND_DISPLAY")){
        if (system("which wl-copy > /dev/null 2>&1") != 0 ||
            system("which wl-paste > /dev/null 2>&1") != 0) {
            fprintf(stderr,
                    "Could not find wl-copy and/or wl-paste. "
                    "Install wl-clipboard to enable clipboard support.\n");
            return;
        }
        clipboard_suport = true;
    }
    else{
        if (system("which xclip > /dev/null 2>&1") != 0) {
            fprintf(stderr, "Could not find xclip on your system consider downloading it to enable clipboard support.\n");
            return;
        }
        clipboard_suport = true;
    }
}

char* get_clipboard_content(void){
    if(!clipboard_suport){
        return NULL;
    }
    const char *cmd;

    if (getenv("WAYLAND_DISPLAY"))
        cmd = "wl-paste -n";
    else
        cmd = "xclip -selection clipboard -o";

    FILE *fp = popen(cmd, "r");
    if (!fp)
        return NULL;

    char *buf = NULL;

    int c;
    while ((c = fgetc(fp)) != EOF) {
    	arrput(buf, (char)c);
    }

    arrput(buf, '\0');

    pclose(fp);
    return buf;
}

void set_clipboard_content(const char *text){
    if(!clipboard_suport){
        return;
    }
    const char *cmd;

    if (getenv("WAYLAND_DISPLAY"))
        cmd = "wl-copy";
    else
        cmd = "xclip -selection clipboard";

    FILE *fp = popen(cmd, "w");
    if (!fp)
        return;

    fputs(text, fp);
    pclose(fp);
}

#endif

#endif