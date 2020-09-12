#include "_bpt.h"

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
		buf_get_page(table_id, c->offsets[0], (page_t *)c);
		h++;
	}

	return h;
}

int path_to_root(int table_id, in_page * root, in_page * n) {
	int length = 0;
	in_page * child = n;

	while (child->offset != root->offset) {
		buf_get_page(table_id, child->parent_offset, (page_t *)child);
		length++;
	}

	return length;
}

void print_leaves(int table_id) {
	leaf_page * c = (leaf_page *)root;
	in_page * tmp;

	if (c == NULL) {
		printf("Empty tree.\n");
		return;
	}

	while (!c->is_leaf) {
		tmp = (in_page *)c;
		buf_get_page(table_id, tmp->offsets[0], (page_t *)c);
	}

	while (true) {
		for (int i = 0; i<c->num_keys; i++) {
			printf("%" PRId64 "", c->pointers[i].key);
		}

		if (c->next_offset != 0) {
			printf(" | ");
			buf_get_page(table_id, c->next_offset, (page_t *)c);
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

	buf_get_page(table_id, root[table_id]->offset, (page_t *)root[table_id]);

	if (root[table_id] == NULL || root[table_id]->offset == 0) {
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
		buf_get_page(table_id, p, (page_t *)n);
		buf_get_page(table_id, n->parent_offset, (page_t *)c);

		if (n->parent_offset != 0 && n->offset == c->offsets[0]) {
			new_rank = path_to_root(table_id, root[table_id], (in_page *)n);

			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}

			buf_get_page(table_id, p, (page_t *)n);
		}

		if (!n->is_leaf) {
			buf_get_page(table_id, n->offset, (page_t *)c);
			for (int i = 0; i < c->num_keys; i++) {
				printf("%" PRId64 " ", c->keys[i]);
			}
			printf("| ");
		}
		else {
			buf_get_page(table_id, n->offset, (page_t *)n);
			for (int i = 0; i< n->num_keys; i++) {
				printf("%" PRId64 " ", n->pointers[i].key);
			}
			printf("| ");
		}

		if (!n->is_leaf) {
			buf_get_page(table_id, n->offset, (page_t *)c);
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

void make_header(int table_id, header_page * header[]) {
	header[table_id]->num_pages = 0;

	buf_get_page(table_id, 0, (page_t *)header[table_id]);
	header[table_id]->num_pages++;

	//freepage can have next page-> deal with cases.
	buf_get_page(table_id, header[table_id]->num_pages, (page_t *)freepage[table_id]);
	freepage[table_id]->next_page_offset = 0;
	header[table_id]->free_offset = freepage[table_id]->offset;

	header[table_id]->root_offset = 1;

	buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);
}
