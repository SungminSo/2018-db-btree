#include "_bpt.h"

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

		buf_get_page(fp, 0, (page_t *)header[fp]);
		buf_get_page(fp, header[fp]->num_pages, (page_t *)freepage[fp]);
		buf_get_page(fp, header[fp]->root_offset, (page_t *)root[fp]);
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