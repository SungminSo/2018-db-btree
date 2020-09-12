#include "_bpt.h"

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


	return 0;
}

// Request to allocate new page and update header page.
pagenum_t buf_alloc_page(int table_id) {
	pagenum_t new_offset;
	int flag = 0;			// If flag == 0, there is no header page in buffer pool.
	buffer * header_buf;
	header_buf = buf[0];

	// Check whether header page is in buffer pool or not.
	while (header_buf->next_of_LRU) {
		header_buf = header_buf->next_of_LRU;
	}

	while (header_buf->prev_of_LRU) {
		if (header_buf->table_id == table_id && header_buf->page_num == header[table_id]->offset) {
			flag = 1;
			break;
		}

		header_buf = header_buf->prev_of_LRU;
	}

	// There is a header page in buffer pool.
	// Check whether header page is dirty. If dirty, write to on-disk page.
	if (flag == 1 && header_buf->is_dirty) {
		file_write_page(table_id, header[table_id]->offset, &header_buf->page);
	}

	// There is no header page in buffer pool.
	// So read from disk to buffer.
	if (flag == 0) {
		while (header_buf->is_pinned) {
			header_buf = header_buf->next_of_LRU;
		}
	}

	file_read_page(table_id, header[table_id]->offset, &(header_buf->page));


	new_offset = file_alloc_page(table_id);

	// Because of file_alloc_page(), header page in buffer pool and header page on disk are different.
	// So again read from disk to buffer.
	file_read_page(table_id, header[table_id]->offset, &header_buf->page);

	return new_offset;
}

// Request to free page and update header page.
void buf_free_page(int table_id, pagenum_t pagenum) {
	int flag = 0;			// If flag == 0, there is no header page in buffer pool.
	buffer * header_buf;
	header_buf = buf[0];

	// Check whether header page is in buffer pool or not.
	while (header_buf->next_of_LRU) {
		header_buf = header_buf->next_of_LRU;
	}

	while (header_buf->prev_of_LRU) {
		if (header_buf->table_id == table_id && header_buf->page_num == header[table_id]->offset) {
			flag = 1;
			break;
		}

		header_buf = header_buf->prev_of_LRU;
	}

	// There is a header page in buffer pool.
	// Check whether header page is dirty. If dirty, write to on-disk page.
	if (flag == 1 && header_buf->is_dirty) {
		file_write_page(table_id, header[table_id]->offset, &header_buf->page);
	}

	// There is no header page in buffer pool.
	// So read from disk to buffer.
	if (flag == 0) {
		while (header_buf->is_pinned) {
			header_buf = header_buf->next_of_LRU;
		}
	}

	file_read_page(table_id, header[table_id]->offset, &header_buf->page);


	file_free_page(table_id, pagenum);

	// Because of file_free_page(), header page in buffer pool and header page on disk are different.
	// So again read from disk to buffer.
	file_read_page(table_id, header[table_id]->offset, &header_buf->page);
}

// Index Manager request(get) page to Buffer Manager.
page_t  * buf_get_page(int table_id, pagenum_t offset, page_t * dest) {
	buffer * tmp_buf;
	tmp_buf = buf[0];

	// Find the last frame in LRU list.
	while (tmp_buf->next_of_LRU) {
		tmp_buf = tmp_buf->next_of_LRU;
	}

	// Find proper buffer frame from last frame in LRU list.
	// Because it has high probability to find again the last used page.
	while (tmp_buf->prev_of_LRU) {
		if (tmp_buf->table_id == table_id && tmp_buf->page_num == offset) {
			tmp_buf->is_pinned++;
			return &tmp_buf->page;
		}

		tmp_buf = tmp_buf->prev_of_LRU;
	}

	// There is no proper page in buffer pool.
	// So read from disk to buffer pool.
	// Check whether the frame is pinned.
	while (tmp_buf->is_pinned)
		tmp_buf = tmp_buf->next_of_LRU;

	file_read_page(table_id, offset, &tmp_buf->page);
	tmp_buf->table_id = table_id;
	tmp_buf->page_num = offset;
	tmp_buf->is_dirty = false;
	tmp_buf->is_pinned++;
	return &(tmp_buf->page);
}

// Index Manager put page to Buffer Manager.
void buf_put_page(int table_id, pagenum_t offset, page_t * src) {
	buffer * tmp_buf;
	buffer * last_buf;
	last_buf = buf[0];

	// Find the last frame in LRU list.
	while (last_buf->next_of_LRU) {
		last_buf = last_buf->next_of_LRU;
	}
	tmp_buf = last_buf;

	// Find proper buffer frame from last frame in LRU list.
	// Because it has high probability to find again the last used page.
	// Check it is dirty or not.
	while (tmp_buf->prev_of_LRU) {
		if (tmp_buf->table_id == table_id && tmp_buf->page_num == offset) {
			tmp_buf->is_pinned--;
			if (&tmp_buf->page != src)
				tmp_buf->is_dirty = true;
			last_buf->next_of_LRU = tmp_buf;
			tmp_buf->next_of_LRU->prev_of_LRU = tmp_buf->prev_of_LRU;
			tmp_buf->prev_of_LRU->next_of_LRU = tmp_buf->next_of_LRU;
			tmp_buf->next_of_LRU = NULL;
			break;
		}
		else
			tmp_buf = tmp_buf->prev_of_LRU;
	}
	
	if (tmp_buf->is_pinned == 0 && tmp_buf->is_dirty == true) {
		file_write_page(table_id, offset, src);
	}
}