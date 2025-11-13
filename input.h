#ifndef INPUT_H
#define INPUT_H

#include "editor.h"
#include <stdbool.h>

// Key code constants
#define KEY_ESCAPE 27       // ESC key
#define KEY_BACKSPACE 127   // Backspace key
#define KEY_ENTER 10        // Enter/newline key

// Arrow key types (detected from escape sequences)
typedef enum {
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_REGULAR  // Not an arrow key
} KeyType;

// ========== Input functions ==========

// Read a single key from stdin (blocking)
int read_key(void);

// Parse escape sequence to detect arrow keys
KeyType parse_arrow_key(int first_key);

// Handle keyboard input and update editor state
// Returns false if user wants to quit, true otherwise
bool handle_input(EditorState *editor);

#endif
