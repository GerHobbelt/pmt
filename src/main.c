#define _CRT_SECURE_NO_DEPRECATE


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bork.h"
#include "mpng.h"
#include "util.h"

static void
briefusage(bool usage_only)
{
    printf("Usage: " PNGMETA_NAME
        " [-ADR]"
        " [-k KEY] [-t TEXT]"
        " [-d OUTDIR]"
        " [-e] [--human]"
        " [--chunk CHUNKIDX]"
        " FILE1 FILE2 ...\n");
    if (!usage_only)
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
    dynstr* kv,
    dynint* selchunks,
    bool exclusive, bool human)
{
    FILE* f = fopen(infile, "rb");
    if (f == NULL)
        die("Unable to open input file '%s'\n", infile);

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = NULL;
    buffer = malloc(fsize * sizeof(unsigned char));
    if (buffer == NULL)
        die("Failed to allocate %ld bytes for reading png file\n", fsize);

    fread(buffer, sizeof(unsigned char), fsize, f);
    fclose(f);

    switch (op)
    {
        case PNGMETA_OP_ADD_TEXT:
        {
            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die("Unable to open output file '%s'\n", outfile);

            int newfsize = 0;
            if (exclusive)
            {
                // deal with this later
                char* chunktype[] = { "tEXt" };
                int newsize = png_remove_chunk_by_type(buffer, fsize, chunktype, 1);
                fsize = newsize;
                newsize = png_add_text_chunks(buffer, fsize, kv->ptr, kv->size);
                if (newfsize == fsize)
                    die(PNGMETA_NAME ": Resulting file size is equal to original file size. Something went wrong.\n");
                fwrite(buffer, sizeof(unsigned char), newsize, output);
            }
            else
            {
                int newsize = png_add_text_chunks(buffer, fsize, kv->ptr, kv->size);
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
            int newsize = png_remove_chunks(buffer, fsize, selchunks->ptr, selchunks->size);
            if (newsize == fsize)
                die("Resulting file size is equal to original file size. Something went wrong.\n");

            FILE* output = fopen(outfile, "wb");
            if (output == NULL)
                die("Unable to open output file '%s'\n", outfile);
            fwrite(buffer, sizeof(unsigned char), newsize, output);

            printf("success: Removed chunk(s) from '%s'\n", outfile);
            fclose(output);
            break;
        }
        case PNGMETA_OP_NONE:
        {
            die("How did you get here?\n");
            break;
        }
    }
}

int
main(int argc, char* argv[])
{
    char* outdir = NULL;
    char* key = NULL;
    char* text = NULL;
    bool exclusive = 0;
    bool human = 0;
    enum PNGMETA_OP png_op = PNGMETA_OP_NONE;

    dynint chunks = dynint_init(1);
    dynstr kv = dynstr_init(2);
    dynstr infiles = dynstr_init(1);

    char* lastopt = "";
    char lastsopt = '\0';
    STARTOPT{
        LONGOPT{
            // check if the last option is key
            // if not followed by text, program dies
            if ((strcmp(lastopt, "key") == 0 || lastsopt == 'k') &&
                strcmp(option, "text") != 0)
                die("--key is not followed immediately by --text");

            if (strcmp(option, "add") == 0)
                png_op = PNGMETA_OP_ADD_TEXT;
            else if (strcmp(option, "dump") == 0)
                png_op = PNGMETA_OP_DUMP_TEXT;
            else if (strcmp(option, "remove") == 0)
                png_op = PNGMETA_OP_REMOVE_TEXT;
            else if (strcmp(option, "help") == 0)
                usage();
            else if (strcmp(option, "key") == 0)
                dynstr_add(&kv, GETS());
            else if (strcmp(option, "text") == 0)
            {
                if (strcmp(lastopt, "key") != 0 &&
                    lastsopt != 'k')
                    die("--text is not preceeded by --key\n");
                dynstr_add(&kv, GETS());
            }
            else if (strcmp(option, "dir") == 0)
                outdir = GETS();
            else if (strcmp(option, "exclusive") == 0)
                exclusive = true;
            else if (strcmp(option, "human") == 0)
                human = true;
            else if (strcmp(option, "chunk") == 0)
                dynint_add(&chunks, strtol_or_die(GETS()));
            else
                die(PNGMETA_NAME ": Unreconigzed option '%s'\n", option);
        lastopt = option;
        }
            SHORTOPT
        {
            if ((strcmp(lastopt, "key") == 0 || lastsopt == 'k') &&
                option != 't')
                die("--key is not followed immediately by --text");
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
                    dynstr_add(&kv, GETS());
                    break;
                case 't':
                    if (strcmp(lastopt, "key") != 0 &&
                        lastsopt != 'k')
                        die("--text is not preceeded by --key\n");
                    dynstr_add(&kv, GETS());
                    break;
                case 'd':
                    outdir = GETS();
                    break;
                case 'e':
                    exclusive = true;
                    break;
                case 'c': {
                    dynint_add(&chunks, strtol_or_die(GETS()));
                    break;
                }
            }
            lastsopt = option;
        } NONOPT {
            dynstr_add(&infiles, nonopt);
        }
    } ENDOPT

    if (kv.size % 2 != 0)
        die("Incomplete key-value pair, how did you even reach here?\n");
    if (png_op == PNGMETA_OP_NONE || infiles.size == 0)
        briefusage(false);
    if (png_op == PNGMETA_OP_ADD_TEXT && kv.size == 0)
        die("no key or text specified.\n");

    for (int i = 0; i < infiles.size; i++)
    {
        bool alloc = false;
        char* infile = infiles.ptr[i];
        char* outfile = NULL;
        if (outdir)
        {
            alloc = true;
            char* filename = getfilename(infile);
            int outdirlen = strlen(outdir);
            int fnlen = filename - infile; // pointer arithmetic might be faster here
            outfile = malloc((outdirlen + fnlen + 1) * sizeof(char));
            if (outfile == NULL)
                die("Unable to allocate %d bytes for creating string\n", outdirlen + fnlen + 1);

            memcpy(outfile, outdir, outdirlen);
            memcpy(outfile + outdirlen, filename, fnlen);
            outfile[outdirlen + fnlen + 1] = '\0';
        }
        else
            outfile = infile;

        
        process(
            infile, outfile,
            png_op,
            &kv,
            &chunks,
            exclusive, human
        );


        if (alloc)
            free(outfile);
    }
}
