#define RPNG_IMPLEMENTATION
#include "rpng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "util.h"
#include "simple-opt.h"

#define PNGMETA_NAME "pngmeta"

static void
dosomething(
    char* infile, char* outfile,
    enum PNGMETA_OP op,
    char* key, char* text,
    int* selchunks, int selchunk,
    bool exclusive, bool human)
{
    FILE* f = fopen(infile, "rb");
    if (f == NULL)
        die("%s: Unable to open file '%s'\n", PNGMETA_NAME, infile);

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* buffer = malloc(fsize * sizeof(unsigned char));
    fread(buffer, sizeof(unsigned char), fsize, f);
    fclose(f);

    char* outpath = outfile != NULL ? outfile : infile;

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
        FILE* output = fopen(outpath, "wb");
        if (output == NULL)
            die("%s: Unable to open output file '%s'\n", PNGMETA_NAME, outpath);

        long newfsize = 0;
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

        printf("%s: Done adding text chunk\n", PNGMETA_NAME);
        fclose(output);
        break;
    }

    case PNGMETA_OP_DUMP_TEXT:
    {
        int count = 0;
        rpng_chunk* chunks = rpng_chunk_read_all_from_memory(buffer, &count);
        if (count == 0 || chunks == NULL)
            die("%s: Unable to read chunks from file '%s'\n", PNGMETA_NAME, infile);

        for (int i = 0; i < count; i++)
        {
            if (strncmp(chunks[i].type, "tEXt", 4) != 0) continue;

            unsigned char* delim = memchr(chunks[i].data, '\0', chunks[i].length);
            if (delim == NULL)
                die("%s: Unable to find EOF character from the chunk, text chunk is invalid\n", PNGMETA_NAME);
            // realloc the buffer with 1 more byte to fit the EOF
            int keysize = delim - chunks[i].data;
            int valsize = chunks[i].length - keysize;

            unsigned char* chunk = realloc(chunks[i].data, (chunks[i].length + 1) * sizeof(unsigned char));
            chunk[chunks[i].length] = '\0';
            if (human)
                printf("Chunk %d of %d (%d bytes): %s: %s\n", i + 1, count, chunks[i].length, chunk, chunk + keysize + 1);
            else
                printf("%d %d %d %s %s\n", i, count, chunks[i].length, chunk, chunk + keysize + 1);
            free(chunks[i].data);
        }

        free(chunks);
        break;
    }

    case PNGMETA_OP_REMOVE_TEXT:
    {
        long count = 0;
        rpng_chunk* chunks = rpng_chunk_read_all_from_memory(buffer, &count);
        if (count == 0 || chunks == NULL)
            die("%s: Unable to read chunks from file '%s'\n", PNGMETA_NAME, infile);

        // worst case is newbuf size == fsize, because nothing is removed
        unsigned char* newbuf = malloc(fsize * sizeof(unsigned char));
        int newbufsize = 0;

        memcpy(newbuf, png_signature, 8);
        newbufsize += 8;

        for (int i = 0; i < count; i++)
        {
            for (int j = 0; j <= selchunk; j++)
            {
                // if it is not the chunk we want, copy it to output
                if (i != selchunks[j])
                {
                    unsigned int belength = endianswap(chunks[i].length);
                    unsigned int becrc = endianswap(chunks[i].crc);
                    memcpy(newbuf + newbufsize, &belength, 4);
                    memcpy(newbuf + newbufsize + 4, chunks[i].type, 4);
                    memcpy(newbuf + newbufsize + 8, chunks[i].data, chunks[i].length);
                    memcpy(newbuf + newbufsize + 8 + chunks[i].length, &becrc, 4);
                    newbufsize += (8 + chunks[i].length + 4);
                }
            }
        }

        FILE* output = fopen(outpath, "wb");
        if (output == NULL)
            die("%s: Unable to open output file '%s'\n", PNGMETA_NAME, outpath);
        fwrite(newbuf, sizeof(unsigned char), newbufsize, output);
        fclose(output);
    }
    }
}

int
main(int argc, char* argv[])
{
    char* infile = NULL;
    char* outfile = NULL;
    char* key = NULL;
    char* text = NULL;
    bool exclusive = 0;
    bool human = 0;
    enum PNGMETA_OP op = PNGMETA_OP_NONE;

    char** types = malloc(ARRAY_INITIAL_SIZE * sizeof(char*));
    int typeidx = 0;
    int typesize = ARRAY_INITIAL_SIZE;

    int* chunks = malloc(ARRAY_INITIAL_SIZE * sizeof(int));
    int chunkidx = 0;
    int chunksize = ARRAY_INITIAL_SIZE;

    struct simple_opt options[] = {
        { SIMPLE_OPT_FLAG,     'R',  "remove",	   false,  "Add text chunk to a file" },
        { SIMPLE_OPT_FLAG,     'D',  "dump ",	   false,  "Remove text chunk from a file" },
        { SIMPLE_OPT_FLAG,     'A',  "add",		   false,  "Dump text chunks from a file" },
        { SIMPLE_OPT_FLAG,     'h',  "help",	   false, "Prints this help message" },
        { SIMPLE_OPT_STRING,   'k',  "key",        false, "Keyword of the text" },
        { SIMPLE_OPT_STRING,   't',  "text",	   false, "Text" },
        { SIMPLE_OPT_STRING,   'f',  "input",      true,  "Input file name" },
        { SIMPLE_OPT_STRING,   'o',  "output",     true,  "Output file name" },
        { SIMPLE_OPT_FLAG,     'e',  "exclusive",  false, "Remove all text chunks from the file before adding any" },
        { SIMPLE_OPT_UNSIGNED, 'c',  "chunk",      false, "Specifies which chunk to remove. This can be used multiple times." },
        { SIMPLE_OPT_STRING,   '\0', "type",       false, "Specifies which type of chunks. Case sensitive" },
        { SIMPLE_OPT_FLAG,     '\0', "human",      false, "Produces human readable output instead of easy to parse output" },
        { SIMPLE_OPT_END }
    };

    struct simple_opt_result result = simple_opt_parse(argc, argv, options);
    if (result.result_type != SIMPLE_OPT_RESULT_SUCCESS)
    {
        simple_opt_print_error(stderr, 0, PNGMETA_NAME, result);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; options[i].type != SIMPLE_OPT_END; i++)
    {
        if (!options[i].was_seen) continue;
        switch (options[i].short_name)
        {
            case 'R':
                op = PNGMETA_OP_REMOVE_TEXT;
                break;
            case 'D':
                op = PNGMETA_OP_DUMP_TEXT;
                break;
            case 'A':
                op = PNGMETA_OP_ADD_TEXT;
                break;
            case 'h':
                simple_opt_print_usage(stdout, 0, PNGMETA_NAME, "[option] file1 file2 file...", NULL, options);
                break;
            case 'k':
                key = custom_strdup(options[i].val.v_string);
                break;
            case 't':
                text = custom_strdup(options[i].val.v_string);
                break;
            case 'f':
                infile = custom_strdup(options[i].val.v_string);
            case 'o':
            {
                int arglen = strlen(options[i].val.v_string);
                if (options[i].val.v_string[arglen - 1] != '\\' && options[i].val.v_string[arglen - 1] != '/')
                {
                    outfile = malloc((arglen + 2) * sizeof(char));
                    strncpy(outfile, options[i].val.v_string, arglen);
#ifdef _WIN32
                    outfile[arglen] = '\\';
#else
                    outfile[arglen] = '/';
#endif
                    outfile[arglen + 1] = '\0';
                }
                else
                    outfile = custom_strdup(options[i].val.v_string);
                break;
            }
            case 'e':
                exclusive = true;
                break;
            case 'c': {
                if (chunkidx >= chunksize - 1)
                    realloc(chunks, (chunksize *= 2) * sizeof(int));
                chunks[++chunkidx] = options[i].val.v_unsigned;
                break;
            }
            // longopt only
            case '\0':
            {
                if (strncmp(options[i].long_name, "--human", 7) == 0)
                    human = true;
                else if (strncmp(options[i].long_name, "--type", 6) == 0)
                {
                    if (strlen(options[i].val.v_string) != 4)
                        die("Type must be a valid png chunk type");

                    if (typeidx >= typesize - 1)
                        realloc(types, (typesize *= 2) * sizeof(char*));
                    types[++typeidx] = strdup(options[i].val.v_string);
                }
                break;
            }
        }
    }

    if (op == PNGMETA_OP_NONE)
        simple_opt_print_usage(stdout, 0, PNGMETA_NAME, "[option] file1 file2 file...", NULL, options);
    if (op == PNGMETA_OP_ADD_TEXT && (text == NULL || key == NULL))
        die("%s: no text specified.\n", PNGMETA_NAME);

    dosomething(
        infile, outfile,
        op,
        key, text,
        types, typeidx,
        chunks, chunkidx,
        exclusive, human
    );
}