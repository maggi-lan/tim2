#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include "display.h"

static struct termios orig_termios;

void term_init(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;  // Wait for at least 1 character
    raw.c_cc[VTIME] = 0; // No timeout - immediate response
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    term_hide_cursor();
}

void term_cleanup(void) {
    term_show_cursor();
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    term_clear();
    term_move_cursor(0, 0);
}

void term_clear(void) {
    printf("\033[2J");
    fflush(stdout);
}

void term_move_cursor(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
    fflush(stdout);
}

void term_hide_cursor(void) {
    printf("\033[?25l");
    fflush(stdout);
}

void term_show_cursor(void) {
    printf("\033[?25h");
    fflush(stdout);
}

void get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        *rows = 24;
        *cols = 80;
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    }
}

// Calculate display width of a character (1 for regular, 4 for tab)
int char_display_width(char c) {
    if (c == '\t')
        return 4;
    return 1;
}

// Calculate display column from character position in a string
int get_display_col(char *str, int char_pos) {
    int display_col = 0;
    for (int i = 0; i < char_pos && str[i] != '\0' && str[i] != '\n'; i++) {
        display_col += char_display_width(str[i]);
    }
    return display_col;
}

// Calculate display column from rope position on a line
int get_display_col_from_rope(EditorState *editor, int line, int char_col) {
    if (!editor || !editor->rope)
        return char_col;

    int line_start = get_line_start(editor->rope, line);
    int display_col = 0;

    for (int i = 0; i < char_col; i++) {
        char c = char_at(editor->rope, line_start + i);
        if (c == '\0' || c == '\n')
            break;
        display_col += char_display_width(c);
    }

    return display_col;
}

// Helper to get a line from the insert buffer
void get_buffer_line(char *buffer, int buffer_len, int line_offset, char *output, int max_len) {
    int current_line = 0;
    int start_idx = 0;

    // Find the start of the desired line in buffer
    for (int i = 0; i < buffer_len; i++) {
        if (buffer[i] == '\n') {
            if (current_line == line_offset) {
                break;
            }
            current_line++;
            start_idx = i + 1;
        }
    }

    // If we haven't found enough lines, return empty
    if (current_line < line_offset) {
        output[0] = '\0';
        return;
    }

    // Copy the line
    int out_idx = 0;
    for (int i = start_idx; i < buffer_len && buffer[i] != '\n' && out_idx < max_len - 1; i++) {
        output[out_idx++] = buffer[i];
    }
    output[out_idx] = '\0';
}

// Count newlines in buffer
int count_buffer_newlines(char *buffer, int buffer_len) {
    int count = 0;
    for (int i = 0; i < buffer_len; i++) {
        if (buffer[i] == '\n')
            count++;
    }
    return count;
}

void display_content(EditorState *editor, int rows, int cols) {
    if (!editor)
        return;

    // Handle empty rope
    if (!editor->rope || editor->rope->total_len == 0) {
        if (editor->mode == MODE_INSERT && editor->insert_buffer_len > 0) {
            // Display buffer content even with empty rope
            int line = 0;
            int idx = 0;
            term_move_cursor(0, 0);

            while (idx < editor->insert_buffer_len && line < rows - 1) {
                int col = 0;
                while (idx < editor->insert_buffer_len && col < cols) {
                    if (editor->insert_buffer[idx] == '\n') {
                        printf("\033[K");
                        idx++;
                        line++;
                        if (line < rows - 1)
                            term_move_cursor(line, 0);
                        break;
                    } else {
                        if (editor->insert_buffer[idx] == '\t') {
                            printf("    ");
                            col += 4;
                        } else {
                            putchar(editor->insert_buffer[idx]);
                            col++;
                        }
                        idx++;
                    }
                }
                if (col < cols && idx >= editor->insert_buffer_len) {
                    printf("\033[K");
                    line++;
                }
            }

            // Fill remaining lines with tildes
            for (int i = line; i < rows - 1; i++) {
                term_move_cursor(i, 0);
                printf("~\033[K");
            }
        } else {
            // Empty file, show tildes
            for (int i = 0; i < rows - 1; i++) {
                term_move_cursor(i, 0);
                printf("~\033[K");
            }
        }
        return;
    }

    int total_lines = count_total_lines(editor->rope);

    // Adjust top_line for scrolling
    if (editor->cursor_line < editor->top_line) {
        editor->top_line = editor->cursor_line;
    }
    if (editor->cursor_line >= editor->top_line + rows - 1) {
        editor->top_line = editor->cursor_line - rows + 2;
    }

    if (editor->top_line < 0)
        editor->top_line = 0;

    // In INSERT mode, calculate how many extra lines the buffer adds
    int buffer_newlines = 0;
    int insert_rope_line = 0;

    if (editor->mode == MODE_INSERT) {
        buffer_newlines = count_buffer_newlines(editor->insert_buffer, editor->insert_buffer_len);
        // Find which line in the rope the insert started
        if (editor->insert_start_pos >= 0 && editor->insert_start_pos <= editor->rope->total_len) {
            int pos = 0;
            insert_rope_line = 0;
            while (pos < editor->insert_start_pos) {
                char c = char_at(editor->rope, pos);
                if (c == '\n')
                    insert_rope_line++;
                pos++;
            }
        }
    }

    // Display lines
    for (int i = 0; i < rows - 1; i++) {
        int line_num = editor->top_line + i;
        term_move_cursor(i, 0);

        // In INSERT mode, check if this line is affected by the buffer
        if (editor->mode == MODE_INSERT && line_num >= insert_rope_line &&
            line_num < insert_rope_line + buffer_newlines + 1) {

            int buffer_line_offset = line_num - insert_rope_line;

            if (buffer_line_offset == 0) {
                // First line: show rope content before insert + buffer content (may span multiple display lines)
                int line_start = get_line_start(editor->rope, insert_rope_line);
                int line_end = line_start + get_line_length(editor->rope, insert_rope_line);
                int insert_offset = editor->insert_start_pos - line_start;

                int displayed = 0;

                // Content before insert point
                for (int j = 0; j < insert_offset && line_start + j < editor->rope->total_len && displayed < cols; j++) {
                    char c = char_at(editor->rope, line_start + j);
                    if (c == '\n')
                        break;
                    if (c == '\t') {
                        printf("    ");
                        displayed += 4;
                    } else if (c != '\0') {
                        putchar(c);
                        displayed++;
                    }
                }

                // Buffer content for this line (only up to first newline or end of buffer)
                for (int b = 0; b < editor->insert_buffer_len && displayed < cols; b++) {
                    if (editor->insert_buffer[b] == '\n')
                        break;
                    if (editor->insert_buffer[b] == '\t') {
                        printf("    ");
                        displayed += 4;
                    } else {
                        putchar(editor->insert_buffer[b]);
                        displayed++;
                    }
                }

                // Check if buffer contains newline - determines if we show rest of line
                int first_newline_pos = -1;
                for (int b = 0; b < editor->insert_buffer_len; b++) {
                    if (editor->insert_buffer[b] == '\n') {
                        first_newline_pos = b;
                        break;
                    }
                }

                // If no newline in buffer, show content after insert point from original line
                if (first_newline_pos == -1) {
                    for (int j = insert_offset; line_start + j < line_end && displayed < cols; j++) {
                        char c = char_at(editor->rope, line_start + j);
                        if (c == '\n')
                            break;
                        if (c == '\t') {
                            printf("    ");
                            displayed += 4;
                        } else if (c != '\0') {
                            putchar(c);
                            displayed++;
                        }
                    }
                }

                printf("\033[K");
            } else {
                // Lines created by newlines in the buffer
                char line_buffer[1024];
                get_buffer_line(editor->insert_buffer, editor->insert_buffer_len,
                               buffer_line_offset, line_buffer, sizeof(line_buffer));

                int displayed = 0;

                // Display buffer line content
                for (int j = 0; line_buffer[j] != '\0' && displayed < cols; j++) {
                    if (line_buffer[j] == '\t') {
                        printf("    ");
                        displayed += 4;
                    } else {
                        putchar(line_buffer[j]);
                        displayed++;
                    }
                }

                // If this is the last buffer line and buffer doesn't end with newline,
                // show the rest of the original line
                int last_buffer_line = buffer_line_offset;
                int actual_buffer_newlines = count_buffer_newlines(editor->insert_buffer, editor->insert_buffer_len);

                if (last_buffer_line == actual_buffer_newlines) {
                    // This is the last line from buffer
                    // Show remainder of original line
                    int line_start = get_line_start(editor->rope, insert_rope_line);
                    int line_end = line_start + get_line_length(editor->rope, insert_rope_line);
                    int insert_offset = editor->insert_start_pos - line_start;

                    for (int j = insert_offset; line_start + j < line_end && displayed < cols; j++) {
                        char c = char_at(editor->rope, line_start + j);
                        if (c == '\n')
                            break;
                        if (c == '\t') {
                            printf("    ");
                            displayed += 4;
                        } else if (c != '\0') {
                            putchar(c);
                            displayed++;
                        }
                    }
                }

                printf("\033[K");
            }
        } else {
            // Normal line display
            int actual_line = line_num;

            // Adjust line number if we're past the insert point
            if (editor->mode == MODE_INSERT && line_num > insert_rope_line) {
                actual_line = line_num - buffer_newlines;
            }

            if (actual_line >= 0 && actual_line < total_lines) {
                int line_start = get_line_start(editor->rope, actual_line);
                int line_len = get_line_length(editor->rope, actual_line);

                int displayed = 0;
                for (int j = 0; j < line_len && displayed < cols; j++) {
                    char c = char_at(editor->rope, line_start + j);
                    if (c == '\t') {
                        printf("    ");
                        displayed += 4;
                    } else if (c != '\0') {
                        putchar(c);
                        displayed++;
                    }
                }
                printf("\033[K");
            } else {
                printf("~\033[K");
            }
        }
    }
}

void display_status_bar(EditorState *editor, int rows, int cols) {
    term_move_cursor(rows - 1, 0);

    // Set inverted colors for status bar
    printf("\033[7m");

    char status[256];
    char mode_str[20];

    switch (editor->mode) {
        case MODE_NORMAL:
            strcpy(mode_str, "NORMAL");
            break;
        case MODE_INSERT:
            strcpy(mode_str, "INSERT");
            break;
        case MODE_DELETE:
            strcpy(mode_str, "DELETE");
            break;
    }

    char *filename = editor->filename ? editor->filename : "[No Name]";
    char modified_indicator = editor->modified ? '+' : ' ';

    snprintf(status, sizeof(status), " %s %c | %s | Line %d, Col %d ",
             filename, modified_indicator, mode_str,
             editor->cursor_line + 1, editor->cursor_col + 1);

    printf("%-*s", cols, status);

    // Reset colors
    printf("\033[0m");
    fflush(stdout);
}

void display_editor(EditorState *editor) {
    int rows, cols;
    get_terminal_size(&rows, &cols);

    term_clear();
    display_content(editor, rows, cols);
    display_status_bar(editor, rows, cols);

    // Position cursor
    int screen_row = editor->cursor_line - editor->top_line;

    // Clamp screen position
    if (screen_row < 0)
        screen_row = 0;
    if (screen_row >= rows - 1)
        screen_row = rows - 2;

    // Calculate display column accounting for tabs
    int display_col = 0;

    if (editor->mode == MODE_INSERT) {
        // In insert mode, we need to account for both rope and buffer
        int insert_line = 0;
        int pos = 0;

        // Find which line the insert started on
        if (editor->rope && editor->insert_start_pos >= 0 && editor->insert_start_pos <= editor->rope->total_len) {
            while (pos < editor->insert_start_pos) {
                if (char_at(editor->rope, pos) == '\n')
                    insert_line++;
                pos++;
            }
        }

        if (editor->cursor_line == insert_line) {
            // On the first line of insertion
            // Display col = rope before insert + buffer content
            int line_start = get_line_start(editor->rope, insert_line);
            int insert_offset = editor->insert_start_pos - line_start;

            display_col = get_display_col_from_rope(editor, insert_line, insert_offset);
            display_col += get_display_col(editor->insert_buffer, editor->insert_buffer_len);
        } else if (editor->cursor_line > insert_line) {
            // On a new line created in buffer
            int buffer_line_offset = editor->cursor_line - insert_line;

            // Find the start of this line in the buffer
            int newline_count = 0;
            int line_start_in_buffer = 0;
            for (int i = 0; i < editor->insert_buffer_len; i++) {
                if (editor->insert_buffer[i] == '\n') {
                    newline_count++;
                    if (newline_count == buffer_line_offset) {
                        line_start_in_buffer = i + 1;
                        break;
                    }
                }
            }

            // Calculate display column from that point
            display_col = get_display_col(editor->insert_buffer + line_start_in_buffer,
                                         editor->insert_buffer_len - line_start_in_buffer);
        }
    } else {
        // In normal or delete mode, just calculate from rope
        display_col = get_display_col_from_rope(editor, editor->cursor_line, editor->cursor_col);
    }

    if (display_col >= cols)
        display_col = cols - 1;
    if (display_col < 0)
        display_col = 0;

    term_move_cursor(screen_row, display_col);
    term_show_cursor();

    fflush(stdout);
}
