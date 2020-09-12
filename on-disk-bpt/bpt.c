#include "bpt.h"

// GLOBALS.

header_page * header;
free_page * freepage;
in_page * root = NULL;
queue * q = NULL;				// For print tree.

								// Utility

void usage(void) {
	printf("Let's start on-disk B+ tree.\n");
	printf("The order of leaf page and internal page is as follows. \n"
		"leaf_order = 32, internal_order = 249. \n");
	printf("To start with input from a file of newline_delimited integers, \n"
		"start again and enter the order followed by the filename:\n"
		"bpt <inputfile> . \n");
	printf("Enter any of the following commands after the prompt > :\n"
		"\ti <k> <s>  -- Insert <k> (an integer) and <s> (a string).\n"
		"\td <k>      -- Delete key <k> and its associated value.\n"
		"\tt          -- print the B+ tree.\n"
		"\tq          -- Quit. (Or use Ctl-D.)\n");
}

// Helper function for printing the tree out.
// See print_tree.
void enqueue(pagenum_t new_page_offset) {
	queue * c;
	queue * k;

	c = (queue *)calloc(1, sizeof(queue));
	if (q == NULL) {
		q = c;
		q->page = new_page_offset;
		q->next = NULL;
	}
	else {
		k = q;
		while (k->next != NULL) {
			k = k->next;
		}
		c->page = new_page_offset;
		c->next = NULL;
		k->next = c;
	}
}

// Helper function for printing the tree out.
// See print_tree.
pagenum_t dequeue(void) {
	queue * c = q;
	q = q->next;
	c->next = NULL;
	return c->page;
}

int height() {
	int h = 0;
	in_page * c = root;

	while (!c->is_leaf) {
		file_read_page(c->offsets[0], (page_t *)c);
		h++;
	}

	return h;
}

int path_to_root(in_page * root, in_page * n) {
	int length = 0;
	in_page * child = n;

	while (child->offset != root->offset) {
		file_read_page(child->parent_offset, (page_t *)child);
		length++;
	}

	return length;
}

void print_leaves() {
	leaf_page * c = (leaf_page *)root;
	in_page * tmp;

	if (c == NULL) {
		printf("Empty tree.\n");
		return;
	}

	while (!c->is_leaf) {
		tmp = (in_page *)c;
		file_read_page(tmp->offsets[0], (page_t *)c);
	}

	while (true) {
		for (int i = 0; i<c->num_keys; i++) {
			printf("%" PRId64 "", c->pointers[i].key);
		}

		if (c->next_offset != 0) {
			printf(" | ");
			file_read_page(c->next_offset, (page_t *)c);
		}
		else
			break;
	}
	printf("\n");
}

void print_tree() {
	leaf_page * n = NULL;		// For dequeue()(leaf_page).
	in_page * c;				// For print.(in_page->keys).
	pagenum_t p;				// For enqueue(in_page).
	int new_rank = 0;
	int rank = 0;

	if (root == NULL || root->offset == 0) {
		printf("Empty tree.\n");

		return;
	}

	// Alloc memory to n,c.
	n = (leaf_page *)calloc(1, sizeof(leaf_page));
	c = (in_page *)calloc(1, sizeof(in_page));

	// Initailize queue.
	q = NULL;
	enqueue(root->offset);

	// Put page in queue one by one, print page's keys one by one.
	while (q != NULL) {
		p = dequeue();
		file_read_page(p, (page_t *)n);
		file_read_page(n->parent_offset, (page_t *)c);

		if (n->parent_offset != 0 && n->offset == c->offsets[0]) {
			new_rank = path_to_root(root, (in_page *)n);

			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}

			file_read_page(p, (page_t *)n);
		}

		if (!n->is_leaf) {
			file_read_page(n->offset, (page_t *)c);
			for (int i = 0; i < c->num_keys; i++) {
				printf("%" PRId64 " ", c->keys[i]);
			}
			printf("| ");
		}
		else {
			file_read_page(n->offset, (page_t *)n);
			for (int i = 0; i< n->num_keys; i++) {
				printf("%" PRId64 " ", n->pointers[i].key);
			}
			printf("| ");
		}

		if (!n->is_leaf) {
			file_read_page(n->offset, (page_t *)c);
			for (int i = 0; i <= c->num_keys; i++) {
				enqueue(c->offsets[i]);
			}
		}
	}
	printf("\n");

	free(n);
	free(c);
}

int cut(int order) {
	if (order % 2 == 0)
		return order / 2;
	else
		return (order / 2) + 1;
}

void make_header(header_page * header) {
	header->num_pages = 0;

	file_read_page(0, (page_t *)header);
	header->num_pages++;

	//freepage can have next page-> deal with cases.
	file_read_page(header->num_pages, (page_t *)freepage);
	freepage->next_page_offset = 0;
	header->free_offset = freepage->offset;

	header->root_offset = 0;

	file_write_page(header->offset, (page_t *)header);
}


// File Manager API

// Allocate an on-disk page from the free page list

pagenum_t file_alloc_page() {
	free_page * freepage;
	freepage = (free_page *)calloc(1, sizeof(free_page));

	file_read_page(header->free_offset, (page_t *)freepage);

	// Update next free page in header.
	header->num_pages++;
	if (freepage->next_page_offset != 0) {
		header->free_offset = freepage->next_page_offset;
	}
	else {
		header->free_offset = header->num_pages;
	}

	// Write to the on-disk page
	file_write_page(header->offset, (page_t *)header);

	return freepage->offset;
}

// Free an on-disk page to the free page list

void file_free_page(pagenum_t offset) {
	free_page * page;

	// Alloc memory to page.
	page = (free_page *)calloc(1, sizeof(free_page));
	//next = (page_t *)calloc(1, sizeof(page_t));

	file_read_page(offset, (page_t *)page);
	memset(page, 0, PAGE_SIZE);

	page->next_page_offset = header->free_offset;
	header->free_offset = page->offset;
	header->num_pages--;

	file_write_page(offset, (page_t *)page);
	file_write_page(header->offset, (page_t *)header);

	free(page);
}

// Read an on-disk page into the in-memory page structure(dest)

void file_read_page(pagenum_t offset, page_t* dest) {
	lseek(fp, PAGE_SIZE * offset, SEEK_SET);
	read(fp, dest, PAGE_SIZE);
	dest->offset = offset;
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t offset, const page_t* src) {
	lseek(fp, PAGE_SIZE * offset, SEEK_SET);
	write(fp, src, PAGE_SIZE);
}


// Open

int open_db(char * pathname) {
	// Open file.
	fp = open(pathname, O_RDWR);

	// If the file exist, upload each page's information.
	if (fp >= 0) {
		// Alloc header_page, free_page, root.
		header = (header_page *)calloc(1, sizeof(header_page));
		freepage = (free_page *)calloc(1, sizeof(free_page));
		root = (in_page *)calloc(1, sizeof(in_page));

		file_read_page(0, (page_t *)header);
		file_read_page(header->free_offset, (page_t *)freepage);
		file_read_page(header->root_offset, (page_t *)root);
	}

	// If the file does not exist, create one. And create header_page.
	else {
		fp = open(pathname, O_CREAT | O_RDWR, 0775);

		// Alloc header_page, free_page.
		header = (header_page *)calloc(1, sizeof(header_page));
		freepage = (free_page *)calloc(1, sizeof(free_page));

		// If fail to create new one, return -1
		if (fp < 0) {
			perror("failed to create new file");
			return -1;
		}

		make_header(header);
	}

	return 0;
}


// Find

leaf_page * find_leaf(uint64_t key) {
	int i;
	int flag = 0;
	in_page * c;

	// Case: the tree does not exist yet.
	if (root == NULL || root->offset == 0)
		return NULL;

	// Alloc memory to c.
	c = (in_page *)calloc(1, sizeof(in_page));

	// Read root page.
	file_read_page(root->offset, (page_t *)c);

	// Find proper leaf page.
	while (!c->is_leaf) {
		int p = 0;
		int q = c->num_keys;
		i = (p + q) / 2;
		flag = 0;

		while (key != c->keys[i]) {
			if (p >= q) {
				flag = 1;
				break;
			}
			if (key < c->keys[i]) {
				q = i;
			}
			else {
				p = i + 1;
			}
			i = (p + q) / 2;
		}
		if (flag == 1)
			file_read_page(c->offsets[i], (page_t *)c);
		else
			file_read_page(c->offsets[i + 1], (page_t *)c);
	}

	return (leaf_page *)c;
}

char * find(uint64_t key) {
	int i = 0;
	int flag = 0;
	leaf_page * c;

	c = find_leaf(key);

	// Case: the tree does not exist yet or there is no key.
	if (c == NULL)
		return NULL;

	// Case: the tree exist.
	// Find exact index of key by binary search
	int p = 0;
	int q = c->num_keys;
	i = (p + q) / 2;

	while (key != c->pointers[i].key) {
		if (p >= q) {
			flag = 1;
			break;
		}

		if (key < c->pointers[i].key) {
			q = i;
		}
		else {
			p = i + 1;
		}
		i = (p + q) / 2;
	}

	// There is no key.
	if (flag == 1)
		return NULL;

	return c->pointers[i].value;
}


// Insert

record make_record(uint64_t key, char * value) {
	record new_record;
	memset(&new_record.value, 0, VALUE_SIZE);

	// Alloc proper key and value.
	new_record.key = key;
	strcpy(new_record.value, value);

	return new_record;
}

page_t * make_page(void) {
	page_t * c;

	// Alloc memory to c/
	c = (page_t *)calloc(1, sizeof(page_t));

	// Alloc new page.
	file_read_page(file_alloc_page(), c);

	// Fail to alloc new page.
	if (c == NULL) {
		perror("Page creation");
		exit(EXIT_FAILURE);
	}

	// Alloc proper value to struct's member(suppose it is not a leaf page).
	// If new page is a leaf page, should change is_leaf to true.
	c->parent_offset = 0;
	c->is_leaf = false;
	c->num_keys = 0;

	return c;
}

int get_left_index(in_page * parent, page_t * child) {
	int left_index = 0;

	while (left_index < parent->num_keys && parent->offsets[left_index] != child->offset)
		left_index++;

	return left_index;
}

page_t * insert_into_leaf(leaf_page * leaf, uint64_t key, record pointer) {
	int insertion_point;
	insertion_point = 0;

	// Find insertion point in a leaf page.
	while (insertion_point < leaf->num_keys && leaf->pointers[insertion_point].key < key)
		insertion_point++;

	// Move the elements to right one by one until insertion_point.
	for (int i = leaf->num_keys; i > insertion_point; i--) {
		leaf->pointers[i] = leaf->pointers[i - 1];
	}

	// Put key and pointer at insertion_point.
	leaf->pointers[insertion_point] = pointer;
	leaf->num_keys++;

	// Write to on-disk page.
	file_write_page(leaf->offset, (page_t *)leaf);

	return (page_t *)leaf;
}

page_t * insert_into_leaf_after_splitting(leaf_page * leaf, uint64_t key, record pointer) {
	leaf_page * new_leaf;
	record * tmp_pointers;
	int insertion_index;
	int split;
	uint64_t new_key;
	int i, j;

	// make new leaf page to split.
	new_leaf = (leaf_page *)make_page();
	new_leaf->is_leaf = true;
	new_leaf->next_offset = 0;

	// Alloc tmp pointers.
	tmp_pointers = (record *)calloc(LEAF_ORDER, sizeof(record));

	if (tmp_pointers == NULL) {
		perror("Temporary pointers array.");
		exit(EXIT_FAILURE);
	}

	insertion_index = 0;

	// Find proper insertion_index in a existing leaf page.
	while (insertion_index < LEAF_ORDER - 1 && leaf->pointers[insertion_index].key < key)
		insertion_index++;

	// Assign values except insertion index.
	for (i = 0, j = 0; i<leaf->num_keys; i++, j++) {
		if (j == insertion_index) j++;
		tmp_pointers[j] = leaf->pointers[i];
	}

	// Assign key and pointer at insertion index.
	tmp_pointers[insertion_index] = pointer;

	// Start redistributing keys to existing page and new page half of keys.
	leaf->num_keys = 0;
	split = cut(LEAF_ORDER - 1);

	// Put to existing leaf page first half of keys.
	for (int i = 0; i<split; i++) {
		leaf->pointers[i] = tmp_pointers[i];
		leaf->num_keys++;
	}

	// Put to new leaf page rest of keys.
	for (i = split, j = 0; i<LEAF_ORDER; i++, j++) {
		new_leaf->pointers[j] = tmp_pointers[i];
		new_leaf->num_keys++;
	}

	free(tmp_pointers);

	// Change parent,next pointer.
	new_leaf->next_offset = leaf->next_offset;
	leaf->next_offset = new_leaf->offset;
	new_leaf->parent_offset = leaf->parent_offset;
	new_key = new_leaf->pointers[0].key;

	// Clean up remain pointers.
	for (int i = leaf->num_keys; i<LEAF_ORDER - 1; i++) {
		leaf->pointers[i] = make_record(0, "");
	}

	for (int i = new_leaf->num_keys; i<LEAF_ORDER - 1; i++) {
		new_leaf->pointers[i] = make_record(0, "");
	}

	// Write to on-disk page.
	file_write_page(leaf->offset, (page_t *)leaf);
	file_write_page(new_leaf->offset, (page_t *)new_leaf);

	return insert_into_parent((page_t *)leaf, new_key, (page_t *)new_leaf);
}

page_t * insert_into_node(in_page * parent, int left_index, uint64_t key, in_page * right) {
	// Move the elements to right one by one until left_index.
	for (int i = parent->num_keys; i>left_index; i--) {
		parent->keys[i] = parent->keys[i - 1];
		parent->offsets[i + 1] = parent->offsets[i];
	}

	// Put key and pointer at left_index.
	parent->keys[left_index] = key;
	parent->offsets[left_index + 1] = right->offset;
	parent->num_keys++;

	// Write to on-disk page.
	file_write_page(parent->offset, (page_t *)parent);

	return (page_t*)root;
}

page_t * insert_into_node_after_splitting(in_page * old_page, int left_index, uint64_t key, in_page * right) {
	int split;
	int k_prime;
	in_page * new_page;
	in_page * child;
	uint64_t * tmp_keys;
	pagenum_t * tmp_offsets;
	int i, j;

	// Make new node to split.
	new_page = (in_page *)make_page();

	// Alloc tmp_keys.
	tmp_keys = (uint64_t *)calloc(INTERNAL_ORDER, sizeof(uint64_t));

	if (tmp_keys == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(EXIT_FAILURE);
	}

	// Alloc tmp_offsets.
	tmp_offsets = (pagenum_t *)calloc(INTERNAL_ORDER + 1, sizeof(pagenum_t));

	if (tmp_offsets == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(EXIT_FAILURE);
	}

	// Copy the values except left_index.
	for (i = 0, j = 0; i<old_page->num_keys; i++, j++) {
		if (j == left_index) j++;
		tmp_keys[j] = old_page->keys[i];
	}

	for (i = 0, j = 0; i<old_page->num_keys + 1; i++, j++) {
		if (j == left_index + 1) j++;
		tmp_offsets[j] = old_page->offsets[i];
	}

	// Assign key and pointer at left_index
	tmp_keys[left_index] = key;
	tmp_offsets[left_index + 1] = right->offset;

	// Start redistributing keys to old node and new node half of keys.
	split = cut(INTERNAL_ORDER);
	old_page->num_keys = 0;

	// Put to old node first half of keys.
	for (i = 0; i<split - 1; i++) {
		old_page->keys[i] = tmp_keys[i];
		old_page->offsets[i] = tmp_offsets[i];
		old_page->num_keys++;
	}
	old_page->offsets[i] = tmp_offsets[i];
	k_prime = tmp_keys[split - 1];

	// Put to new node rest of keys.
	for (i = split, j = 0; i<INTERNAL_ORDER; i++, j++) {
		new_page->keys[j] = tmp_keys[i];
		new_page->offsets[j] = tmp_offsets[i];
		new_page->num_keys++;
	}
	new_page->offsets[j] = tmp_offsets[i];

	free(tmp_keys);
	free(tmp_offsets);

	// Alloc memory to child.
	child = (in_page *)calloc(1, sizeof(in_page));

	// Change parent_offset.
	new_page->parent_offset = old_page->parent_offset;
	for (i = 0; i <= new_page->num_keys; i++) {
		file_read_page(new_page->offsets[i], (page_t *)child);
		child->parent_offset = new_page->offset;
		file_write_page(child->offset, (page_t *)child);
	}

	free(child);

	// Clean up rest of keys and offsets.
	for (i = old_page->num_keys; i<INTERNAL_ORDER - 1; i++) {
		old_page->keys[i] = 0;
		old_page->offsets[i + 1] = 0;
	}

	for (i = new_page->num_keys; i<INTERNAL_ORDER - 1; i++) {
		new_page->keys[i] = 0;
		new_page->offsets[i + 1] = 0;
	}

	// Wirte to on-disk page.
	file_write_page(old_page->offset, (page_t *)old_page);
	file_write_page(new_page->offset, (page_t *)new_page);

	return insert_into_parent((page_t *)old_page, k_prime, (page_t *)new_page);
}

page_t * insert_into_parent(page_t * child, uint64_t new_key, page_t * new_child) {
	//left page = leaf, right page = new_leaf
	int left_index;
	in_page * parent;

	// Alloc memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));

	file_read_page(child->parent_offset, (page_t *)parent);

	// Case: new root.
	if (parent->offset == 0) {
		insert_into_new_root(child, new_key, new_child);

		return (page_t *)root;
	}

	// Find the parent's pointer of the left page.
	left_index = get_left_index(parent, child);

	// Case: the new key fits into the page.
	if (parent->num_keys < INTERNAL_ORDER - 1)
		return insert_into_node(parent, left_index, new_key, (in_page *)new_child);

	// Case: Harder. split a page.
	return insert_into_node_after_splitting(parent, left_index, new_key, (in_page *)new_child);
}

page_t * insert_into_new_root(page_t * left, uint64_t key, page_t * right) {
	in_page * new_root;

	new_root = (in_page *)make_page();
	new_root->keys[0] = key;
	new_root->offsets[0] = left->offset;
	new_root->offsets[1] = right->offset;
	new_root->num_keys++;
	left->parent_offset = new_root->offset;
	right->parent_offset = new_root->offset;

	root = new_root;
	header->root_offset = root->offset;

	// Write to on-disk page.
	file_write_page(left->offset, (page_t *)left);
	file_write_page(right->offset, (page_t *)right);
	file_write_page(root->offset, (page_t *)root);
	file_write_page(header->offset, (page_t *)header);

	return (page_t *)root;
}

page_t * start_new_tree(uint64_t key, record pointer) {
	leaf_page * new_root;

	// Assign new page.
	new_root = (leaf_page *)make_page();
	new_root->is_leaf = true;
	new_root->next_offset = 0;
	new_root->pointers[0] = pointer;
	new_root->num_keys++;

	root = (in_page *)new_root;
	header->root_offset = root->offset;

	// Write to on-disk page.
	file_write_page(root->offset, (page_t *)root);
	file_write_page(header->offset, (page_t *)header);

	return (page_t *)root;
}

int insert(uint64_t key, char * value) {
	record pointer;
	leaf_page * leaf;

	//check is it duplicated
	if (find(key) != NULL)
		return -1;

	//create a new record
	pointer = make_record(key, value);

	// Case: the tree does not exist yet.
	// Start a new tree.
	if (root == NULL) {
		start_new_tree(key, pointer);
		return 0;
	}

	// Case: the tree does exist.
	leaf = find_leaf(key);

	// Case: leaf has room for key and pointer.
	if (leaf->num_keys < LEAF_ORDER - 1) {
		insert_into_leaf(leaf, key, pointer);
	}
	// Case: leaf must be split.
	else {
		insert_into_leaf_after_splitting(leaf, key, pointer);
	}

	return 0;
}


// Delete

page_t * remove_entry_from_node(page_t * page, uint64_t key) {
	int i;
	int num_pointers;

	// Remove the pointer and shift other pointers accordingly.
	// First determine number of pointers.
	num_pointers = page->is_leaf ? page->num_keys : page->num_keys + 1;
	i = 0;

	// Case.(leaf or internal)
	if (page->is_leaf) {
		leaf_page * page_leaf;
		page_leaf = (leaf_page *)calloc(1, sizeof(leaf_page));
		file_read_page(page->offset, (page_t *)page_leaf);

		while (page_leaf->pointers[i].key != key)
			i++;

		for (++i; i<page_leaf->num_keys; i++) {
			page_leaf->pointers[i - 1] = page_leaf->pointers[i];
		}

		// Delete last one.
		page_leaf->pointers[page->num_keys - 1] = make_record(0, "");

		// One key fewer.
		page_leaf->num_keys--;
		page->num_keys--;

		// Write in on-disk.
		file_write_page(page->offset, (page_t *)page_leaf);

		free(page_leaf);
	}
	else {
		in_page * page_internal;
		page_internal = (in_page *)calloc(1, sizeof(in_page));
		file_read_page(page->offset, (page_t *)page_internal);

		while (page_internal->keys[i] != key)
			i++;

		for (++i; i<page_internal->num_keys; i++) {
			page_internal->keys[i - 1] = page_internal->keys[i];
			page_internal->offsets[i] = page_internal->offsets[i + 1];
		}

		// Delete last one.
		page_internal->keys[page->num_keys - 1] = 0;
		page_internal->offsets[page->num_keys] = 0;

		// One key fewer.
		page_internal->num_keys--;
		page->num_keys--;

		// Write in on-disk;
		file_write_page(page->offset, (page_t *)page_internal);

		free(page_internal);
	}

	return page;
}

page_t * adjust_root() {
	leaf_page * new_root;

	// Case: nonempty root.
	if (root->num_keys > 0)
		return (page_t *)root;

	// Allocate memory to new_root
	new_root = (leaf_page *)calloc(1, sizeof(leaf_page));

	// Case: empty root.
	// Case: it has a child(only child)
	if (!root->is_leaf) {
		file_read_page(root->offsets[0], (page_t *)new_root);
		new_root->parent_offset = 0;

		file_free_page(root->offset);

		root = (in_page *)new_root;
		header->root_offset = root->offset;
	}

	// Case: it is a leaf(no child)
	else {
		free(new_root);

		file_free_page(root->offset);
		root = NULL;
		header->root_offset = 0;
	}

	// Write in on-disk page.
	file_write_page(root->offset, (page_t *)root);
	file_write_page(header->offset, (page_t *)header);

	return (page_t *)new_root;
}

int get_neighbor_index(page_t * page) {
	int i;
	in_page * parent;

	// Allocate memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));

	file_read_page(page->parent_offset, (page_t *)parent);

	// Return the index if the key to the left of the pointer in the parent pointing to page.
	// If page is the leftmost child, this means return -1.
	int p = 0;
	int q = parent->num_keys;
	i = (p + q) / 2;

	while (page->offset != parent->offsets[i]) {
		if (page->offset < parent->offsets[i]) {
			q = i;
		}
		else {
			p = i + 1;
		}
		i = (p + q) / 2;
	}

	return i - 1;
}

page_t * merge_nodes(page_t * page, page_t * neighbor, int neighbor_index, uint64_t k_prime) {
	int i;
	page_t * tmp;
	in_page * parent;
	uint64_t new_k_prime;

	// Allocate memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));
	file_read_page(page->parent_offset, (page_t *)parent);

	// Suppose that page->num_keys == 0(right node is empty), by delaying merge.
	// Swap neighbor with page if page is on the extreme left and neighbor is to its right.
	// neighbor page means leaf side page.
	if (neighbor_index == -1) {
		tmp = page;
		page = neighbor;
		neighbor = tmp;
	}

	if (!neighbor->is_leaf) {
		in_page * tmp_page;
		in_page * tmp_neighbor;
		page_t * tmp_child;

		tmp_page = (in_page *)calloc(1, sizeof(in_page));
		tmp_neighbor = (in_page *)calloc(1, sizeof(in_page));
		tmp_child = (page_t *)calloc(1, sizeof(page_t));

		file_read_page(page->offset, (page_t *)tmp_page);
		file_read_page(neighbor->offset, (page_t *)tmp_neighbor);

		// Appends k_prime and following pointer.
		tmp_neighbor->keys[tmp_neighbor->num_keys] = k_prime;
		tmp_neighbor->num_keys++;

		// Shift tmp_page's keys and offsets to neighbor(left <- right)
		for (int i = 0; i < cut(tmp_page->num_keys + tmp_neighbor->num_keys) && tmp_page->num_keys != 0; i++) {
			// Move the key and pointer from page to neighbor.
			tmp_neighbor->keys[tmp_neighbor->num_keys] = tmp_page->keys[i];
			tmp_neighbor->offsets[tmp_neighbor->num_keys + 1] = tmp_page->offsets[i];
			tmp_neighbor->num_keys++;

			// Shift the key and pointer and remove the last one in page.
			tmp_page->keys[i] = tmp_page->keys[i + 1];
			tmp_page->offsets[i] = tmp_page->offsets[i + 1];
			tmp_page->keys[tmp_page->num_keys - 1] = 0;
			tmp_page->offsets[tmp_page->num_keys] = 0;
			tmp_page->num_keys--;
		}
		tmp_neighbor->offsets[tmp_neighbor->num_keys + 1] = tmp_page->offsets[tmp_page->num_keys];

		new_k_prime = tmp_page->keys[0];

		// Change tmp_page's offsets's parent
		for (int i = 0; i <= tmp_neighbor->num_keys; i++) {
			file_read_page(tmp_neighbor->offsets[i], (page_t *)tmp_child);
			tmp_child->parent_offset = tmp_neighbor->offset;
			file_write_page(tmp_child->offset, (page_t *)tmp_child);
		}

		file_write_page(neighbor->offset, (page_t *)tmp_neighbor);
		file_write_page(page->offset, (page_t *)tmp_page);

		free(tmp_page);
		free(tmp_neighbor);
		free(tmp_child);
	}
	else {
		leaf_page * tmp_page;
		leaf_page * tmp_neighbor;

		tmp_page = (leaf_page *)calloc(1, sizeof(leaf_page));
		tmp_neighbor = (leaf_page *)calloc(1, sizeof(leaf_page));

		file_read_page(page->offset, (page_t *)tmp_page);
		file_read_page(neighbor->offset, (page_t *)tmp_neighbor);

		// Shift tmp_page's keys and offsets to neighbor(left <- right)
		for (int i = 0; i < cut(tmp_page->num_keys + tmp_neighbor->num_keys) && tmp_page->num_keys != 0; i++) {
			if(tmp_neighbor->num_keys == 0) {
				// Move the key and pointer from page to neighbor.
				tmp_neighbor->pointers[tmp_neighbor->num_keys] = tmp_page->pointers[i];
				tmp_neighbor->num_keys++;

				// Shift the key and pointer and remove the last one in page.
				tmp_page->pointers[i] = tmp_page->pointers[i + 1];
				tmp_page->pointers[tmp_page->num_keys - 1] = make_record(0,"");
				tmp_page->num_keys--;
			}
			else {
				// Move the key and pointer from page to neighbor.
				tmp_page->pointers[i] = tmp_neighbor->pointers[tmp_neighbor->num_keys - 1];
				tmp_page->num_keys++;

				// Remove the last one in page.
				tmp_neighbor->pointers[tmp_page->num_keys - 1] = make_record(0, "");
				tmp_neighbor->num_keys--;
			}
		}

		new_k_prime = tmp_page->pointers[0].key;

		if (tmp_page->num_keys == 0) {
			tmp_neighbor->next_offset = tmp_page->next_offset;
		}

		file_write_page(neighbor->offset, (page_t *)tmp_neighbor);
		file_write_page(page->offset, (page_t *)tmp_page);

		free(tmp_page);
		free(tmp_neighbor);
	}

	// If page's num_keys is 0 free an on-disk page to the free page list.
	file_read_page(page->offset, (page_t *)page);

	if (page->num_keys == 0) {
		file_free_page(page->offset);


		// Write in on-disk page.
		file_write_page(header->offset, (page_t *)header);

		return delete_entry((page_t *)parent, k_prime);
	}
	// Else the page remains and the keys of parent were changed.
	else {
		return replace_entry((page_t *)parent, new_k_prime);
	}
}

page_t * delete_entry(page_t * page, uint64_t key) {
	page_t * neighbor;
	in_page * parent;
	int neighbor_index;
	uint64_t k_prime;
	int k_prime_index;
	page = remove_entry_from_node(page, key);

	// Case: deletion from the root.
	if (page->offset == root->offset)
		return adjust_root();

	// Case: deletion from a node below the root.
	// Delayed merge.
	// Until num_keys == 0, no merge operation.
	if (page->num_keys == 0) {
		// Allocate memory to parent, neighbor.
		parent = (in_page *)calloc(1, sizeof(in_page));
		neighbor = (page_t *)calloc(1, sizeof(page_t));

		neighbor_index = get_neighbor_index((page_t *)page);
		k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

		// Read page.
		file_read_page(page->parent_offset, (page_t *)parent);

		k_prime = parent->keys[k_prime_index];

		neighbor->offset = neighbor_index == -1 ? parent->offsets[1] : parent->offsets[neighbor_index];

		file_read_page(neighbor->offset, neighbor);

		return merge_nodes(page, neighbor, neighbor_index, k_prime);
	}

	return (page_t *)root;
}

page_t * replace_entry(page_t * parent, uint64_t new_k_prime) {
	int i;
	int num_pointers;
	in_page * tmp_parent;

	tmp_parent = (in_page *)calloc(1, sizeof(in_page));
	file_read_page(parent->offset, (page_t *)tmp_parent);

	i = 0;

	// Find proper index to replace with new_k_prime.
	while (tmp_parent->keys[i] < new_k_prime) {
		i++;
	}

	// Change the value of key.
	tmp_parent->keys[i - 1] = new_k_prime;

	// Write on disk-page.
	file_write_page(parent->offset, (page_t *)tmp_parent);

	return (parent);
}

int delete(uint64_t key) {
	leaf_page * key_leaf;
	char * key_value;

	// Find key's record and page.
	key_leaf = find_leaf(key);
	key_value = find(key);

	// Case: The key exist.
	if (key_value != NULL && key_leaf != NULL) {
		delete_entry((page_t *)key_leaf, key);
		return 0;
	}

	// Case: The key not exist.
	else {
		printf("There is no %" PRId64 "in tree\n", key);
		return -1;
	}
}

void destroy_tree(in_page * root) {
	int i;

	if (root->is_leaf) {
		for (i = 0; i<root->num_keys; i++) {
			file_free_page(root->offsets[i]);
		}
	}
	else {
		in_page * c;
		for (i = 0; i<root->num_keys; i++) {
			file_read_page(root->offsets[i], (page_t *)c);
			destroy_tree(c);
		}
	}

	free(root->offsets);
	free(root->keys);
	free(root);
}


// MAIN
int main(int argc, char ** argv) {
	char string[VALUE_SIZE];
	uint64_t input;
	char instruction;

	usage();

	if (argc > 1) {
		strcpy(string, argv[1]);

		if (open_db(string) != 0) {
			perror("open file.");
			exit(EXIT_FAILURE);
		}
	}
	else {
		scanf("%s", string);

		if (open_db(string) != 0) {
			perror("open file.");
			exit(EXIT_FAILURE);
		}
	}

	print_tree();

	printf("> ");
	while (scanf(" %c", &instruction) != EOF) {
		switch (instruction) {
		case 'd':
			scanf("%" PRId64"", &input);
			delete(input);
			print_tree();
			break;
		case 'i':
			scanf("%" PRId64 "%s", &input, string);
			insert(input, string);
			print_tree();
			break;
		case 'f':
			scanf("%" PRId64 "", &input);
			printf("%s\n", find(input));
			break;
		case 't':
			print_tree();
			break;
		case 'q':
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
		default:
			break;
		}
		while (getchar() != (int)'\n');
		printf("> ");
	}
	if (fp != 0) {
		free(header);
		free(freepage);
		free(root);
		close(fp);
	}

	printf("\n");

	return EXIT_SUCCESS;
}
