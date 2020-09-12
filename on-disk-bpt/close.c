#include "_bpt.h"

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