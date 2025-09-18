#ifndef _H_DEFINE
#define _H_DEFINE
// Some keys are not define in curses.h or arent right
#define CUSTOM_KEY_BACKSPACE 8
#define CUSTOM_KEY_ENTER 13
#define CUSTOM_KEY_ESCAPE 27

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

typedef enum {LAYER_CODE=0, LAYER_CONSOLE} LayerType;
typedef enum {TOKEN_INVALID = 0, TOKEN_COMMAND, TOKEN_STRING, TOKEN_INTEGER, TOKEN_UNKNOWN} ConsoleTokenType;
typedef enum {COMMAND_INVALID = 0, COMMAND_OPEN, COMMAND_QUIT, COMMAND_GOTO, COMMAND_SAVE, COMMAND_WRITE, COMMAND_CHANGE_NAME, COMMAND_SYS} CommandType;

static const bool is_whitespace[256] = {
  [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1
};

typedef struct {
	ConsoleTokenType type;
	CommandType command_type;
	union {
		struct {
			char* start;
			int32_t size;
		} string;
		int32_t integer;
	};
} Token;

typedef struct {
	Token* tokens;
	char* string_storage;
} TokenizationOutput;

typedef struct {
	char* source;
	size_t index;
} TokenizerState;

typedef struct {
    char* filename;
    bool saved;
    char** code_buffer;
} LayerCodeData;


typedef struct {
    char* console_buffer;
    int console_buffer_x;
} LayerConsoleData;


typedef struct {
    LayerType type;
    bool consume_input;
    void* layer_data;
} Layer;


typedef struct  {
    int x;
    int y;
    int xoff;
    int yoff;
} Cursor;


typedef struct {
    Cursor* cursor;
    Layer** layers;
} CCode;


// global flags
bool RUNNING = true;
bool CLOSE_CONSOLE = false;

#endif