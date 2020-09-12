#include "_bpt.h"

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
		buf_get_page(table_id, page->offset, (page_t *)page_leaf);

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
		buf_put_page(table_id, page->offset, (page_t *)page_leaf);

		free(page_leaf);
	}
	else {
		in_page * page_internal;
		page_internal = (in_page *)calloc(1, sizeof(in_page));
		buf_get_page(table_id, page->offset, (page_t *)page_internal);

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
		buf_get_page(table_id, root[table_id]->offsets[0], (page_t *)new_root);
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

	buf_get_page(table_id, page->parent_offset, (page_t *)parent);

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
	buf_get_page(table_id, page->parent_offset, (page_t *)parent);

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

		buf_get_page(table_id, page->offset, (page_t *)tmp_page);
		buf_get_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);

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
			buf_get_page(table_id, tmp_neighbor->offsets[i], (page_t *)tmp_child);
			tmp_child->parent_offset = tmp_neighbor->offset;
			buf_put_page(table_id, tmp_child->offset, (page_t *)tmp_child);
		}

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

		buf_get_page(table_id, page->offset, (page_t *)tmp_page);
		buf_get_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);

		int split = cut(tmp_page->num_keys + tmp_neighbor->num_keys);

		// Shift tmp_page's keys and offsets to neighbor(left <- right)
		for (int i = 0; i < split && tmp_page->num_keys != 0; i++) {
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

		if(tmp_page->num_keys == 0) {
			tmp_neighbor->next_offset = tmp_page->next_offset;
		}

		buf_put_page(table_id, neighbor->offset, (page_t *)tmp_neighbor);
		buf_put_page(table_id, page->offset, (page_t *)tmp_page);

		free(tmp_page);
		free(tmp_neighbor);
	}

	// If page's num_keys is 0 free an on-disk page to the free page list.
	buf_get_page(table_id, page->offset, (page_t *)page);
	
	if(page->num_keys == 0) {
		buf_free_page(table_id, page->offset);

		// Write in on-disk page.
		buf_put_page(table_id, header[table_id]->offset, (page_t *)header[table_id]);

		return delete_entry(table_id, (page_t *)parent, k_prime);
	}
	// Else the page remains and the keys of parent were changed.
	else {
		return replace_entry(table_id, (page_t *)parent, new_k_prime);
	}
}

page_t * delete_entry(int table_id, page_t * page, int64_t key) {
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
		buf_get_page(table_id, page->parent_offset, (page_t *)parent);

		k_prime = parent->keys[k_prime_index];

		neighbor->offset = neighbor_index == -1 ? parent->offsets[1] : parent->offsets[neighbor_index];

		buf_get_page(table_id, neighbor->offset, neighbor);

		return merge_nodes(table_id, page, neighbor, neighbor_index, k_prime);
	}

	return (page_t *)root[table_id];
}

page_t * replace_entry(int table_id, page_t * page, int64_t new_k_prime) {
	int i;
	int num_pointers;
	in_page * tmp_parent;

	tmp_parent = (in_page *)calloc(1, sizeof(in_page));
	buf_get_page(table_id, page->offset, (page_t *)tmp_parent);

	i = 0;

	// Find proper index to replace with new_k_prime.
	while(tmp_parent->keys[i] < new_k_prime) {
		i++;
	}

	// Change the value of the key.
	tmp_parent->keys[i - 1] = new_k_prime;

	// Write on disk-page.
	buf_put_page(table_id, page->offset, (page_t *)tmp_parent);

	return page;
}

int delete(int table_id, int64_t key) {
	leaf_page * key_leaf;
	char * key_value;

	// Find key's record and page.
	key_leaf = find_leaf(table_id, key);
	key_value = find(table_id, key);

	// Case: The key exist.
	if (key_value != NULL && key_leaf != NULL) {
		delete_entry(table_id, (page_t *)key_leaf, key);
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
			buf_get_page(table_id, root->offsets[i], (page_t *)c);
			destroy_tree(table_id, c);
		}
	}

	free(root->offsets);
	free(root->keys);
	free(root);
}