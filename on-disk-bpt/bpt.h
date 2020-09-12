#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Set order
#define LEAF_ORDER 4 //32
#define INTERNAL_ORDER 4 //249

#define VALUE_SIZE 120
#define PAGE_SIZE 4096

// TYPES.

typedef uint64_t pagenum_t;

// Record
typedef struct record {
	uint64_t key;
	char value[VALUE_SIZE];
} record;

// Page_t
typedef struct page_t {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;
			bool is_leaf;
			int num_keys;
		};
		char page[PAGE_SIZE];
	};

} page_t;

// leaf page
typedef struct leaf_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;	// Parent page
			bool is_leaf;				// yes->1
			int num_keys;				// Number of keys
			char reserved[104];			// Reserved memory 104 bytes
			pagenum_t next_offset; 	// Right sibling page
			record pointers[LEAF_ORDER - 1];		// key(8) + value(120)
		};
		char page[PAGE_SIZE];
	};
}leaf_page;

// Internal page
typedef struct in_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t parent_offset;				// Parent page
			bool is_leaf;							// no->0
			int num_keys;							// Number of keys
			char reserved[104];                      // Reserved memory 104 bytes
			pagenum_t offsets[INTERNAL_ORDER];		// Page offset(8)
			int64_t keys[INTERNAL_ORDER - 1];		// key(8)
		};
		char page[PAGE_SIZE];
	};
} in_page;

// Free pages
typedef struct free_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t next_page_offset;			// Next free page offset
			char not_used[PAGE_SIZE - 8];			// Not used memory 4088 bytes
		};
		char page[PAGE_SIZE];
	};
} free_page;

// Header page
typedef struct header_page {
	pagenum_t offset;
	union {
		struct {
			pagenum_t free_offset;		// Free page offset
			pagenum_t root_offset;		// Root page offset
			pagenum_t num_pages;		// Number of pages
			char reserved[4072];		// Reserved memory 4072 bytes
		};
		char page[PAGE_SIZE];
	};
} header_page;

// Queue
typedef struct queue {
	pagenum_t page;					// Page's offset
	struct queue * next;			// Next page
} queue;


// GLOBALS

int fp;

extern queue * q;


// Utility

void usage(void);
void enqueue(pagenum_t new_page);
pagenum_t dequeue(void);
int height();
int path_to_root(in_page * root, in_page * n);
void print_leaves(void);
void print_tree(void);
int cut(int order);
void make_header(header_page * header);


// File Manager API

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);


// Open

int open_db(char * pathname);


// Find

leaf_page * find_leaf(uint64_t key);
char * find(uint64_t key);


// Insertion.

record make_record(uint64_t key, char * value);
page_t * make_page(void);
int get_left_index(in_page * parent, page_t * child);
page_t * insert_into_leaf(leaf_page * leaf, uint64_t key, record pointer);
page_t * insert_into_leaf_after_splitting(leaf_page * leaf, uint64_t key,
	record pointer);
page_t * insert_into_node(in_page * parent,
	int left_index, uint64_t key, in_page * right);
page_t * insert_into_node_after_splitting(in_page * old_node,
	int left_index,
	uint64_t key, in_page * right);
page_t * insert_into_parent(page_t * child, uint64_t new_key, page_t * new_child);
page_t * insert_into_new_root(page_t * left, uint64_t key, page_t * right);
page_t * start_new_tree(uint64_t key, record pointer);
int insert(uint64_t key, char * value);


// Deletion.

page_t * remove_entry_from_node(page_t * page, uint64_t key);
page_t * adjust_root();
int get_neighbor_index(page_t * page);
page_t * merge_nodes(page_t * page, page_t * neighbor, int neighbor_index, uint64_t k_prime);
page_t * delete_entry(page_t * page, uint64_t key);
page_t * replace_entry(page_t * parent, uint64_t new_k_prime);
int delete(uint64_t key);
void destroy_tree(in_page * root);


#endif /* __BPT_H__*/