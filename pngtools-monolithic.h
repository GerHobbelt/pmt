
#pragma once

#if defined(BUILD_MONOLITHIC)

#ifdef __cplusplus
extern "C" {
#endif

int pngcrush_main(int argc, const char** argv);
int pngmeta_main(int argc, const char** argv);

int pngzop_zlib_to_idat_main(int argc, const char** argv);
int pngidat_main(int argc, const char** argv);
int pngiend_main(int argc, const char** argv);
int pngihdr_main(int argc, const char** argv);

#ifdef __cplusplus
}
#endif

#endif
