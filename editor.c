#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"

/**
 * Create a new editor state
 * Initializes all fields and loads the specified file
 */
EditorState *editor_create(char *filename) {
    // Allocate memory for editor state
    EditorState *editor = calloc(1, sizeof(EditorState));
    if (!editor) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    // Copy filename if provided
    editor->filename = filename ? string_copy(filename) : NULL;

    // Load file into rope if filename exists
    editor->rope = filename ? load_file(filename) : NULL;

    // If file is empty or doesn't exist, create empty rope
    if (!editor->rope) {
        editor->rope = build_rope("");
    }

    // Initialize cursor at top-left
    editor->cursor_line = 0;
    editor->cursor_col = 0;
    editor->top_line = 0;

    // File starts unmodified
    editor->modified = false;

    // Start in NORMAL mode
    editor->mode = MODE_NORMAL;

    // Clear insert buffer
    editor->insert_buffer[0] = '\0';
    editor->insert_buffer_len = 0;
    editor->insert_start_pos = 0;

    // Reset delete counter
    editor->delete_count = 0;

    return editor;
}

/**
 * Free all editor resources
 */
void editor_free(EditorState *editor) {
    if (!editor)
        return;

    // Free rope structure
    if (editor->rope)
        free_rope(editor->rope);

    // Free filename string
    if (editor->filename)
        free(editor->filename);

    // Free editor state itself
    free(editor);
}

/**
 * Get absolute character position of cursor in rope
 * Converts (line, column) to single index
 */
int editor_get_cursor_position(EditorState *editor) {
    if (!editor || !editor->rope)
        return 0;

    // Handle empty rope
    if (editor->rope->total_len == 0)
        return 0;

    // Get starting position of current line
    int pos = get_line_start(editor->rope, editor->cursor_line);

    // Add column offset
    pos += editor->cursor_col;

    // Clamp to valid range
    if (pos > editor->rope->total_len)
        pos = editor->rope->total_len;

    return pos;
}

/**
 * Get length of line where cursor is currently positioned
 */
int editor_get_current_line_length(EditorState *editor) {
    if (!editor || !editor->rope)
        return 0;

    if (editor->rope->total_len == 0)
        return 0;

    return get_line_length(editor->rope, editor->cursor_line);
}

/**
 * Clamp cursor to valid position within document bounds
 * Prevents cursor from going out of bounds
 */
void editor_clamp_cursor(EditorState *editor) {
    if (!editor || !editor->rope)
        return;

    // Get total lines in document
    int total_lines = count_total_lines(editor->rope);

    // Ensure at least 1 line for empty files
    if (total_lines == 0)
        total_lines = 1;

    // Clamp line number
    if (editor->cursor_line < 0)
        editor->cursor_line = 0;
    if (editor->cursor_line >= total_lines)
        editor->cursor_line = total_lines - 1;

    // Get length of current line
    int line_len = editor_get_current_line_length(editor);

    // Clamp column to line length
    if (editor->cursor_col < 0)
        editor->cursor_col = 0;
    if (editor->cursor_col > line_len)
        editor->cursor_col = line_len;
}

/**
 * Move cursor up one line
 * Preserves column if possible, otherwise clamps to line length
 */
void editor_move_up(EditorState *editor) {
    if (editor->cursor_line > 0) {
        editor->cursor_line--;
        editor_clamp_cursor(editor);  // Adjust column if new line is shorter
    }
}

/**
 * Move cursor down one line
 * Preserves column if possible, otherwise clamps to line length
 */
void editor_move_down(EditorState *editor) {
    int total_lines = count_total_lines(editor->rope);
    if (total_lines == 0)
        total_lines = 1;

    if (editor->cursor_line < total_lines - 1) {
        editor->cursor_line++;
        editor_clamp_cursor(editor);  // Adjust column if new line is shorter
    }
}

/**
 * Move cursor left one character
 * Wraps to end of previous line if at beginning of line
 */
void editor_move_left(EditorState *editor) {
    if (editor->cursor_col > 0) {
        // Move within current line
        editor->cursor_col--;
    } else if (editor->cursor_line > 0) {
        // Wrap to end of previous line
        editor->cursor_line--;
        editor->cursor_col = editor_get_current_line_length(editor);
    }
}

/**
 * Move cursor right one character
 * Wraps to beginning of next line if at end of line
 */
void editor_move_right(EditorState *editor) {
    int line_len = editor_get_current_line_length(editor);

    if (editor->cursor_col < line_len) {
        // Move within current line
        editor->cursor_col++;
    } else {
        // Wrap to beginning of next line
        int total_lines = count_total_lines(editor->rope);
        if (total_lines == 0)
            total_lines = 1;

        if (editor->cursor_line < total_lines - 1) {
            editor->cursor_line++;
            editor->cursor_col = 0;
        }
    }
}

/**
 * Flush insert buffer to rope structure
 * This performs the actual insertion into the rope data structure
 */
void editor_flush_insert_buffer(EditorState *editor) {
    // Nothing to flush if buffer is empty
    if (editor->insert_buffer_len == 0)
        return;

    // Null-terminate buffer
    editor->insert_buffer[editor->insert_buffer_len] = '\0';

    // Validate insert position
    if (editor->insert_start_pos < 0)
        editor->insert_start_pos = 0;
    if (editor->rope && editor->insert_start_pos > editor->rope->total_len)
        editor->insert_start_pos = editor->rope->total_len;

    // Insert entire buffer at once (efficient batched operation)
    editor->rope = insert_at(editor->rope, editor->insert_start_pos, editor->insert_buffer);

    // Clear buffer (cursor position already updated during live typing)
    editor->insert_buffer_len = 0;
    editor->insert_buffer[0] = '\0';

    // Mark as modified
    editor->modified = true;
}

/**
 * Add character to insert buffer
 * Updates cursor position for live display but doesn't modify rope yet
 */
void editor_insert_char(EditorState *editor, char c) {
    // Flush buffer if it's getting full
    if (editor->insert_buffer_len >= MAX_BUFFER_SIZE - 1) {
        editor_flush_insert_buffer(editor);
        // Reset start position after flush
        editor->insert_start_pos = editor_get_cursor_position(editor);
    }

    // Add character to buffer
    editor->insert_buffer[editor->insert_buffer_len++] = c;

    // Update cursor position for live display
    if (c == '\n') {
        // Newline: move to next line, column 0
        editor->cursor_line++;
        editor->cursor_col = 0;
    } else {
        // Regular character: advance column
        editor->cursor_col++;
    }
}

/**
 * Delete character from insert buffer (backspace in INSERT mode)
 * Removes from buffer without touching rope
 */
void editor_delete_buffer_char(EditorState *editor) {
    if (editor->insert_buffer_len > 0) {
        // Remove last character from buffer
        editor->insert_buffer_len--;
        char deleted_char = editor->insert_buffer[editor->insert_buffer_len];
        editor->insert_buffer[editor->insert_buffer_len] = '\0';

        // Update cursor based on what was deleted
        if (deleted_char == '\n') {
            // Deleted a newline: move back to previous line
            if (editor->cursor_line > 0) {
                editor->cursor_line--;

                // Find column position on previous line
                // Need to look at buffer to see where we should be
                int col = 0;
                int last_newline = -1;

                // Find last newline in remaining buffer
                for (int i = editor->insert_buffer_len - 1; i >= 0; i--) {
                    if (editor->insert_buffer[i] == '\n') {
                        last_newline = i;
                        break;
                    }
                }

                if (last_newline >= 0) {
                    // There's a newline in buffer, count chars after it
                    col = editor->insert_buffer_len - last_newline - 1;
                } else {
                    // No newline in buffer, add buffer length to original line length
                    int orig_line_len = 0;

                    if (editor->rope && editor->rope->total_len > 0 &&
                        editor->insert_start_pos >= 0 && editor->insert_start_pos <= editor->rope->total_len) {
                        // Find which line insert started on
                        int insert_line = 0;
                        int pos = 0;
                        while (pos < editor->insert_start_pos && pos < editor->rope->total_len) {
                            if (char_at(editor->rope, pos) == '\n')
                                insert_line++;
                            pos++;
                        }

                        // Get original line length up to insert point
                        if (insert_line < count_total_lines(editor->rope)) {
                            int line_start = get_line_start(editor->rope, insert_line);
                            orig_line_len = editor->insert_start_pos - line_start;
                        }
                    }

                    col = orig_line_len + editor->insert_buffer_len;
                }

                editor->cursor_col = col;
            }
        } else {
            // Regular character deletion: just move back one column
            if (editor->cursor_col > 0)
                editor->cursor_col--;
        }
    }
}

/**
 * Flush delete operations (placeholder)
 * Currently just resets counter since deletes happen immediately
 */
void editor_flush_delete_buffer(EditorState *editor) {
    if (editor->delete_count == 0)
        return;

    // Reset counter
    editor->delete_count = 0;
    editor->modified = true;
}

/**
 * Delete character from rope (backspace in DELETE mode)
 * Deletes immediately from rope structure (not buffered)
 */
void editor_delete_char(EditorState *editor) {
    // Can't delete from empty rope
    if (!editor->rope || editor->rope->total_len == 0)
        return;

    // Get current position
    int pos = editor_get_cursor_position(editor);

    // Can only delete if there's something before cursor
    if (pos > 0) {
        // Get character that will be deleted
        char prev_char = char_at(editor->rope, pos - 1);

        // If deleting newline, capture previous line length BEFORE merge
        int prev_line_end_col = 0;
        if (prev_char == '\n' && editor->cursor_line > 0) {
            prev_line_end_col = get_line_length(editor->rope, editor->cursor_line - 1);
        }

        // Perform deletion
        editor->rope = delete_at(editor->rope, pos - 1, 1);

        // Update cursor based on what was deleted
        if (prev_char == '\n') {
            // Deleted newline: lines merged, go to end of previous line
            if (editor->cursor_line > 0) {
                editor->cursor_line--;
                editor->cursor_col = prev_line_end_col;  // Use captured length
            } else {
                editor->cursor_col = 0;
            }
        } else {
            // Regular character: just move back one column
            if (editor->cursor_col > 0)
                editor->cursor_col--;
        }

        // Track deletion and mark as modified
        editor->delete_count++;
        editor->modified = true;
    }
}

/**
 * Enter INSERT mode
 * Records starting position and clears buffer
 */
void editor_enter_insert_mode(EditorState *editor) {
    editor->mode = MODE_INSERT;
    editor->insert_buffer_len = 0;
    editor->insert_buffer[0] = '\0';
    // Remember where insert mode started for batched insertion
    editor->insert_start_pos = editor_get_cursor_position(editor);
}

/**
 * Enter DELETE mode
 * Resets deletion counter
 */
void editor_enter_delete_mode(EditorState *editor) {
    editor->mode = MODE_DELETE;
    editor->delete_count = 0;
}

/**
 * Return to NORMAL mode
 * Flushes any pending operations and clamps cursor
 */
void editor_enter_normal_mode(EditorState *editor) {
    if (editor->mode == MODE_INSERT) {
        // Flush insert buffer to rope
        editor_flush_insert_buffer(editor);
    } else if (editor->mode == MODE_DELETE) {
        // Flush delete operations (currently just resets counter)
        editor_flush_delete_buffer(editor);
    }

    editor->mode = MODE_NORMAL;

    // Ensure cursor is in valid position
    editor_clamp_cursor(editor);
}

/**
 * Save current rope contents to file
 * Returns true on success, false on failure
 */
bool editor_save(EditorState *editor) {
    // Need filename to save
    if (!editor->filename)
        return false;

    // Write rope to file
    if (save_file(editor->rope, editor->filename)) {
        editor->modified = false;  // Clear modified flag
        return true;
    }

    return false;
}
