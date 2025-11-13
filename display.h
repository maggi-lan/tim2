#ifndef DISPLAY_H
#define DISPLAY_H

#include "editor.h"

// ========== Terminal control ==========

// Initialize terminal in raw mode for immediate input
void term_init(void);

// Restore terminal to original state
void term_cleanup(void);

// Clear entire screen
void term_clear(void);

// Move cursor to specific row and column
void term_move_cursor(int row, int col);

// Hide cursor (cleaner display during rendering)
void term_hide_cursor(void);

// Show cursor
void term_show_cursor(void);

// ========== Display functions ==========

// Main display function - renders entire editor
void display_editor(EditorState *editor);

// Render status bar at bottom of screen
void display_status_bar(EditorState *editor, int rows, int cols);

// Render text content area
void display_content(EditorState *editor, int rows, int cols);

// ========== Terminal size ==========

// Get current terminal dimensions
void get_terminal_size(int *rows, int *cols);

// ========== Helper functions for tab handling ==========

// Calculate display width of a character (4 for tab, 1 for others)
int char_display_width(char c);

// Calculate display column from character position in string
int get_display_col(char *str, int char_pos);

// Calculate display column from rope position on a line
int get_display_col_from_rope(EditorState *editor, int line, int char_col);

#endif
