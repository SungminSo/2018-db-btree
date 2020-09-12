#include "bpt.h"

// GLOBALS.

header_page * header[10];
free_page * freepage[10];
in_page * root[10];
queue * q = NULL;				// For print tree.
buffer * buf[BUFFER_SIZE];
buffer * last_buf;
buffer * first_buf;


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
		"\tf <k>	  -- Find value of key\n"
		"\tt          -- print the B+ tree.\n"
		"\tc <s>	  -- Change current file to <s> file\n"
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

int height(int table_id) {
	int h = 0;
	in_page * c = root[table_id];

	while (!c->is_leaf) {
		c = (in_page *)buf_get_page(table_id, c->offsets[0], (page_t *)c);
		h++;
	}

	return h;
}

int path_to_root(int table_id, in_page * root, in_page * n) {
	int length = 0;
	in_page * child = n;

	while (child->offset != root->offset) {
		child = (in_page *)buf_get_page(table_id, child->parent_offset, (page_t *)child);
		length++;
	}

	return length;
}

void print_leaves(int table_id) {
	leaf_page * c = (leaf_page *)root[table_id];
	in_page * tmp;

	if (c == NULL) {
		printf("Empty tree.\n");
		return;
	}

	while (!c->is_leaf) {
		tmp = (in_page *)c;
		c = (leaf_page *)buf_get_page(table_id, tmp->offsets[0], (page_t *)c);
	}

	while (true) {
		for (int i = 0; i<c->num_keys; i++) {
			printf("%" PRId64 "", c->pointers[i].key);
		}

		if (c->next_offset != 0) {
			printf(" | ");
			c = (leaf_page *)buf_get_page(table_id, c->next_offset, (page_t *)c);
		}
		else
			break;
	}
	printf("\n");
}

void print_tree(int table_id) {
	leaf_page * n = NULL;		// For dequeue()(leaf_page).
	in_page * c;				// For print.(in_page->keys).
	pagenum_t p;				// For enqueue(in_page).
	int new_rank = 0;
	int rank = 0;

	root[table_id] = (in_page *)buf_get_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);

	if (root[table_id] == NULL || root[table_id]->offset == 0) {
		buf_put_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);
		printf("Empty tree.\n");

		return;
	}

	// Alloc memory to n,c.
	n = (leaf_page *)calloc(1, sizeof(leaf_page));
	c = (in_page *)calloc(1, sizeof(in_page));

	// Initailize queue.
	q = NULL;
	enqueue(root[table_id]->offset);

	// Put page in queue one by one, print page's keys one by one.
	while (q != NULL) {
		p = dequeue();
		n = (leaf_page *)buf_get_page(table_id, p, (page_t *)n);
		c = (in_page *)buf_get_page(table_id, n->parent_offset, (page_t *)c);

		if (n->parent_offset != 0 && n->offset == c->offsets[0]) {
			new_rank = path_to_root(table_id, root[table_id], (in_page *)n);

			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}
		}
		buf_put_page(table_id, c->offset, (page_t *)c);

		if (!n->is_leaf) {
			c = (in_page *)buf_get_page(table_id, n->offset, (page_t *)c);
			for (int i = 0; i < c->num_keys; i++) {
				printf("%" PRId64 " ", c->keys[i]);
			}
			printf("| ");
			buf_put_page(table_id, c->offset, (page_t *)c);
		}
		else {
			n = (leaf_page *)buf_get_page(table_id, n->offset, (page_t *)n);
			for (int i = 0; i< n->num_keys; i++) {
				printf("%" PRId64 " ", n->pointers[i].key);
			}
			printf("| ");
			buf_put_page(table_id, n->offset, (page_t *)n);
		}

		if (!n->is_leaf) {
			c = (in_page *)buf_get_page(table_id, n->offset, (page_t *)c);
			for (int i = 0; i <= c->num_keys; i++) {
				enqueue(c->offsets[i]);
			}
			buf_put_page(table_id, c->offset, (page_t *)c);
		}

		buf_put_page(table_id, n->offset, (page_t *)n);
	}
	printf("\n");

	buf_put_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);

	//free(n);
	//free(c);
}

int cut(int order) {
	if (order % 2 == 0)
		return order / 2;
	else
		return (order / 2) + 1;
}

void make_header(int table_id, header_page * header[]) {
	header[table_id]->num_pages = 0;

	header[table_id] = (header_page *)buf_get_page(table_id, 0, (page_t *)header[table_id]);
	header[table_id]->num_pages++;

	// At first freepage's offset is 1.
	freepage[table_id]->offset = header[table_id]->num_pages;
	header[table_id]->free_offset = freepage[table_id]->offset;

	header[table_id]->root_offset = 0;

	buf_find_frame(table_id, header[table_id]->offset, 1);
	buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);
}


// File Manager API

// Allocate an on-disk page from the free page list

pagenum_t file_alloc_page(int table_id) {
	free_page * freepage;
	freepage = (free_page *)calloc(1, sizeof(free_page));

	file_read_page(table_id, header[table_id]->free_offset, (page_t *)freepage);

	// Update next free page in header.
	header[table_id]->num_pages++;
	if (freepage->next_page_offset != 0) {
		header[table_id]->free_offset = freepage->next_page_offset;
	}
	else {
		header[table_id]->free_offset = header[table_id]->num_pages;
	}

	// Write to the on-disk page
	file_write_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

	return freepage->offset;
}

// Free an on-disk page to the free page list

void file_free_page(int table_id, pagenum_t offset) {
	free_page * page;

	// Alloc memory to page.
	page = (free_page *)calloc(1, sizeof(free_page));
	//next = (page_t *)calloc(1, sizeof(page_t));

	file_read_page(table_id, offset, (page_t *)page);
	memset(page, 0, PAGE_SIZE);

	page->next_page_offset = header[table_id]->free_offset;
	header[table_id]->free_offset = page->offset;
	header[table_id]->num_pages--;

	file_write_page(table_id, offset, (page_t *)page);
	file_write_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

	free(page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int table_id, pagenum_t offset, page_t* dest) {
	lseek(table_id, PAGE_SIZE * offset, SEEK_SET);
	read(table_id, dest, PAGE_SIZE);
	dest->offset = offset;
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int table_id, pagenum_t offset, const page_t* src) {
	lseek(table_id, PAGE_SIZE * offset, SEEK_SET);
	write(table_id, src, PAGE_SIZE);
}


// Buffer Manager API

// Allocate the buffer pool with the given number of entries.
int init_db(int num_buf) {

	// Allocate memory to each buffer frame.
	for (int i = 0; i < num_buf; i++) {
		buf[i] = (buffer *)calloc(1, sizeof(buffer));
		//&buf[i]->page = NULL;
		buf[i]->is_dirty = false;
		buf[i]->is_pinned = false;
	}

	last_buf = (buffer *)calloc(1, sizeof(buffer));
	first_buf = (buffer *)calloc(1, sizeof(buffer));

	// Point next and prev buffer frame by LRU.
	for (int i = 0; i < num_buf; i++) {
		if (i == num_buf - 1) {
			buf[i]->prev_of_LRU = buf[i - 1];
			buf[i]->next_of_LRU = NULL;
		}
		else if (i == 0) {
			buf[i]->prev_of_LRU = NULL;
			buf[i]->next_of_LRU = buf[i + 1];
		}
		else {
			buf[i]->prev_of_LRU = buf[i - 1];
			buf[i]->next_of_LRU = buf[i + 1];
		}
	}

	last_buf = buf[BUFFER_SIZE - 1];
	first_buf = buf[0];

	return 0;
}

// Find proper buffer frame.
buffer * buf_find_frame(int table_id, pagenum_t offset, int flag) {
	buffer * tmp_buf;
	tmp_buf = last_buf;

	// Find proper buffer frame from last frame in LRU list.
	// Because it has high probability to find again the last used page.
	while (tmp_buf != NULL) {
		if (tmp_buf->table_id == table_id && tmp_buf->page_num == offset) {
			if (flag == 1)
				tmp_buf->is_dirty++;

			break;
		}

		tmp_buf = tmp_buf->prev_of_LRU;
	}

	return tmp_buf;
}

// Request to allocate new page and update header page.
pagenum_t buf_alloc_page(int table_id) {
	pagenum_t new_offset;
	buffer * header_buf;

	header_buf = buf_find_frame(table_id, header[table_id]->offset, 0);

	// There is a header page in buffer pool.
	// Check whether header page is dirty. If dirty, write to on-disk page and read it again.
	if (header_buf != NULL && header_buf->is_dirty) {
		file_write_page(table_id, header[table_id]->offset, &header_buf->page);
		file_read_page(table_id, header[table_id]->offset, &(header_buf->page));
	}

	// There is no header page in buffer pool.
	// So read from disk to buffer.
	else if (header_buf == NULL) {
		header_buf = first_buf;

		while (header_buf->is_pinned) {
			header_buf = header_buf->next_of_LRU;
		}
		file_read_page(table_id, header[table_id]->offset, &(header_buf->page));
	}

	new_offset = file_alloc_page(table_id);

	// Because of file_alloc_page(), header page in buffer pool and header page on disk are different.
	// So again read from disk to buffer.
	// file_read_page(table_id, header[table_id]->offset, &header_buf->page);

	return new_offset;
}


// Request to free page and update header page.
void buf_free_page(int table_id, pagenum_t pagenum) {
	buffer * header_buf;

	header_buf = buf_find_frame(table_id, pagenum, 0);

	// There is a header page in buffer pool.
	// Check whether header page is dirty. If dirty, write to on-disk page.
	if (header_buf != NULL && header_buf->is_dirty) {
		file_write_page(table_id, header[table_id]->offset, &header_buf->page);
	}

	// There is no header page in buffer pool.
	// So read from disk to buffer.
	else if (header_buf == NULL) {
		header_buf = first_buf;

		while (header_buf->is_pinned) {
			header_buf = header_buf->next_of_LRU;
		}
		file_read_page(table_id, header[table_id]->offset, &(header_buf->page));
	}


	file_free_page(table_id, pagenum);

	// Because of file_free_page(), header page in buffer pool and header page on disk are different.
	// So again read from disk to buffer.
	file_read_page(table_id, header[table_id]->offset, &header_buf->page);
}

// Index Manager request(get) page to Buffer Manager.
page_t  * buf_get_page(int table_id, pagenum_t offset, page_t * dest) {
	buffer * tmp_buf;

	tmp_buf = buf_find_frame(table_id, offset, 0);

	if (tmp_buf != NULL) {
		tmp_buf->is_pinned++;

		if (tmp_buf->next_of_LRU != NULL && tmp_buf->prev_of_LRU != NULL) {
			tmp_buf->next_of_LRU->prev_of_LRU = tmp_buf->prev_of_LRU;
			tmp_buf->prev_of_LRU->next_of_LRU = tmp_buf->next_of_LRU;
			last_buf->next_of_LRU = tmp_buf;
			tmp_buf->prev_of_LRU = last_buf;
			tmp_buf->next_of_LRU = NULL;
			last_buf = tmp_buf;
		}
		else if (tmp_buf->prev_of_LRU == NULL) {
			tmp_buf->next_of_LRU->prev_of_LRU = NULL;
			first_buf = tmp_buf->next_of_LRU;
			last_buf->next_of_LRU = tmp_buf;
			tmp_buf->prev_of_LRU = last_buf;
			tmp_buf->next_of_LRU = NULL;
			last_buf = tmp_buf;
		}

		return &tmp_buf->page;
	}


	// There is no proper page in buffer pool.
	// So read from disk to buffer pool.
	// Check whether the frame is pinned.
	else {
		tmp_buf = first_buf;

		while (tmp_buf->is_pinned)
			tmp_buf = tmp_buf->next_of_LRU;

		file_read_page(table_id, offset, &tmp_buf->page);
		tmp_buf->table_id = table_id;
		tmp_buf->page_num = offset;
		tmp_buf->is_dirty = false;
		tmp_buf->is_pinned++;

		if (tmp_buf->next_of_LRU != NULL && tmp_buf->prev_of_LRU != NULL) {
			tmp_buf->next_of_LRU->prev_of_LRU = tmp_buf->prev_of_LRU;
			tmp_buf->prev_of_LRU->next_of_LRU = tmp_buf->next_of_LRU;
			last_buf->next_of_LRU = tmp_buf;
			tmp_buf->prev_of_LRU = last_buf;
			tmp_buf->next_of_LRU = NULL;
			last_buf = tmp_buf;
		}
		else if (tmp_buf->prev_of_LRU == NULL) {
			tmp_buf->next_of_LRU->prev_of_LRU = NULL;
			first_buf = tmp_buf->next_of_LRU;
			last_buf->next_of_LRU = tmp_buf;
			tmp_buf->prev_of_LRU = last_buf;
			tmp_buf->next_of_LRU = NULL;
			last_buf = tmp_buf;
		}

		dest = &tmp_buf->page;
		return dest;
	}
}

// Index Manager put page to Buffer Manager.
void buf_put_page(int table_id, pagenum_t offset, page_t * src) {
	buffer * tmp_buf;

	tmp_buf = buf_find_frame(table_id, offset, 0);

	tmp_buf->is_pinned--;

	if (tmp_buf->is_pinned == 0 && tmp_buf->is_dirty == true) {
		file_write_page(table_id, offset, src);
		tmp_buf->is_dirty--;
	}
}


// Open

int open_table(char * pathname) {
	// Open file.
	fp = open(pathname, O_RDWR);

	// If the file exist, upload each page's information.
	if (fp >= 0) {
		// Alloc header_page, free_page, root.
		header[fp] = (header_page *)calloc(1, sizeof(header_page));
		freepage[fp] = (free_page *)calloc(1, sizeof(free_page));
		root[fp] = (in_page *)calloc(1, sizeof(in_page));

		header[fp] = (header_page *)buf_get_page(fp, 0, (page_t *)header[fp]);
		freepage[fp] = (free_page *)buf_get_page(fp, header[fp]->num_pages, (page_t *)freepage[fp]);
		root[fp] = (in_page *)buf_get_page(fp, header[fp]->root_offset, (page_t *)root[fp]);
	}

	// If the file does not exist, create one. And create header_page.
	else {
		fp = open(pathname, O_CREAT | O_RDWR, 0775);

		// Alloc header_page, free_page, root.
		header[fp] = (header_page *)calloc(1, sizeof(header_page));
		freepage[fp] = (free_page *)calloc(1, sizeof(free_page));
		root[fp] = (in_page *)calloc(1, sizeof(in_page));

		// If fail to create new one, return -1
		if (fp < 0) {
			perror("failed to create new file");
			return -1;
		}

		make_header(fp, header);
	}

	return fp;
}


// Close

int close_table(int table_id) {
	buffer * tmp_buf;
	buffer * last_buf;
	tmp_buf = buf[0];

	last_buf = tmp_buf;
	while (last_buf->next_of_LRU) {
		last_buf = last_buf->next_of_LRU;
	}

	while (tmp_buf->next_of_LRU) {
		if (tmp_buf->table_id == table_id && tmp_buf->is_dirty) {
			file_write_page(table_id, tmp_buf->page.offset, &tmp_buf->page);
		}

		last_buf->next_of_LRU = tmp_buf;
		last_buf = last_buf->next_of_LRU;
		tmp_buf = tmp_buf->next_of_LRU;
	}

	free(tmp_buf);
	free(last_buf);
	free(header[table_id]);
	free(freepage[table_id]);
	free(root[table_id]);
	close(table_id);

	return 0;
}

int shutdown_db(int table_id) {
	buffer * tmp_buf;
	buffer * next_buf;

	tmp_buf = buf[0];

	while (tmp_buf->next_of_LRU) {
		file_write_page(table_id, tmp_buf->page.offset, &tmp_buf->page);

		next_buf = tmp_buf->next_of_LRU;
		free(tmp_buf);
		tmp_buf = next_buf;
	}

	free(tmp_buf);
	free(next_buf);

	return 0;
}


// Find

leaf_page * find_leaf(int table_id, int64_t key) {
	int i;
	int flag = 0;
	in_page * c;

	// Case: the tree does not exist yet.
	if (root[table_id] == NULL || root[table_id]->offset == 0)
		return NULL;

	// Alloc memory to c.
	c = (in_page *)calloc(1, sizeof(in_page));

	// Read root page.
	c = (in_page *)buf_get_page(table_id, root[table_id]->offset, (page_t *)c);

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
			c = (in_page *)buf_get_page(table_id, c->offsets[i], (page_t *)c);
		else
			c = (in_page *)buf_get_page(table_id, c->offsets[i + 1], (page_t *)c);
	}

	return (leaf_page *)c;
}

char * find(int table_id, int64_t key) {
	int i = 0;
	int flag = 0;
	leaf_page * c;

	c = find_leaf(table_id, key);

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

record make_record(int64_t key, char * value) {
	record new_record;
	memset(&new_record.value, 0, VALUE_SIZE);

	// Alloc proper key and value.
	new_record.key = key;
	strcpy(new_record.value, value);

	return new_record;
}

page_t * make_page(int table_id) {
	page_t * c;

	// Alloc memory to c/
	c = (page_t *)calloc(1, sizeof(page_t));

	// Alloc new page.
	c = buf_get_page(table_id, buf_alloc_page(table_id), c);

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

page_t * insert_into_leaf(int table_id, leaf_page * leaf, int64_t key, record pointer) {
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

	// Mark is_dirty and Write to on-disk page.
	buf_find_frame(table_id, leaf->offset, 1);
	buf_put_page(table_id, leaf->offset, (page_t *)leaf);

	return (page_t *)leaf;
}

page_t * insert_into_leaf_after_splitting(int table_id, leaf_page * leaf, int64_t key, record pointer) {
	leaf_page * new_leaf;
	record * tmp_pointers;
	int insertion_index;
	int split;
	int64_t new_key;
	int i, j;

	// make new leaf page to split.
	new_leaf = (leaf_page *)make_page(table_id);
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

	// Mark is_dirty to pages in buffer pool.
	buf_find_frame(table_id, leaf->offset, 1);
	buf_find_frame(table_id, new_leaf->offset, 1);

	// Write to on-disk page.
	buf_put_page(table_id, leaf->offset, (page_t *)leaf);
	buf_put_page(table_id, new_leaf->offset, (page_t *)new_leaf);

	return insert_into_parent(table_id, (page_t *)leaf, new_key, (page_t *)new_leaf);
}

page_t * insert_into_node(int table_id, in_page * parent, int left_index, int64_t key, in_page * right) {
	// Move the elements to right one by one until left_index.
	for (int i = parent->num_keys; i>left_index; i--) {
		parent->keys[i] = parent->keys[i - 1];
		parent->offsets[i + 1] = parent->offsets[i];
	}

	// Put key and pointer at left_index.
	parent->keys[left_index] = key;
	parent->offsets[left_index + 1] = right->offset;
	parent->num_keys++;

	// Mark is_dirty and Write to on-disk page.
	buf_find_frame(table_id, parent->offset, 1);
	buf_put_page(table_id, parent->offset, (page_t *)parent);

	return (page_t*)root[table_id];
}

page_t * insert_into_node_after_splitting(int table_id, in_page * old_page, int left_index, int64_t key, in_page * right) {
	int split;
	int k_prime;
	in_page * new_page;
	page_t * child;
	int64_t * tmp_keys;
	pagenum_t * tmp_offsets;
	int i, j;

	// Make new node to split.
	new_page = (in_page *)make_page(table_id);

	// Alloc tmp_keys.
	tmp_keys = (int64_t *)calloc(INTERNAL_ORDER, sizeof(int64_t));

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
	child = (page_t *)calloc(1, sizeof(page_t));

	// Change parent_offset.
	new_page->parent_offset = old_page->parent_offset;
	for (i = 0; i <= new_page->num_keys; i++) {
		child = buf_get_page(table_id, new_page->offsets[i], (page_t *)child);
		child->parent_offset = new_page->offset;
		buf_put_page(table_id, child->offset, (page_t *)child);
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

	// Mark is_dirty to pages in buffer pool.
	buf_find_frame(table_id, old_page->offset, 1);
	buf_find_frame(table_id, new_page->offset, 1);

	// Wirte to on-disk page.
	buf_put_page(table_id, old_page->offset, (page_t *)old_page);
	buf_put_page(table_id, new_page->offset, (page_t *)new_page);

	return insert_into_parent(table_id, (page_t *)old_page, k_prime, (page_t *)new_page);
}

page_t * insert_into_parent(int table_id, page_t * child, int64_t new_key, page_t * new_child) {
	//left page = leaf, right page = new_leaf
	int left_index;
	in_page * parent;

	// Alloc memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));

	parent = (in_page *)buf_get_page(table_id, child->parent_offset, (page_t *)parent);

	// Case: new root.
	if (parent->offset == 0) {
		insert_into_new_root(table_id, child, new_key, new_child);

		return (page_t *)root[table_id];
	}

	// Find the parent's pointer of the left page.
	left_index = get_left_index(parent, child);

	// Case: the new key fits into the page.
	if (parent->num_keys < INTERNAL_ORDER - 1)
		return insert_into_node(table_id, parent, left_index, new_key, (in_page *)new_child);

	// Case: Harder. split a page.
	return insert_into_node_after_splitting(table_id, parent, left_index, new_key, (in_page *)new_child);
}

page_t * insert_into_new_root(int table_id, page_t * left, int64_t key, page_t * right) {
	in_page * new_root;

	new_root = (in_page *)make_page(table_id);
	new_root->keys[0] = key;
	new_root->offsets[0] = left->offset;
	new_root->offsets[1] = right->offset;
	new_root->num_keys++;
	left->parent_offset = new_root->offset;
	right->parent_offset = new_root->offset;

	root[table_id] = new_root;
	header[table_id]->root_offset = root[table_id]->offset;

	// Mark is_dirty to pages in buffer pool.
	buf_find_frame(table_id, left->offset, 1);
	buf_find_frame(table_id, right->offset, 1);
	buf_find_frame(table_id, root[table_id]->offset, 1);
	buf_find_frame(table_id, header[table_id]->offset, 1);

	// Write to on-disk page.
	buf_put_page(table_id, left->offset, (page_t *)left);
	buf_put_page(table_id, right->offset, (page_t *)right);
	buf_put_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);
	buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

	return (page_t *)root[table_id];
}

page_t * start_new_tree(int table_id, int64_t key, record pointer) {
	leaf_page * new_root;

	// Assign new page.
	new_root = (leaf_page *)make_page(table_id);
	new_root->is_leaf = true;
	new_root->next_offset = 0;
	new_root->pointers[0] = pointer;
	new_root->num_keys++;

	root[table_id] = (in_page *)new_root;
	header[table_id]->root_offset = root[table_id]->offset;

	// Mark is_dirty to header page and root page in buffer pool.
	buf_find_frame(table_id, root[table_id]->offset, 1);
	buf_find_frame(table_id, header[table_id]->offset, 1);

	// Write to on-disk page.
	buf_put_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);
	buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

	return (page_t *)root[table_id];
}

int insert(int table_id, int64_t key, char * value) {
	record pointer;
	leaf_page * leaf;

	//check is it duplicated
	if (find(table_id, key) != NULL)
		return -1;

	//create a new record
	pointer = make_record(key, value);

	// Case: the tree does not exist yet.
	// Start a new tree.
	if (root[table_id] == NULL || root[table_id]->offset == 0) {
		start_new_tree(table_id, key, pointer);
		return 0;
	}

	// Case: the tree does exist.
	leaf = find_leaf(table_id, key);

	// Case: leaf has room for key and pointer.
	if (leaf->num_keys < LEAF_ORDER - 1) {
		insert_into_leaf(table_id, leaf, key, pointer);
	}
	// Case: leaf must be split.
	else {
		insert_into_leaf_after_splitting(table_id, leaf, key, pointer);
	}

	return 0;
}


// Delete

page_t * remove_entry_from_node(int table_id, page_t * page, int64_t key) {
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
		page_leaf = (leaf_page *)buf_get_page(table_id, page->offset, (page_t *)page_leaf);

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

		// Mark is_dirty and Write in on-disk.
		buf_find_frame(table_id, page->offset, 1);
		buf_put_page(table_id, page->offset, (page_t *)page_leaf);

		free(page_leaf);
	}
	else {
		in_page * page_internal;
		page_internal = (in_page *)calloc(1, sizeof(in_page));
		page_internal = (in_page *)buf_get_page(table_id, page->offset, (page_t *)page_internal);

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

		// Mark is_dirty and Write in on-disk;
		buf_find_frame(table_id, page->offset, 1);
		buf_put_page(table_id, page->offset, (page_t *)page_internal);

		free(page_internal);
	}

	return page;
}

page_t * adjust_root(int table_id) {
	leaf_page * new_root;

	// Case: nonempty root.
	if (root[table_id]->num_keys > 0)
		return (page_t *)root[table_id];

	// Allocate memory to new_root
	new_root = (leaf_page *)calloc(1, sizeof(leaf_page));

	// Case: empty root.
	// Case: it has a child(only child)
	if (!root[table_id]->is_leaf) {
		new_root = (in_page *)buf_get_page(table_id, root[table_id]->offsets[0], (page_t *)new_root);
		new_root->parent_offset = 0;

		buf_free_page(table_id, root[table_id]->offset);

		root[table_id] = (in_page *)new_root;
		header[table_id]->root_offset = root[table_id]->offset;
	}

	// Case: it is a leaf(no child)
	else {
		free(new_root);

		buf_free_page(table_id, root[table_id]->offset);
		root[table_id] = NULL;
		header[table_id]->root_offset = 0;
	}

	// Mark is_dirty to pages in buffer pool.
	buf_find_frame(table_id, root[table_id]->offset, 1);
	buf_find_frame(table_id, header[table_id]->offset, 1);

	// Write in on-disk page.
	buf_put_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);
	buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

	return (page_t *)new_root;
}

int get_neighbor_index(int table_id, page_t * page) {
	int i;
	in_page * parent;

	// Allocate memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));

	parent = (in_page *)buf_get_page(table_id, page->parent_offset, (page_t *)parent);

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

page_t * merge_nodes(int table_id, page_t * page, page_t * neighbor, int neighbor_index, int64_t k_prime) {
	int i;
	page_t * tmp;
	in_page * parent;
	int64_t new_k_prime;

	// Allocate memory to parent.
	parent = (in_page *)calloc(1, sizeof(in_page));
	parent = (in_page *)buf_get_page(table_id, page->parent_offset, (page_t *)parent);

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

		tmp_page = (in_page *)buf_get_page(table_id, page->offset, (page_t *)tmp_page);
		tmp_neighbor = (in_page *)buf_get_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);

		// Appends k_prime and following pointer.
		tmp_neighbor->keys[tmp_neighbor->num_keys] = k_prime;
		tmp_neighbor->num_keys++;

		int split = cut(tmp_page->num_keys + tmp_neighbor->num_keys);

		// Shift tmp_page's keys and offsets to neighbor(left <- right)
		for (int i = 0; i < split && tmp_page->keys[0] != 0; i++) {
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
			tmp_child = buf_get_page(table_id, tmp_neighbor->offsets[i], (page_t *)tmp_child);
			tmp_child->parent_offset = tmp_neighbor->offset;
			buf_find_frame(table_id, tmp_child->offset, 1);
			buf_put_page(table_id, tmp_child->offset, (page_t *)tmp_child);
		}

		// Mark is_dirty to pages in buffer pool.
		buf_find_frame(table_id, neighbor->offset, 1);
		buf_find_frame(table_id, page->offset, 1);

		// Write to on-disk.
		buf_put_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);
		buf_put_page(table_id, page->offset, (page_t *)tmp_page);

		free(tmp_page);
		free(tmp_neighbor);
		free(tmp_child);
	}
	else {
		leaf_page * tmp_page;
		leaf_page * tmp_neighbor;

		tmp_page = (leaf_page *)calloc(1, sizeof(leaf_page));
		tmp_neighbor = (leaf_page *)calloc(1, sizeof(leaf_page));

		tmp_page = (leaf_page *)buf_get_page(table_id, page->offset, (page_t *)tmp_page);
		tmp_neighbor = (leaf_page *)buf_get_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);

		int split = cut(tmp_page->num_keys + tmp_neighbor->num_keys);

		// Shift tmp_page's keys and offsets to neighbor(left <- right)
		for (int i = 0; i < split && tmp_page->num_keys != 0; i++) {
			if (tmp_neighbor->num_keys == 0) {
				// Move the key and pointer from page to neighbor.
				tmp_neighbor->pointers[tmp_neighbor->num_keys] = tmp_page->pointers[i];
				tmp_neighbor->num_keys++;

				// Shift the key and pointer and remove the last one in page.
				tmp_page->pointers[i] = tmp_page->pointers[i + 1];
				tmp_page->pointers[tmp_page->num_keys - 1] = make_record(0, "");
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

		// Mark is_dirty to pages in buffer pool.
		buf_find_frame(table_id, neighbor->offset, 1);
		buf_find_frame(table_id, page->offset, 1);

		// Write to on-disk.
		buf_put_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);
		buf_put_page(table_id, page->offset, (page_t *)tmp_page);

		free(tmp_page);
		free(tmp_neighbor);
	}

	// If page's num_keys is 0 free an on-disk page to the free page list.
	page = buf_get_page(table_id, page->offset, page);

	if (page->num_keys == 0) {
		buf_free_page(table_id, page->offset);

		// Write in on-disk page.    ?
		// buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);     ?

		return erase_entry(table_id, (page_t *)parent, k_prime);
	}
	// Else the page remains and the keys of parent were changed.
	else {
		return replace_entry(table_id, (page_t *)parent, new_k_prime);
	}
}

page_t * erase_entry(int table_id, page_t * page, int64_t key) {
	page_t * neighbor;
	in_page * parent;
	int neighbor_index;
	int64_t k_prime;
	int k_prime_index;
	page = remove_entry_from_node(table_id, page, key);

	// Case: deletion from the root.
	if (page->offset == root[table_id]->offset)
		return adjust_root(table_id);

	// Case: deletion from a node below the root.
	// Delayed merge.
	// Until num_keys == 0, no merge operation.
	if (page->num_keys == 0) {
		// Allocate memory to parent, neighbor.
		parent = (in_page *)calloc(1, sizeof(in_page));
		neighbor = (page_t *)calloc(1, sizeof(page_t));

		neighbor_index = get_neighbor_index(table_id, (page_t *)page);
		k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

		// Read page.
		parent = (in_page *)buf_get_page(table_id, page->parent_offset, (page_t *)parent);

		k_prime = parent->keys[k_prime_index];

		neighbor->offset = neighbor_index == -1 ? parent->offsets[1] : parent->offsets[neighbor_index];

		neighbor = buf_get_page(table_id, neighbor->offset, neighbor);

		return merge_nodes(table_id, page, neighbor, neighbor_index, k_prime);
	}

	return (page_t *)root[table_id];
}

page_t * replace_entry(int table_id, page_t * page, int64_t new_k_prime) {
	int i;
	int num_pointers;
	in_page * tmp_parent;

	tmp_parent = (in_page *)calloc(1, sizeof(in_page));
	tmp_parent = (in_page *)buf_get_page(table_id, page->offset, (page_t *)tmp_parent);

	i = 0;

	// Find proper index to replace with new_k_prime.
	while (tmp_parent->keys[i] < new_k_prime) {
		i++;
	}

	// Change the value of the key.
	tmp_parent->keys[i - 1] = new_k_prime;

	// Write on disk-page.
	buf_put_page(table_id, page->offset, (page_t *)tmp_parent);

	return page;
}

int erase(int table_id, int64_t key) {
	leaf_page * key_leaf;
	char * key_value;

	// Find key's record and page.
	key_leaf = find_leaf(table_id, key);
	key_value = find(table_id, key);

	// Case: The key exist.
	if (key_value != NULL && key_leaf != NULL) {
		erase_entry(table_id, (page_t *)key_leaf, key);
		return 0;
	}

	// Case: The key not exist.
	else {
		printf("There is no %" PRId64 "in tree\n", key);
		return -1;
	}
}

void destroy_tree(int table_id, in_page * root) {
	int i;

	if (root->is_leaf) {
		for (i = 0; i<root->num_keys; i++) {
			buf_free_page(table_id, root->offsets[i]);
		}
	}
	else {
		in_page * c;
		for (i = 0; i<root->num_keys; i++) {
			c = (in_page *)buf_get_page(table_id, root->offsets[i], (page_t *)c);
			destroy_tree(table_id, c);
		}
	}

	free(root->offsets);
	free(root->keys);
	free(root);
}


// MAIN
int main(int argc, char ** argv) {
	char string[TABLE_COUNT][VALUE_SIZE];
	char tmp_string[VALUE_SIZE];
	int64_t input;
	char instruction;
	int table_id[TABLE_COUNT];
	int current_table = 0;
	int last_table = 0;
	int flag = 0;

	usage();
	init_db(BUFFER_SIZE);
	//
	printf("start\n");

	if (argc > 1) {
		strcpy(string[current_table], argv[1]);
		table_id[current_table] = open_table(string[current_table]);
		last_table++;
	}
	else {
		scanf("%s", string[current_table]);
		table_id[current_table] = open_table(string[current_table]);
		last_table++;
	}

	print_tree(table_id[current_table]);

	printf("> ");
	while (scanf(" %c", &instruction) != EOF) {
		switch (instruction) {
		case 'd':
			scanf("%" PRId64"", &input);
			erase(table_id[current_table], input);
			print_tree(table_id[current_table]);
			break;
		case 'i':
			scanf("%" PRId64 "%s", &input, tmp_string);
			insert(table_id[current_table], input, tmp_string);
			print_tree(table_id[current_table]);
			break;
		case 'f':
			scanf("%" PRId64 "", &input);
			printf("%s\n", find(table_id[current_table], input));
			break;
		case 't':
			print_tree(table_id[current_table]);
			break;
		case 'c':
			scanf("%s", tmp_string);
			for (int i = 0; i<last_table; i++) {
				if (!strcmp(string[i], tmp_string)) {
					current_table = i;
					flag = 1;
				}
			}

			if (flag != 1) {
				current_table = last_table;
				strcpy(string[current_table], tmp_string);
				table_id[current_table] = open_table(string[current_table]);
				last_table++;
			}
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
	for (int i = 0; i<last_table; i++) {
		if (table_id[i] != 0) {
			close_table(table_id[i]);
		}
	}

	printf("\n");

	return EXIT_SUCCESS;
}
