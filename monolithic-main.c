
#include <stdio.h>
#include <stdlib.h>

#include "pngtools-monolithic.h"

int main(int argc, const char** argv)
{
	pngcrush_main(argc, argv);
	pngmeta_main(argc, argv);

	pngzop_zlib_to_idat_main(argc, argv);
	pngidat_main(argc, argv);
	pngiend_main(argc, argv);
	pngihdr_main(argc, argv);

	return 0;
}
