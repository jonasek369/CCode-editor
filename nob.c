#define NOB_IMPLEMENTATION
#include "nob.h"


#define incl "."
#define lib "A:/CCoder/wincon"


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd,
    "gcc", "-m64", "-std=c99", "-g",
    "-I", incl,
    "-o", "main.exe", "main.c",
    "A:/CCoder/wincon/pdcurses.a",
    "-Wall", "-Wextra"
);
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    nob_cmd_append(&cmd, "./main.exe");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}