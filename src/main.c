#define _CRT_SECURE_NO_DEPRECATE

#define RPNG_IMPLEMENTATION
#include "rpng.h"

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
process(
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
        die(PNGMETA_NAME ": Failed to allocate %d bytes for reading png file\n", fsize);

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
            rpng_chunk chunk = {
                .type = "tEXt",
                .length = keysize + strsize,
                .data = chunkdata
            };

            unsigned char* newbuf;
            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die(PNGMETA_NAME ": Unable to open output file '%s'\n", outfile);

            int newfsize = 0;
            if (exclusive)
            {
                newbuf = rpng_chunk_remove_from_memory(buffer, "tEXt", &newfsize);
                free(buffer);
                buffer = rpng_chunk_write_from_memory(newbuf, chunk, &newfsize);
                free(newbuf);
                fwrite(buffer, sizeof(unsigned char), newfsize, output);
                free(buffer);
            }
            else
            {
                newbuf = rpng_chunk_write_from_memory(buffer, chunk, &newfsize);
                free(buffer);
                fwrite(newbuf, sizeof(unsigned char), newfsize, output);
                free(newbuf);
            }

            printf("success: Done adding text chunk to '%s'\n", outfile);
            fclose(output);
            break;
        }

        case PNGMETA_OP_DUMP_TEXT:
        {
            int count = 0;
            rpng_chunk* chunks = rpng_chunk_read_all_from_memory(buffer, &count);
            free(buffer);
            if (count == 0 || chunks == NULL)
                die(PNGMETA_NAME ": Unable to read chunks from file '%s'\n", infile);
            
            printf("file: %s\n", infile);
            for (int i = 0; i < count; i++)
            {
                if (strncmp(chunks[i].type, "tEXt", 4) == 0) {
                    unsigned char* delim = memchr(chunks[i].data, '\0', chunks[i].length);
                    if (delim == NULL)
                    {
                        printf(PNGMETA_NAME ": Unable to find EOF character from the chunk, text chunk is invalid\n");
                        free(chunks[i].data);
                        continue;
                    }
                    // realloc the buffer with 1 more byte to fit the EOF
                    int keysize = delim - chunks[i].data;
                    int valsize = chunks[i].length - keysize - 1;

                    if (human)
                        printf("chunk: %d of %d (%d bytes): %s: %.*s\n", i + 1, count, chunks[i].length, chunks[i].data, valsize, chunks[i].data + keysize + 1);
                    else
                        printf("chunk: %d %d %d %s %.*s\n", i, count, chunks[i].length, chunks[i].data, valsize, chunks[i].data + keysize + 1);
                    free(chunks[i].data);
                }
                else {
                    free(chunks[i].data);
                    continue;
                }
            }
            free(chunks);
            break;
        }

        case PNGMETA_OP_REMOVE_TEXT:
        {
            int count = 0;
            rpng_chunk* chunks = rpng_chunk_read_all_from_memory(buffer, &count);
            free(buffer);
            if (count == 0 || chunks == NULL)
                die(PNGMETA_NAME ": Unable to read chunks from file '%s'\n", infile);

            // worst case is newbuf size == fsize, because nothing is removed
            unsigned char* newbuf = malloc(fsize * sizeof(unsigned char));
            if (newbuf == NULL)
                die(PNGMETA_NAME ": Unable to allocate %d bytes for output file\n", fsize);
            int newbufsize = 0;

            memcpy(newbuf, png_signature, 8);
            newbufsize += 8;

            int idx = 0;
            for (int i = 0; i < count; i++)
            {
                // if it is not the chunk we want, copy it to output
                if (i != selchunks[idx])
                {
                    unsigned int belength = endianswap(chunks[i].length);
                    unsigned int becrc = endianswap(chunks[i].crc);
                    memcpy(newbuf + newbufsize, &belength, 4);
                    memcpy(newbuf + newbufsize + 4, chunks[i].type, 4);
                    memcpy(newbuf + newbufsize + 8, chunks[i].data, chunks[i].length);
                    memcpy(newbuf + newbufsize + 8 + chunks[i].length, &becrc, 4);
                    newbufsize += (8 + chunks[i].length + 4);
                }
                else
                    idx++;
                free(chunks[i].data);
            }

            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die(PNGMETA_NAME ": Unable to open output file '%s'\n", outfile);
            fwrite(newbuf, sizeof(unsigned char), newbufsize, output);

            printf("success: Removed chunk(s) from '%s'\n", outfile);
            fclose(output);
            free(newbuf);
        }
    }
}

int
main(int argc, char* argv[])
{
    char* infile = NULL;
    char* outdir = NULL;
    char* key = NULL;
    char* text = NULL;
    bool exclusive = 0;
    bool human = 0;
    enum PNGMETA_OP png_op = PNGMETA_OP_NONE;

    int* chunks = NULL;
    int chunkidx = 0;
    int chunksize = 0;

    LONGARG {
        if (strcmp(option, "add") == 0)
            png_op = PNGMETA_OP_ADD_TEXT;
        else if (strcmp(option, "dump") == 0)
            png_op = PNGMETA_OP_DUMP_TEXT;
        else if (strcmp(option, "remove") == 0)
            png_op = PNGMETA_OP_REMOVE_TEXT;
        else if (strcmp(option, "help") == 0)
            usage();
        else if (strcmp(option, "key") == 0)
            key = GETL();
        else if (strcmp(option, "text") == 0)
            key = GETL();
        else if (strcmp(option, "dir") == 0)
            outdir = GETL();
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

            chunks[chunkidx++] = strtol_or_die(GETL());
        }
        else
            die(PNGMETA_NAME ": Unreconigzed option '%s'\n", option);
    }
        SHORTARG
    {
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
            // this is the limit of argv.h, it cannot differentiate between -h and --human
            if (option && strcmp(option, "human") != 0)
                usage();
            break;
        case 'k':
            key = GETS();
            break;
        case 't':
            text = GETS();
            break;
        case 'd':
            outdir = GETS();
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

            chunks[chunkidx++] = strtol_or_die(GETS());
            break;
        }
    } ARGEND


    if (png_op == PNGMETA_OP_NONE || !argc)
        briefusage(false);
    if (png_op == PNGMETA_OP_ADD_TEXT && (text == NULL || key == NULL))
        die(PNGMETA_NAME ": no key or text specified.\n");

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

    free(chunks);
}
