/*
 * argv.h - an argument parser that sucks even less.
 * derived work of https://st.suckless.org/
 * by Tom Maltese
 */ 

#ifndef ARGV_H__
#define ARGV_H__

#define LONGARG \
for (argc--, argv++; \
     argc && argv[0][0] == '-'; argc--, argv++) { \
	int brk = 0; \
	char *op, *option; \
	if (argv[0][1] == '-') { \
		if (argv[0][2] == '\0') { \
			argc--; argv++; \
			break; \
		} \
		option = argv[0] + 2; \
		(void)option;

#define SHORTARG \
	} \
	for (op = (*argv) + 1; *op && !brk; op++) { \
		switch (*op)
#define ARGEND \
	}\
}

#define GETL() \
(argv[1] && argv[1][0] != '-') ? (brk = 1, --argc, *(++argv)) : (char*)0x0

#define GETS() \
(!brk && op[1] != '\0') ? (brk = 1, (op + 1)) : \
(argv[1] && argv[1][0] != '-') ? (brk = 1, --argc, *(++argv)) : \
                             (brk = 1, (char*)0x0)

#endif  /* ARGV_H__ */

