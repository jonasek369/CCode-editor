#ifndef _H_PROFILING
#define _H_PROFILING

#include <time.h>
#include <stdio.h>

//#define PROFILING 1
#if(COMPILE_PROFILING == 1)
	#define MAX_TIME_FRAMES 1024
	
	typedef struct {
	    const char* name;
	    size_t elapsed_ns;
	    size_t depth;
	} prof_timeframe;
	
	static prof_timeframe prof_frames[MAX_TIME_FRAMES];
	static size_t prof_frames_size = 0;
	
	static size_t prof_stack[MAX_TIME_FRAMES];
	static size_t depth = 0;
	
	static size_t current_ns() {
	    struct timespec ts;
	    clock_gettime(CLOCK_MONOTONIC, &ts);
	    return (size_t)ts.tv_sec * 1000000000ULL  + ts.tv_nsec;
	}
	
	
	#define START_PROFILING() do { \
	    if (depth < MAX_TIME_FRAMES) \
	        prof_stack[depth++] = current_ns(); \
	} while(0)
	
	#define END_PROFILING(label) do { \
	    if (depth > 0 && prof_frames_size < MAX_TIME_FRAMES) { \
	        prof_timeframe tf; \
	        tf.name = (label); \
	        tf.elapsed_ns = current_ns() - prof_stack[--depth]; \
	        tf.depth = depth; \
	        prof_frames[prof_frames_size++] = tf; \
	    } \
	} while(0)
	
	static void prof_print(WINDOW* win) {
	    if (prof_frames_size == 0) return;
	    int row, col;
	    getyx(stdscr, row, col);
	
	    /* Pre-compute max name length for column alignment */
	    int max_name_col = 0;
	    for (size_t i = 0; i < prof_frames_size; i++) {
	        prof_timeframe* tf = &prof_frames[i];
	        int name_col = (int)(tf->depth * 3) + (int)strlen(tf->name);
	        if (name_col > max_name_col) max_name_col = name_col;
	    }
	    int timing_col = max_name_col + 2;
	
	    size_t y = 1;
	    attron(A_BOLD);
	
	    for (size_t i = prof_frames_size; i-- > 0;) {
	        prof_timeframe* tf = &prof_frames[i];
	        size_t x = 0;
	
	        /* Draw ancestor pipe columns */
	        for (size_t d = 0; d < tf->depth; d++) {
	            mvwprintw(win, y, x, "|  ");
	            x += 3;
	        }
	
	        /* Determine if there's a sibling below this node */
	        bool has_sibling_below = false;
	        for (size_t j = i; j-- > 0;) {
	            if (prof_frames[j].depth == tf->depth) {
	                has_sibling_below = true;
	                break;
	            }
	            if (prof_frames[j].depth < tf->depth) break;
	        }
	
	        const char* branch = has_sibling_below ? "|-- " : "`-- ";
	
	        if (tf->depth == 0) {
	            mvwprintw(win, y, x, "%s", tf->name);
	        } else {
	            mvwprintw(win, y, x - 3, "%s%s", branch, tf->name);
	        }
	
	        /* Right-aligned timing column */
	        double ms = tf->elapsed_ns / 1e6;
	        int name_end = (int)(tf->depth * 3) + (int)strlen(tf->name);
	        int pad = timing_col - name_end;
	        if (pad < 1) pad = 1;
	
	        mvwprintw(win, y, name_end + pad, "%7.3f ms", ms);
	
	        if (++y >= (size_t)(LINES - 1)) {
	            wclear(win);
	            y = 1;
	        }
	    }
	
	    attroff(A_BOLD);
	    move(row, col);
	}

	static void prof_reset() {
    	prof_frames_size = 0;
    	depth = 0;
	}
#else
	#define START_PROFILING()
	#define END_PROFILING(label)
	
	static void prof_print(WINDOW* win) {}
	static void prof_reset() {}
#endif

#endif