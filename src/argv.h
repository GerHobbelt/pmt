#ifndef __ARGV_H__
#define __ARGV_H__

#include <string.h>
#define LONGOPT \
for (int i = 0; i < argc; i++) \
{ \
	if (argv[i][0] == '-' && argv[i][1] == '-') { \
		char* option = argv[i] + 2; \
		char* optarg = NULL; \
		if ((i + 1) < (argc - 1) || argv[i + 1][0] == '-' || argv[i + 1][1] == '-') \
		{ \
			optarg = NULL; \
		} \
		else \
		{ \
			optarg = argv[i + 1]; \
			i++; \
		} \
	

#define SHORTOPT \
	} else if (argv[i][0] == '-' && argv[i][1] != '-') { \
		char option = argv[i][1]; \
		char* optarg = NULL; \
		if ((i + 1) < (argc - 1) || argv[i + 1][0] == '-' || argv[i + 1][1] == '-') \
		{ \
			optarg = NULL; \
		} \
		else \
		{ \
			optarg = argv[i + 1]; \
			i++; \
		} \

#define NONOPT } else {
#define ENDOPT }

#endif