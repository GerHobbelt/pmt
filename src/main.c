#define _CRT_SECURE_NO_DEPRECATE

#include "mpng.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "argv.h"

static void
briefusage(bool usageonly)
{
    printf("Usage: " PNGMETA_NAME
        " [-ADR]"
        " [-k KEY] [-t TEXT]"
        " [-d OUTDIR]"
        " [-e] [--human]"
        " [--chunk CHUNKIDX]"
        " FILE1 FILE2 ...\n");
    if (!usageonly)
    {
        printf(PNGMETA_NAME ": No mode or file specified.\n");
        exit(EXIT_FAILURE);
    }
}

static void
usage()
{
    briefusage(true);
    printf(
        "Mode:\n"
        "\t--add, -A       Add text chunk to a file\n"
        "\t--dump, -D      Dump text chunks from a file\n"
        "\t--remove, -R    Remove text chunk from a file\n\n"
        "Options:\n"
        "\t--help, -h      Prints this help message\n"
        "\t--key, -k       Keyword for the text\n"
        "\t--text, -t      Text\n"
        "\t--dir, -d       Output file directory\n"
        "\t--exclusive, -e Remove all text chunks from the file before adding any chunk\n"
        "\t--chunk, -c     Specifies which chunk to remove. This can be used multiple times\n"
        "\t--human         Produces human readable output instead of easy to parse output\n"
    );
    exit(EXIT_FAILURE);
}

static void
_process(
    char* infile, char* outfile,
    enum PNGMETA_OP op,
    char* key, char* text,
    int* selchunks, int selchunkidx,
    bool exclusive, bool human)
{
    FILE* f = fopen(infile, "rb");
    if (f == NULL)
        die(PNGMETA_NAME ": Unable to open input file '%s'\n", infile);

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = NULL;
    buffer = malloc(fsize * sizeof(unsigned char));
    if (buffer == NULL)
        die(PNGMETA_NAME ": Failed to allocate %ld bytes for reading png file\n", fsize);

    fread(buffer, sizeof(unsigned char), fsize, f);
    fclose(f);

    switch (op)
    {
        case PNGMETA_OP_ADD_TEXT:
        {
            int keysize = strlen(key) + 1;
            int strsize = strlen(text);
            unsigned char* chunkdata = malloc((keysize + strsize) * sizeof(unsigned char)); // png texts no EOF, only keys need it
            memcpy(chunkdata, key, keysize);
            memcpy(chunkdata + keysize, text, strsize);
            png_chunk chunk = {
                .type = "tEXt",
                .length = keysize + strsize,
                .data = chunkdata
            };
            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die(PNGMETA_NAME ": Unable to open output file '%s'\n", outfile);

            int newfsize = 0;
            if (exclusive)
            {
                // deal with this later
                char* chunktype[] = { "tEXt" };
                int newsize = png_remove_chunk_by_type(buffer, fsize, chunktype, 1);
                fsize = newsize;
                newsize = png_add_chunk(buffer, fsize, &chunk, 1);
                if (newfsize == fsize)
                    die(PNGMETA_NAME ": Resulting file size is equal to original file size. Something went wrong.\n");
                fwrite(buffer, sizeof(unsigned char), newsize, output);
            }
            else
            {
                int newsize = png_add_chunk(buffer, fsize, &chunk, 1);
                if (newfsize == fsize)
                    die(PNGMETA_NAME ": Resulting file size is equal to original file size. Something went wrong.\n");
                fwrite(buffer, sizeof(unsigned char), newsize, output);
            }

            printf("success: Done adding text chunk to '%s'\n", outfile);
            fclose(output);
            break;
        }

        case PNGMETA_OP_DUMP_TEXT:
        {      
            printf("file: %s\n", infile);
            png_state state = { 0 };
            png_chunk chunk = { 0 };
            
            if (png_parser_create(&state, buffer, fsize) != PNG_OK)
                die(PNGMETA_NAME ": Broken input file '%s'\n", infile);
               
            png_result res = 0;
            while ((res = png_parser_next(&state, &chunk)) != PNG_END)
            {
                if (strncmp((const char*)chunk.type, "tEXt", 4) == 0) {
                    unsigned char* delim = memchr(chunk.data, '\0', chunk.length);
                    if (delim == NULL)
                    {
                        printf(PNGMETA_NAME ": Unable to find EOF character from the chunk, text chunk is invalid\n");
                        continue;
                    }
                    // realloc the buffer with 1 more byte to fit the EOF
                    int keysize = delim - chunk.data;
                    int valsize = chunk.length - keysize - 1;

                    if (human)
                        printf("chunk: %d (%d bytes): %s: %.*s\n", state.count, chunk.length, chunk.data, valsize, chunk.data + keysize + 1);
                    else
                        printf("chunk: %d %d %s %.*s\n", state.count, chunk.length, chunk.data, valsize, chunk.data + keysize + 1);
                }
            }
            break;
        }

        case PNGMETA_OP_REMOVE_TEXT:
        {
            int newsize = png_remove_chunk(buffer, fsize, selchunks, selchunkidx);
            if (newsize == fsize)
                die(PNGMETA_NAME ": Resulting file size is equal to original file size. Something went wrong.\n");

            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die(PNGMETA_NAME ": Unable to open output file '%s'\n", outfile);
            fwrite(buffer, sizeof(unsigned char), newsize, output);

            printf("success: Removed chunk(s) from '%s'\n", outfile);
            fclose(output);
            break;
        }
		case PNGMETA_OP_NONE:
		{
			// TODO: Do todo
		}
    }
}

int
main(int argc, char* argv[])
{
    char* _infile = NULL;
    char* outdir = NULL;
    char* key = NULL;
    char* text = NULL;
    bool exclusive = 0;
    bool human = 0;
    enum PNGMETA_OP png_op = PNGMETA_OP_NONE;

    int* chunks = NULL;
    int chunkidx = 0;
    int chunksize = 0;

    LONGOPT {
        if (strcmp(option, "add") == 0)
            png_op = PNGMETA_OP_ADD_TEXT;
        else if (strcmp(option, "dump") == 0)
            png_op = PNGMETA_OP_DUMP_TEXT;
        else if (strcmp(option, "remove") == 0)
            png_op = PNGMETA_OP_REMOVE_TEXT;
        else if (strcmp(option, "help") == 0)
            usage();
        else if (strcmp(option, "key") == 0)
            key = optarg;
        else if (strcmp(option, "text") == 0)
            key = optarg;
        else if (strcmp(option, "dir") == 0)
            outdir = optarg;
        else if (strcmp(option, "exclusive") == 0)
            exclusive = true;
        else if (strcmp(option, "human") == 0)
            human = true;
        else if (strcmp(option, "chunk") == 0)
        {
            if (chunks == NULL)
            {
                chunks = malloc(1 * sizeof(int));
                if (chunks == NULL)
                    die(PNGMETA_NAME ": Unable to allocate %d bytes to store chunk index\n", 1);
                chunksize = 1;
            }
            if (chunkidx >= chunksize - 1)
            {
                int *newchunks = realloc(chunks, (chunksize *= 2) * sizeof(int));
                if (newchunks == NULL)
                    die(PNGMETA_NAME ": Unable to reallocate %d bytes to store chunk index\n", chunksize);
                chunks = newchunks;
            }

            chunks[chunkidx++] = strtol_or_die(optarg);
        }
        else
            die(PNGMETA_NAME ": Unreconigzed option '%s'\n", option);
    }
        SHORTOPT
    {
        switch (option) {
            case 'A':
                png_op = PNGMETA_OP_ADD_TEXT;
                break;
            case 'D':
                png_op = PNGMETA_OP_DUMP_TEXT;
                break;
            case 'R':
                png_op = PNGMETA_OP_REMOVE_TEXT;
                break;
            case 'h':
                    usage();
                break;
            case 'k':
                key = optarg;
                break;
            case 't':
                text = optarg;
                break;
            case 'd':
                outdir = optarg;
                break;
            case 'e':
                exclusive = true;
                break;
            case 'c': {
                if (chunks == NULL)
                {
                    chunks = malloc(1 * sizeof(int));
                    if (chunks == NULL)
                        die(PNGMETA_NAME ": Unable to allocate %d bytes to store chunk index\n", 1);
                    chunksize = 1;
                }
                if (chunkidx >= chunksize - 1)
                {
                    int* newchunks = realloc(chunks, (chunksize *= 2) * sizeof(int));
                    if (newchunks == NULL)
                        die(PNGMETA_NAME ": Unable to reallocate %d bytes to store chunk index\n", chunksize);
                    chunks = newchunks;
                }

                chunks[chunkidx++] = strtol_or_die(optarg);
                break;
            }
        }
    } NONOPT {
        // TODO
    } ENDOPT


    if (png_op == PNGMETA_OP_NONE || !argc)
        briefusage(false);
    if (png_op == PNGMETA_OP_ADD_TEXT && (text == NULL || key == NULL))
        die(PNGMETA_NAME ": no key or text specified.\n");

    /*
    for (int i = 0; i < argc; i++)
    {
        bool alloc = false;
        char* outfile = NULL;
        if (outdir)
        {
            alloc = true;
            char* infilename = getfilename(argv[i]);
            int outdirlen = strlen(outdir);
            int ifnlen = infilename - argv[i]; // pointer arithmetic might be faster here
            outfile = malloc((ifnlen + outdirlen + 1) * sizeof(char));
            if (outfile == NULL)
                die(PNGMETA_NAME ": Unable to allocate %d bytes for creating string\n", ifnlen + outdirlen + 1);

            memcpy(outfile, outdir, outdirlen);
            memcpy(outfile + outdirlen, infilename, ifnlen);
            outfile[outdirlen + ifnlen + 1] = '\0';
        }
        else
            outfile = argv[i];

        
        process(
            argv[i], outfile,
            png_op,
            key, text,
            chunks, chunkidx,
            exclusive, human
        );


        if (alloc)
            free(outfile);
    }
    */

    free(chunks);
}}
