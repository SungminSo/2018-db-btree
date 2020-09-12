#include "_bpt.h"

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
	init_db(TMP_BUFFER_SIZE);

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
			delete(table_id[current_table], input);
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