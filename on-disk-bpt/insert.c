#include "_bpt.h"

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
	buf_get_page(table_id, buf_alloc_page(table_id), c);

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

	// Write to on-disk page.
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

	// Write to on-disk page.
	buf_put_page(table_id, parent->offset, (page_t *)parent);

	return (page_t*)root[table_id];
}

page_t * insert_into_node_after_splitting(int table_id, in_page * old_page, int left_index, int64_t key, in_page * right) {
	int split;
	int k_prime;
	in_page * new_page;
	in_page * child;
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
	child = (in_page *)calloc(1, sizeof(in_page));

	// Change parent_offset.
	new_page->parent_offset = old_page->parent_offset;
	for (i = 0; i <= new_page->num_keys; i++) {
		buf_get_page(table_id, new_page->offsets[i], (page_t *)child);
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

	buf_get_page(table_id, child->parent_offset, (page_t *)parent);

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