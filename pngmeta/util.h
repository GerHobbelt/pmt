#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PNGMETA_NAME "pngmeta"
#define die(...) { fprintf(stderr, PNGMETA_NAME ": " __VA_ARGS__); exit(EXIT_FAILURE); }

enum PNGMETA_OP {
	PNGMETA_OP_NONE,
	PNGMETA_OP_ADD_TEXT,
	PNGMETA_OP_DUMP_TEXT,
	PNGMETA_OP_REMOVE_TEXT
};

typedef struct dynstr {
	size_t capacity;
	size_t size;
	char** ptr;
} dynstr;

typedef struct dynint {
	size_t capacity;
	size_t size;
	int* ptr;
} dynint;

#define dynstr_init(min_size) { min_size, 0, malloc(min_size * sizeof(char*)) }
#define dynint_init(min_size) { min_size, 0, malloc(min_size * sizeof(int)) }

inline dynstr_add(dynstr* dyn, char* val)
{
	if (dyn->size >= dyn->capacity)
	{
		dyn->capacity *= 2;
		char** ptr = realloc(dyn->ptr, dyn->capacity * sizeof(char*));
		if (ptr == NULL)
			die("Unable to allocate %zu bytes for array.\n", dyn->capacity * sizeof(char*));
		dyn->ptr = ptr;
	}
	dyn->ptr[dyn->size++] = val;
}

inline dynint_add(dynint* dyn, int val)
{
	if (dyn->size >= dyn->capacity)
	{
		dyn->capacity *= 2;
		int* ptr = realloc(dyn->ptr, dyn->capacity * sizeof(int));
		if (ptr == NULL)
			die("Unable to allocate %zu bytes for array.\n", dyn->capacity * sizeof(char*));
		dyn->ptr = ptr;
	}
	dyn->ptr[dyn->size++] = val;
}

char*
getfilename(char* path)
{
	char* lastoccur = strrchr(path, '\\');
	if (lastoccur == NULL)
		lastoccur = strrchr(path, '/');
	if (lastoccur == NULL)
		return path;
	else
		return lastoccur;
}

int strtol_or_die(char* str)
{
	char* endptr;
	errno = 0;
	int number = strtol(str, &endptr, 0);

	if (endptr == str || *endptr != '\0' ||
		(errno == ERANGE))
		die(PNGMETA_NAME ": Unable to parse option '%s' as a number\n", str);
	
	return number;
}

int numcmp(const void* a, const void* b)
{
	int x = *(const int*)a;
	int y = *(const int*)b;
	if (x < y)
		return -1;
	else if (x > y)
		return 1;
	return 0;
}
#endif
