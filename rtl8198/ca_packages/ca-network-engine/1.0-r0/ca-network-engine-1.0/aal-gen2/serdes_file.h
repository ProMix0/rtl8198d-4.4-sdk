#ifndef _SERDES_FILE_H_872DCD85_7d60_4b70_8d26_2d8866c8f8e4_
#define _SERDES_FILE_H_872DCD85_7d60_4b70_8d26_2d8866c8f8e4_

struct Serdes;

struct Serdes *parse_serdes_file(const char *filename, int serdes_index);

int lookup_column_index(const struct Serdes *serdes,
			const char *name1, const char *name2);

void show_serdes_headers(const struct Serdes *serdes);
#endif
