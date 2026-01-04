# tim2: Rope-Based Modal Text Editor

A modal text editor built in C using a rope data structure. Inspired by Vim, this editor provides three distinct modes for text editing with immediate visual feedback.

![Demo](static/demo.gif)

## Features

- **Rope Data Structure**: Efficient string operations using an AVL-balanced rope tree
- **Modal Editing**: Three modes (NORMAL, INSERT, DELETE) inspired by Vim
- **Efficient Buffer Management**: Batched rope updates for optimal performance
- **File I/O**: Load and save text files
- **Vim-like Interface**: Status bar and familiar key bindings

## Architecture

The project uses a clean multi-file architecture:

```
├── rope.h / rope.c          # Core rope data structure implementation
├── editor.h / editor.c      # Editor state and operations
├── display.h / display.c    # Terminal control and rendering
├── input.h / input.c        # Keyboard input handling
├── main.c                   # Program entry point
└── Makefile                 # Build configuration
```

## Building

Compile the editor using the included Makefile:

```bash
make
```

This will generate the `tim2` executable.

## Usage

```bash
./tim2 <filename>
```

### Modes

#### NORMAL Mode (Default)
Navigate through the document without modifying text.

**Key Bindings:**
- `h` / `←` - Move cursor left
- `j` / `↓` - Move cursor down
- `k` / `↑` - Move cursor up
- `l` / `→` - Move cursor right
- `i` - Enter INSERT mode
- `d` - Enter DELETE mode
- `s` - Save file
- `q` - Quit editor

#### INSERT Mode
Insert text at the cursor position. All typed characters are buffered and added to the rope when returning to NORMAL mode.

**Key Bindings:**
- Type characters to insert them
- `Enter` - Insert newline
- `Tab` - Insert tab
- `Backspace` - Delete from insert buffer
- `ESC` - Return to NORMAL mode (commits changes to rope)

#### DELETE Mode
Delete characters using backspace.

**Key Bindings:**
- `Backspace` - Delete character before cursor
- `ESC` - Return to NORMAL mode

### Status Bar

The status bar (bottom of screen) displays:
- Filename
- Modified indicator (`+` if unsaved changes)
- Current mode
- Cursor position (line and column)

## Rope Data Structure

The editor uses a rope data structure for efficient text manipulation:

- **Leaf Nodes**: Store text chunks (up to 128 characters)
- **Internal Nodes**: Binary tree structure with AVL balancing
- **Metadata**: Each node tracks weight, total length, height, and newline count

### Key Operations

- `insert_at(root, idx, text)` - Insert text at position (O(log n))
- `delete_at(root, start, len)` - Delete text range (O(log n))
- `split(root, idx, &left, &right)` - Split rope at position (O(log n))
- `concat(left, right)` - Concatenate two ropes (O(log n))

## Performance Features

1. **Buffered Inserts**: INSERT mode buffers keystrokes and performs a single rope update when exiting to NORMAL mode
2. **AVL Balancing**: Maintains **log n** height for consistent performance
3. **Chunked Storage**: Files are loaded in 128-byte chunks for efficient memory usage
4. **Immediate Visual Feedback**: Display shows buffer content overlaid on rope structure without expensive updates

## Technical Details

### Terminal Control

- Uses ANSI escape sequences for cursor control and screen clearing
- Raw terminal mode for immediate character input
- Dynamic terminal size detection

### Memory Management

- Proper cleanup with `free_rope()` to prevent memory leaks
- Safe string copying and substring operations
- Bounds checking throughout to prevent segmentation faults

### File Operations

- Chunked file reading (128 bytes at a time)
- Recursive tree traversal for file writing

## Requirements

- GCC or compatible C compiler
- POSIX-compatible system (Linux, macOS, BSD)
- Terminal with ANSI escape sequence support

## Limitations

- No syntax highlighting
- No undo/redo functionality
- No search/replace
- No line wrapping (lines extend beyond screen width)
- Single file editing only

## Future Enhancements

Potential improvements:
- Line wrapping
- Syntax highlighting
- Undo/redo stack
- Command mode with search and replace
- Visual selection mode

