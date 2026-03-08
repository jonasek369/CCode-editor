#define NOB_IMPLEMENTATION
#include "nob.h"

#define WINCON 0

#define incl "A:/PDCurses"

#define lib "A:/PDCurses/wincon"

//#define lib "C:/Users/mato/Desktop/CCode-editor/PDCurses/sdl2"

//#define sdl_incl "C:/Users/mato/Downloads/SDL2-devel-2.32.0-mingw/SDL2-2.32.0/x86_64-w64-mingw32/include"
//#define sdl_lib "C:/Users/mato/Downloads/SDL2-devel-2.32.0-mingw/SDL2-2.32.0/x86_64-w64-mingw32/lib"




int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd,
        "gcc", "-m64", "-std=c99", "-g",
        "-I", incl,
        "-L", lib,
        //"-I", sdl_incl,
        //"-L", sdl_lib,
        //"-O3", "-march=native",
        "-o", "main.exe", "main.c",
        "-lpdcurses",/*"-lSDL2",*/
        "-Wall", "-Wextra", "-Wno-sign-compare"
    );
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    nob_cmd_append(&cmd, "./main.exe", "main.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}