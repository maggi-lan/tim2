#include <stdio.h>
#include <unistd.h>
#include "input.h"

/**
 * Read a single character from stdin
 * Blocking call - waits for input
 */
int read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1)
        return c;
    return -1;
}

/**
 * Parse escape sequence to detect arrow keys
 * Arrow keys send: ESC [ A/B/C/D for up/down/right/left
 */
KeyType parse_arrow_key(int first_key) {
    // Not an escape sequence
    if (first_key != KEY_ESCAPE)
        return KEY_REGULAR;

    char seq[2];

    // Read next two characters of escape sequence
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
        return KEY_REGULAR;
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
        return KEY_REGULAR;

    // Check for arrow key pattern: ESC [ <letter>
    if (seq[0] == '[') {
        switch (seq[1]) {
            case 'A': return KEY_ARROW_UP;
            case 'B': return KEY_ARROW_DOWN;
            case 'C': return KEY_ARROW_RIGHT;
            case 'D': return KEY_ARROW_LEFT;
        }
    }

    return KEY_REGULAR;
}

/**
 * Main input handler - processes keyboard input based on current mode
 * Returns false if user wants to quit, true to continue editing
 */
bool handle_input(EditorState *editor) {
    // Read one key
    int c = read_key();

    // No input available
    if (c == -1)
        return true;

    // Handle input based on current mode
    switch (editor->mode) {
        case MODE_NORMAL: {
            // Try to parse as arrow key
            KeyType key_type = parse_arrow_key(c);

            // Movement keys (vim-style hjkl or arrow keys)
            if (key_type == KEY_ARROW_UP || c == 'k') {
                editor_move_up(editor);
            }
            else if (key_type == KEY_ARROW_DOWN || c == 'j') {
                editor_move_down(editor);
            }
            else if (key_type == KEY_ARROW_LEFT || c == 'h') {
                editor_move_left(editor);
            }
            else if (key_type == KEY_ARROW_RIGHT || c == 'l') {
                editor_move_right(editor);
            }
            // Mode switching keys
            else if (c == 'i') {
                editor_enter_insert_mode(editor);
            }
            else if (c == 'd') {
                editor_enter_delete_mode(editor);
            }
            // File operations
            else if (c == 's') {
                editor_save(editor);
            }
            else if (c == 'q') {
                return false;  // Quit editor
            }
            break;
        }

        case MODE_INSERT: {
            if (c == KEY_ESCAPE) {
                // Exit insert mode, flush buffer to rope
                editor_enter_normal_mode(editor);
            }
            else if (c == KEY_BACKSPACE) {
                // Delete from buffer (not from rope yet)
                editor_delete_buffer_char(editor);
            }
            else if (c >= 32 || c == KEY_ENTER || c == '\t') {
                // Printable character, newline, or tab
                editor_insert_char(editor, c);
            }
            // Ignore other control characters
            break;
        }

        case MODE_DELETE: {
            if (c == KEY_ESCAPE) {
                // Exit delete mode
                editor_enter_normal_mode(editor);
            }
            else if (c == KEY_BACKSPACE) {
                // Delete from rope immediately
                editor_delete_char(editor);
            }
            // Ignore other keys in delete mode
            break;
        }
    }

    return true;  // Continue editing
}
