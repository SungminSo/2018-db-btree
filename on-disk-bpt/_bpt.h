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
#define LEAF_ORDER 32
#define INTERNAL_ORDER 249

#define VALUE_SIZE 120
#define PAGE_SIZE 4096

#define TMP_BUFFER_SIZE 10
#define TABLE_COUNT 10

// TYPES.

typedef uint64_t pagenum_t;

// Record
typedef struct record {
	int64_t key;
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

// Buffer
typedef struct buffer {
	union {
		page_t page;
		char frame[PAGE_SIZE];
	};
	int table_id;
	uint64_t page_num;
	bool is_dirty;
	bool is_pinned;
	struct buffer * next_of_LRU;
	struct buffer * prev_of_LRU;
} buffer;


// GLOBALS

int fp;

extern queue * q;

header_page * header[10];
free_page * freepage[10];
in_page * root[10];
queue * q = NULL;				// For print tree.
buffer * buf[TMP_BUFFER_SIZE];


// Utility

void usage(void);
void enqueue(pagenum_t new_page);
pagenum_t dequeue(void);
int height(int table_id);
int path_to_root(int table_id, in_page * root, in_page * n);
void print_leaves(int table_id);
void print_tree(int table_id);
int cut(int order);
void make_header(int table_id, header_page * header[]);


// File Manager API

// Allocate an on-disk page from the free page list.
pagenum_t file_alloc_page(int table_id);

// Free an on-disk page to the free page list.
void file_free_page(int table_id, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest).
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page.
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src);


// Buffer Manager API

// Allocate the buffer pool with the given number of entries.
int init_db(int num_buf);

// Request to allocate page.
pagenum_t buf_alloc_page(int table_id);

// Request to free page.
void buf_free_page(int table_id, pagenum_t pagenum);

// Index Manager request(get) page to Buffer Manager.
page_t * buf_get_page(int table_id, pagenum_t offset, page_t * dest);

// Index Manager put page to Buffer Manager.
void buf_put_page(int table_id, pagenum_t offset, page_t * src);


// Open

int open_table(char * pathname);


// Close

int close_table(int table_id);
int shutdown_db(int table_id);


// Find

leaf_page * find_leaf(int table_id, int64_t key);
char * find(int table_id, int64_t key);


// Insertion.

record make_record(int64_t key, char * value);
page_t * make_page(int table_id);
int get_left_index(in_page * parent, page_t * child);
page_t * insert_into_leaf(int table_id, leaf_page * leaf, int64_t key, record pointer);
page_t * insert_into_leaf_after_splitting(int table_id, leaf_page * leaf, int64_t key,
	record pointer);
page_t * insert_into_node(int table_id, in_page * parent,
	int left_index, int64_t key, in_page * right);
page_t * insert_into_node_after_splitting(int table_id, in_page * old_node,
	int left_index,
	int64_t key, in_page * right);
page_t * insert_into_parent(int table_id, page_t * child, int64_t new_key, page_t * new_child);
page_t * insert_into_new_root(int table_id, page_t * left, int64_t key, page_t * right);
page_t * start_new_tree(int table_id, int64_t key, record pointer);
int insert(int table_id, int64_t key, char * value);


// Deletion.

page_t * remove_entry_from_node(int table_id, page_t * page, int64_t key);
page_t * adjust_root(int table_id);
int get_neighbor_index(int table_id, page_t * page);
page_t * merge_nodes(int table_id, page_t * page, page_t * neighbor, int neighbor_index, int64_t k_prime);
page_t * delete_entry(int table_id, page_t * page, int64_t key);
page_t * replace_entry(int table_id, page_t * page, int64_t new_k_prime);
int delete(int table_id, int64_t key);
void destroy_tree(int table_id, in_page * root);


#endif /* __BPT_H__*/
