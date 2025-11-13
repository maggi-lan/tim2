#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rope.h"


// Returns true if a given node is a leaf node, else false
bool is_leaf(RopeNode *node) {
	// Edge case when node is NULL
	if (node == NULL)
		return false;

	// Returns true when no children is available, else false
	if ((node->left == NULL) && (node->right == NULL))
		return true;
	else
		return false;
}


// Returns height of a given node (returns 0 if NULL pointer is passed)
int node_height(RopeNode *node) {
	if (node != NULL)
		return node->height;
	else
		return 0;  // returns 0 if node is NULL
}


// Returns the length of a given string (returns 0 if NULL string is passed)
int string_length(char *str) {
	if (str != NULL)
		return strlen(str);
	else
		return 0;  // returns 0 if node is NULL
}


// Returns the number '\n's in a string
int count_newlines(char *str) {
	// Edge case when str is NULL
	if (str == NULL)
		return 0;

	// Iteratively count the '\n's
	int count = 0;
	for (int i = 0; str[i] != '\0'; i++)
		if (str[i] == '\n')
			count++;

	return count;
}


// Recomputes total_len, weight, height and newlines of a node
void update_metadata(RopeNode *node) {
	// Edge case when node is NULL
	if (node == NULL)
		return;

	// CASE 1: node = leaf node
	if (is_leaf(node)) {
		node->total_len = string_length(node->str);
		node->weight = node->total_len;              // weight of a leaf node = strlen(node->str)

		node->height = 1;                            // height of a leaf node is 1

		node->newlines = count_newlines(node->str);  // calculates the count of newlines in node->str
	}

	// CASE 2: node = internal node
	else {
		// total_len of internal node = sum of total_len of left & right nodes
		int left_len = node->left ? node->left->total_len : 0;
		int right_len = node->right ? node->right->total_len : 0;
		node->total_len = left_len + right_len;

		// Weight of internal node = total length of characters in the left subtree
		node->weight = left_len;

		// Calculates the height based on the heights of the node's children
		node->height = 1 + MAX(node_height(node->left), node_height(node->right));

		// newline = sum of number of newlines in left & right nodes
		node->newlines = 0;
		if (node->left)
			node->newlines += node->left->newlines;
		if (node->right)
			node->newlines += node->right->newlines;
	}
}


// Allocates memory for a string, copies the input to it and returns the new string
char *string_copy(char *src) {
	char *dst = malloc(string_length(src) + 1);  // Space for length plus null
	// If malloc fails
	if (dst == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	strcpy(dst, src);                      // Copy the characters
	return dst;                            // Return the new string
}


// Allocates memory for a string, copies a substring (of length n) of the input and returns the new string
char *substr(char *start, int n) {
	// Edge case
	if (start == NULL)
		return NULL;
	if (n > string_length(start)) {
		n = string_length(start);
	}

    char *dst = malloc(n + 1);  // Space for length plus null
	// If malloc fails
    if (dst == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	strncpy(dst, start, n);  // Copy the characters
	dst[n] = '\0';           // Add null terminator
	return dst;              // Return the new string
}


// Allocates a new rope node, sets metadata and returns it
RopeNode *create_leaf(char *text) {
	RopeNode *node = calloc(1, sizeof(RopeNode));	
	// If calloc fails
	if (node == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	// Set metadata
	node->str = string_copy(text);  // allocates & copies text into node->str
	update_metadata(node);          // update the metadata of the node

	return node;
}


// Combines two subtrees and returns the root of the concatenated tree
// NOTE: concat() rebalances just the new concatenated subtree, not the whole tree
// NOTE: don't forget to rebalance the above the subtree after using concat()
RopeNode *concat(RopeNode *left_subtree, RopeNode *right_subtree) {
	// Edge cases
	if (left_subtree == NULL)
		return right_subtree;
	if (right_subtree == NULL)
		return left_subtree;

	// Calculate skew
	int skew = node_height(right_subtree) - node_height(left_subtree);

	// CASE-1: There isn't much height difference between left & height
	// Create a new parent node and attach left & right subtree as its children
	if (skew >= -1 && skew <= 1) {
		// Create an internal node whose children would be left & right
		RopeNode *node = calloc(1, sizeof(RopeNode));

		// If calloc fails
		if (node == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}

		// Update the new internal/parent node
		node->left = left_subtree;
		node->right = right_subtree;
		update_metadata(node);

		// Set the parent pointers of left & right subtree
		left_subtree->parent = node;
		right_subtree->parent = node;

		// Return the new internal/parent node
		return node;
	}

	// CASE-2: Right subtree is heavier: attach left subtree deep in left spine of right subtree
	if (skew >= 2) {
		// Recurse down the left spine of right subtree to find the perfect spot for concatenating (|skew| < 1)
		right_subtree->left = concat(left_subtree, right_subtree->left);

		// Update the parent pointer of right_subtree->left
		if (right_subtree->left)
			right_subtree->left->parent = right_subtree;

		// Update metadata & rebalance the node and return the rebalanced root
		update_metadata(right_subtree);
		return rebalance(right_subtree);
	}

	// CASE-3: Left subtree is heavier: attach right subtree deep in right spine of left subtree
	if (skew <= -2) {
		// Recurse down the right spine of left subtree to find the perfect spot for concatenating (|skew| < 1)
		left_subtree->right = concat(left_subtree->right, right_subtree);

		// Update the parent pointer of left_subtree->right
		if (left_subtree->right)
			left_subtree->right->parent = left_subtree;

		// Update metadata & rebalance the node and return the rebalanced root
		update_metadata(left_subtree);
		return rebalance(left_subtree);
	}

	// concat() should never reach here but this silences warnings
	return NULL;
}


// Splits a tree into two parts at a given index recursively and concatenates to rebuild the trees
// 'left' and 'right' are the resulting subtrees
void split(RopeNode *node, int idx, RopeNode **left, RopeNode **right) {
	// Edge case: node is NULL
	if (node == NULL) {
		*left = NULL;
		*right = NULL;
		return;
	}

	// BASE CASE: node is a leaf node
	if (is_leaf(node)) {
		int len = string_length(node->str);

		// Everything to the right
		if (idx <= 0) {
			*left = NULL;
			*right = node;
		}

		// Everything to the left
		else if (idx >= len) {
			*left = node;
			*right = NULL;
		}

		// Split the leaf into two leaves
		else {
			// Slicing the leaf
			char *left_part = substr(node->str, idx);
			char *right_part = substr(node->str + idx, len - idx);

			// Creating new leaves with the slices
			*left = create_leaf(left_part);
			*right = create_leaf(right_part);

			// Free memory
			free(left_part);
			free(right_part);
			free(node->str);
			free(node);
		}

		return;
	}

	// CASE-1: required index is in the left subtree
	if (idx < node->weight) {
		RopeNode *L;  // left split of the left subtree
		RopeNode *R;  // right split of the left subtree

		// Split the left subtree
		split(node->left, idx, &L, &R);

		// concatenate the right portion of the split of left subtree with right portion of current split
		*right = concat(R, node->right);

		// Update 'left' with the left portion of the split of left subtree
		*left = L;
	}

	// CASE-2: required index is in the right subtree
	else {
		RopeNode *L;  // left split of the right subtree
		RopeNode *R;  // right split of the right subtree

		// Split the right subtree
		split(node->right, idx - node->weight, &L, &R);  // NOTE: index changes when we recurse to right subtree

		// concatenate the left portion of the split of right subtree with left portion of current split
		*left = concat(node->left, L);

		// Update 'right' with the right portion of the split of right subtree
		*right = R;
	}

	// Free the old internal node
	free(node);
}


// Builds a rope from a string
RopeNode *build_rope(char *text) {
	// Edge case
	if (text == NULL)
		return NULL;

	int len = string_length(text);
	RopeNode *root = NULL;

	// Iteratively create leaves and concatenate to the root
	for (int i = 0; i < len; i += CHUNK_SIZE) {
		char *buffer = substr(text + i, CHUNK_SIZE);  // copies a substring
		RopeNode *leaf = create_leaf(buffer);         // creates a leaf
		root = concat(root, leaf);                    // concatenate with root
		free(buffer);                                 // free the substring
	}

	// Return the root of the rope
	return root;
}

// Inserts a string at a given index
// Splits the rope into two at the given index and concatenates a new rope in between
// Returns the new root
RopeNode *insert_at(RopeNode *root, int idx, char *text) {
	// Edge case
	if (root == NULL)
		return build_rope(text);
	if (text == NULL)
		return root;

	// Index handling
	if (idx < 0)
		idx = 0;
	else if (idx > root->total_len)
		idx = root->total_len;

	// Split the rope
	RopeNode *left, *right;
	split(root, idx, &left, &right);

	// Build the middle rope
	RopeNode *mid = build_rope(text);

	// Concatenate the ropes
	RopeNode *result;
	result = concat(left, mid);
	result = concat(result, right);

	// Return the resulting rope
	return result;
}


// Delete a string of certain length at a given index
// Splits the rope at two points and concats the left most and right most ropes
// NOTE: root = root node of the rope
// NOTE: start = starts deleting from this index
// NOTE: len = length of text to be deleted
// Returns the new root
RopeNode *delete_at(RopeNode *root, int start, int len) {
	// Edge case
	if (root == NULL || len <= 0)
		return root;

	// Clamp start and len
	if (start < 0)
		start = 0;
	if (start >= root->total_len)
		return root;  // nothing to delete
	if (start + len > root->total_len)
		len = root->total_len - start;

	// Split at 'start' -> left + mid
	RopeNode *left = NULL;
	RopeNode *mid = NULL;
	split(root, start, &left, &mid);

	// Split at 'mid' at 'len' -> mid + right
	RopeNode *right = NULL;
	split(mid, len, &mid, &right);

	// Delete 'mid'
	free_rope(mid);
	mid = NULL;

	// Rebuild the rope with 'left' & 'right'
	RopeNode *result = concat(left, right);
	result = rebalance(result);

	// Return the result
	return result;
}


// Recursively frees all nodes and their strings in the rope
void free_rope(RopeNode *root) {
	// BASE-CASE
	if (root == NULL)
		return;

	// Free children first: Post Order
	free_rope(root->left);
	free_rope(root->right);

	// Free string if root is leaf
	if (root->str != NULL)
		free(root->str);

	// Set everything to NULL
	root->left = NULL;
	root->right = NULL;
	root->parent = NULL;
	root->str = NULL;

	// Free the node
	free(root);
}


// Loads the file into a rope
RopeNode *load_file(char *filename) {
	FILE *fp = fopen(filename, "r");  // open the file in read mode
	// Error handling
	if (!fp) {
        perror("Error opening file");
        return NULL;
    }

    RopeNode *root = NULL;              // root of the whole rope
    char buffer[CHUNK_SIZE + 1] = {0};  // buffer to read chunks (of fixed size) from the file

	// Read the file in chunks [fread() loads the chunk of text into buffer]
	int n;
	while ((n = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {  // fread() returns the number of characters that were read
		buffer[n] = '\0';                                 // terminate buffer with null character
		RopeNode *leaf = create_leaf(buffer);             // create a leaf with the buffer
		root = concat(root, leaf);                        // append the leaf to the tree
    }

    fclose(fp);
    return root;
}


// Writes rope content to file recursively
void write_rope_to_file(RopeNode *node, FILE *fp) {
	// Base condition-1: NULL is reached
	if (node == NULL)
		return;

	// Base condition-2: leaf is reached
	if (is_leaf(node)) {
		if (node->str != NULL)
			fprintf(fp, "%s", node->str);     // appends the string to the file
		return;
	}

	// Recurse to children
	write_rope_to_file(node->left, fp);
	write_rope_to_file(node->right, fp);
}


// Saves the rope to a file
bool save_file(RopeNode *root, char *filename) {
	// Edge case
	if (filename == NULL)
		return false;

	// Open file
	FILE *fp = fopen(filename, "w");
	if (!fp) {
		perror("Error saving file");
		return false;
	}

	// Use the recursive helper function to write to the file
	write_rope_to_file(root, fp);
	fclose(fp);

	return true;
}


// Returns the height difference between left and right children of a node
int get_skew(RopeNode *node) {
	// Edge case: node is NULL
	if (node == NULL)
		return 0;

	return node_height(node->right) - node_height(node->left);
}


// Performs a right rotation and returns the root of the rotation
RopeNode *rotate_right(RopeNode *node) {
	/*
	# initially:
         y
        / \
       x  [C]
      / \
    [A] [B]

	# after right-rotation:
         x
        / \
      [A]  y
          / \
        [B] [C]
	*/

	// Edge case: y is NULL or x is NULL
	if (node == NULL || node->left == NULL)
		return NULL;

	RopeNode *y = node;
	RopeNode *x = y->left;
	RopeNode *B = x->right;

	// Shift the parent of y to become the parent of x
	RopeNode *parent = node->parent;
	y->parent = NULL;
	x->parent = parent;
	if (parent != NULL) {
		if (parent->left == y)
			parent->left = x;
		else
			parent->right = x;
	}

	// Shift y to be the right child of x
	x->right = y;
	y->parent = x;

	// Move B
	if (B != NULL) {
		y->left = B;
		B->parent = y;
	}

	// Update height, weight, total_len and newlines for x & y
	update_metadata(y);
	update_metadata(x);

	return x;
}


// Performs a left rotation and returns the root of the rotation
RopeNode *rotate_left(RopeNode *node) {
	/*
	# initially:
         x
        / \
      [A]  y
          / \
        [B] [C]

	# after left-rotation:
         y
        / \
       x  [C]
      / \
    [A] [B]
	*/

	// Edge case: x is NULL or y is NULL
	if (node == NULL || node->right == NULL)
		return NULL;

	RopeNode *x = node;
	RopeNode *y = x->right;
	RopeNode *B = y->left;

	// Shift the parent of x to become the parent of y
	RopeNode *parent = node->parent;
	x->parent = NULL;
	y->parent = parent;
	if (parent != NULL) {
		if (parent->left == x)
			parent->left = y;
		else
			parent->right = y;
	}

	// Shift x to be the left child of y
	y->left = x;
	x->parent = y;

	// Move B
	if (B != NULL) {
		x->right = B;
		B->parent = x;
	}

	// Update height, weight, total_len and newlines for x & y
	update_metadata(x);
	update_metadata(y);

	return y;
}


// Performs AVL balancing and returns the root of the subtree on which the balancing is done on
RopeNode *rebalance(RopeNode *node) {
	// Edge case: node is NULL
	if (node == NULL)
		return NULL;

	update_metadata(node);      // update node meta data before proceeding
	int skew = get_skew(node);  // no need to rebalance if skew is either -1, 0 or 1

	// If right side is heavier
	if (skew == 2) {
		int right_skew = get_skew(node->right);  // right_skew = skew of the right node

		// CASE 1: right_skew > -1: one left rotation on the root node
		if (right_skew == 1 || right_skew == 0) {
			RopeNode *result = rotate_left(node);
			update_metadata(result);
			return result;
		}

		// CASE 2: right_skew = -1: one right rotation on the right node + one left rotation on the root node
		else if (right_skew == -1) {
			update_metadata(rotate_right(node->right));
			RopeNode *result = rotate_left(node);
			update_metadata(result);
			return result;
		}
	}

	// If left side is heavier
	else if (skew == -2) {
		int left_skew = get_skew(node->left);

		// CASE 1: left_skew < 1: one right rotation on the root node
		if (left_skew == -1 || left_skew == 0) {
			RopeNode *result = rotate_right(node);
			update_metadata(result);
			return result;
		}

		// CASE 2: left_skew = 1: one left rotation on the left node + one right rotation on the root node
		else if (left_skew == 1) {
			update_metadata(rotate_left(node->left));
			RopeNode *result = rotate_right(node);
			update_metadata(result);
			return result;
		}
	}

	// If skew = -1, 0, 1 or anything else
	return node;
}


// Prints all the text in a rope using recursion (useful for debugging)
void print_text(RopeNode *node) {
	// Return void if node is NULL
	if (node == NULL)
		return;

	// CASE 1: node = leaf node
	if (is_leaf(node))
		printf("%s", node->str);

	// CASE 2: node = internal node
	else {
		print_text(node->left);   // recurse to the left subtree
		print_text(node->right);  // recurse to the right subtree
	}
}


// Prints the tree structure (useful for debugging)
void print_tree(RopeNode *root) {
	printf("\n========== ROPE TREE DUMP ==========\n");
	if (root == NULL)
		printf("(empty tree)\n");
	else
		print_tree_rec(root, 0, '*');
	printf("====================================\n\n");
}


// Recursive helper function for print_tree()
void print_tree_rec(RopeNode *node, int depth, char branch) {
	if (node == NULL)
		return;

	// Indentation based on depth
	for (int i = 0; i < depth; i++)
		printf("    ");

	// Print branch direction (root = '*')
	if (depth == 0)
		printf("* ");
	else if (branch == 'L')
		printf("L── ");
	else if (branch == 'R')
		printf("R── ");

	// Print node metadata
	printf("[%p] h=%d w=%d len=%d nl=%d ",
		   (void *)node, node->height, node->weight, node->total_len, node->newlines);

	// Leaf preview
	if (node->str != NULL) {
		printf("leaf=\"");
		for (int i = 0; i < 20 && node->str[i] != '\0'; i++) {
			if (node->str[i] == '\n')
				printf("\\n");
			else
				putchar(node->str[i]);
		}
		if (strlen(node->str) > 20)
			printf("...");
		printf("\" ");
	}

	// Parent pointer
	printf(" parent=%p\n", (void *)node->parent);

	// Recursive printing
	print_tree_rec(node->left,  depth + 1, 'L');
	print_tree_rec(node->right, depth + 1, 'R');
}


// Returns the character at a given index using a recursive algorithm
char char_at(RopeNode *root, int idx) {
	// Edge case
    if (root == NULL || idx < 0 || idx >= root->total_len)
        return '\0';

	// BASE CASE
    if (is_leaf(root)) {
        if (idx < string_length(root->str))
            return root->str[idx];
        return '\0';
    }

	// RECURSIVE CASE-1: recurse to the left tree
    if (idx < root->weight)
        return char_at(root->left, idx);

	// RECURSIVE CASE-2: recurse to the right tree
    else
        return char_at(root->right, idx - root->weight);
}


// Returns the index of the start of a line
int get_line_start(RopeNode *root, int line) {
	// Edge cases
    if (root == NULL || line < 0)
        return 0;
    if (root->total_len == 0)
        return 0;

    int pos = 0;
    int current_line = 0;

	// Iteratively find the start of the line
    while (pos < root->total_len && current_line < line) {
        if (char_at(root, pos) == '\n')
            current_line++;
        pos++;
    }

	// Return the index
    return pos;
}


// Returns the length of a line
int get_line_length(RopeNode *root, int line) {
	// Edge cases
    if (root == NULL)
        return 0;
    if (root->total_len == 0)
        return 0;

    int start = get_line_start(root, line);
    int len = 0;

	// Iteratively calculate the length
    while (start + len < root->total_len) {
        char c = char_at(root, start + len);
        if (c == '\n')
            break;
        len++;
    }

	// Return the length
    return len;
}


// Returns the count of total number of lines in a rope
int count_total_lines(RopeNode *root) {
    if (root == NULL || root->total_len == 0)
        return 1;
    return root->newlines + 1;
}
