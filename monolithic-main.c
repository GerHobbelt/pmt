
#include "pngtools-monolithic.h"

#define USAGE_NAME   "png-pmt-tools"

#include "monolithic_main_internal_defs.h"

// declare your own monolith dispatch table:
MONOLITHIC_CMD_TABLE_START()
	{ "pngcrush", {.fa = pngcrush_main } },
	{ "pngmeta", {.fa = pngmeta_main } },
	{ "zlib_to_idat", {.fa = pngzop_zlib_to_idat_main } },
	{ "pngidat", {.fa = pngidat_main } },
	{ "pngiend", {.fa = pngiend_main } },
	{ "pngihdr", {.fa = pngihdr_main } },
MONOLITHIC_CMD_TABLE_END();

#include "monolithic_main_tpl.h"
