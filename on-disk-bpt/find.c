#include "_bpt.h"

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
	buf_get_page(table_id, root[table_id]->offset, (page_t *)c);

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
			buf_get_page(table_id, c->offsets[i], (page_t *)c);
		else
			buf_get_page(table_id, c->offsets[i + 1], (page_t *)c);
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