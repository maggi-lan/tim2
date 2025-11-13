#ifndef EDITOR_H
#define EDITOR_H

#include "rope.h"
#include <stdbool.h>

// Maximum size of insert buffer before flushing to rope
#define MAX_BUFFER_SIZE 4096

// Editor modes (inspired by Vim)
typedef enum {
    MODE_NORMAL,   // Navigate without editing
    MODE_INSERT,   // Insert text (buffered until ESC)
    MODE_DELETE    // Delete text with backspace
} EditorMode;

// Editor state - contains all editor data
typedef struct {
    RopeNode *rope;              // The rope data structure containing file content
    int cursor_line;             // Current line number (0-indexed)
    int cursor_col;              // Current column number (0-indexed, character position not display)
    int top_line;                // Top line currently visible on screen (for scrolling)
    char *filename;              // Name of file being edited
    bool modified;               // True if file has unsaved changes
    EditorMode mode;             // Current editor mode
    char insert_buffer[MAX_BUFFER_SIZE];  // Buffer for insert mode text (not yet in rope)
    int insert_buffer_len;       // Current length of insert buffer
    int insert_start_pos;        // Position in rope where insert mode started
    int delete_count;            // Count of deletions in delete mode
} EditorState;

// ========== Editor initialization and cleanup ==========

// Create new editor state and load file
EditorState *editor_create(char *filename);

// Free all editor resources
void editor_free(EditorState *editor);

// ========== Cursor movement ==========

// Move cursor up one line
void editor_move_up(EditorState *editor);

// Move cursor down one line
void editor_move_down(EditorState *editor);

// Move cursor left one character (wraps to previous line if at start)
void editor_move_left(EditorState *editor);

// Move cursor right one character (wraps to next line if at end)
void editor_move_right(EditorState *editor);

// Clamp cursor position to valid bounds
void editor_clamp_cursor(EditorState *editor);

// ========== Mode operations ==========

// Enter INSERT mode (start buffering insertions)
void editor_enter_insert_mode(EditorState *editor);

// Enter DELETE mode (allow backspace deletions)
void editor_enter_delete_mode(EditorState *editor);

// Return to NORMAL mode (flush buffers and update rope)
void editor_enter_normal_mode(EditorState *editor);

// ========== Insert mode operations ==========

// Add character to insert buffer
void editor_insert_char(EditorState *editor, char c);

// Flush insert buffer to rope structure
void editor_flush_insert_buffer(EditorState *editor);

// Delete character from insert buffer (backspace in INSERT mode)
void editor_delete_buffer_char(EditorState *editor);

// ========== Delete mode operations ==========

// Delete character from rope (backspace in DELETE mode)
void editor_delete_char(EditorState *editor);

// Flush delete operations (currently just resets counter)
void editor_flush_delete_buffer(EditorState *editor);

// ========== File operations ==========

// Save current rope contents to file
bool editor_save(EditorState *editor);

// ========== Helper functions ==========

// Get absolute position of cursor in rope (converts line+col to index)
int editor_get_cursor_position(EditorState *editor);

// Get length of current line
int editor_get_current_line_length(EditorState *editor);

#endif
