#include "_bpt.h"

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