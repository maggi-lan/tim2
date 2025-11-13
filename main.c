#include <stdio.h>
#include <stdlib.h>
#include "editor.h"
#include "display.h"
#include "input.h"

/**
 * Main entry point for the text editor
 * Usage: ./tim2 <filename>
 */
int main(int argc, char **argv) {
    // Check command line arguments
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Initialize editor state and load file
    EditorState *editor = editor_create(argv[1]);

    // Setup terminal for raw input mode
    term_init();

    // Main editor loop
    bool running = true;
    while (running) {
        // Render editor (content + status bar)
        display_editor(editor);

        // Process keyboard input
        // Returns false when user presses 'q' to quit
        running = handle_input(editor);
    }

    // Restore terminal to normal mode
    term_cleanup();

    // Free all editor resources
    editor_free(editor);

    return 0;
}
