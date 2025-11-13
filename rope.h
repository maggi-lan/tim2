#ifndef ROPE_H
#define ROPE_H

#include <stdbool.h>

// Macros
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CHUNK_SIZE 128  // Size of text chunks stored in leaf nodes


// Rope node structure representing either an internal node or leaf node
typedef struct RopeNode {
    int weight;        // For internal: length of all text in left subtree; For leaf: length of str
    int total_len;     // Total number of characters in this subtree
    char *str;         // Text content (only for leaf nodes)
    int height;        // Height of node (for AVL balancing)
    int newlines;      // Count of '\n' characters in subtree

    struct RopeNode *left;    // Left child
    struct RopeNode *right;   // Right child
    struct RopeNode *parent;  // Parent node
} RopeNode;


// ========== Helper functions ==========

// Check if a node is a leaf node
bool is_leaf(RopeNode *node);

// Get height of a node (returns 0 if NULL)
int node_height(RopeNode *node);

// Get length of a string (returns 0 if NULL)
int string_length(char *str);

// Count number of newlines in a string
int count_newlines(char *str);

// Recompute metadata (total_len, weight, height, newlines) for a node
void update_metadata(RopeNode *node);

// Allocate and copy a string
char *string_copy(char *src);

// Extract substring of length n from start position
char *substr(char *start, int n);

// ========== Core rope operations ==========

// Create a new leaf node with given text
RopeNode *create_leaf(char *text);

// Concatenate two rope trees
RopeNode *concat(RopeNode *left, RopeNode *right);

// Split rope at given index into left and right parts
void split(RopeNode *root, int idx, RopeNode **left, RopeNode **right);

// Build a rope from a text string (creates balanced tree of chunks)
RopeNode *build_rope(char *text);

// Insert text at given index in rope
RopeNode *insert_at(RopeNode *root, int idx, char *text);

// Delete len characters starting at start index
RopeNode *delete_at(RopeNode *root, int start, int len);

// Free all memory associated with rope tree
void free_rope(RopeNode *root);

// ========== File operations ==========

// Load file into a rope structure
RopeNode *load_file(char *filename);

// Save rope contents to file
bool save_file(RopeNode *root, char *filename);

// Helper to write rope to file (recursive)
void write_rope_to_file(RopeNode *node, FILE *fp);

// ========== AVL balancing ==========

// Calculate skew (height difference) of a node
int get_skew(RopeNode *node);

// Perform right rotation on node
RopeNode *rotate_right(RopeNode *node);

// Perform left rotation on node
RopeNode *rotate_left(RopeNode *node);

// Rebalance node using AVL rotations if needed
RopeNode *rebalance(RopeNode *node);

// ========== Debug helpers ==========

// Print all text in rope (in-order traversal)
void print_text(RopeNode *node);

// Print tree structure for debugging
void print_tree(RopeNode *root);

// Recursive helper for print_tree
void print_tree_rec(RopeNode *node, int depth, char branch);

// ========== Editor utility functions ==========

// Get character at given index in rope
char char_at(RopeNode *root, int idx);

// Find position of nth newline in tree (internal use)
int find_newline_pos(RopeNode *root, int newline_idx, int offset);

// Get starting position (character index) of a line
int get_line_start(RopeNode *root, int line);

// Get length of a specific line (excluding newline)
int get_line_length(RopeNode *root, int line);

// Count total number of lines in rope
int count_total_lines(RopeNode *root);

#endif
