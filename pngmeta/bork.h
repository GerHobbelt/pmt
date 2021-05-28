/*
Command-line argument parsing with C macros. Choice of public domain or MIT-0. See license statement at the end of this file.
bork - v2.0.1 - 2020-8-8

Kelvin Voon - takase1121@outlook.com

GitHub: https://github.com/takase1121/bork
*/

/*
# Bork
A single file library for command-line argument parsing, implemented in C macros.

I created this library for my own little programs, taking inspiration from [argv.h](https://github.com/tommalt/argv) which is derived from [st](https://st.suckless.org/). Ironically, it sucks more than both of them.

To use bork, put this in your C file:
```c
#include "bork.h"
```

Here is an example:
```c
// assuming you included bork before
STARTOPT {
    LONGOPT {
        if (strcmp(option, "help") == 0)
            usage();
        else if (strcmp(option, "dostuff") == 0)
            dostuff(GETS());
        else
            die("Unknown option '%s'", option);
    } SHORTOPT {
        switch (option) {
            case 'h':
                usage();
                break;
            case 'D':
                dostuff(GETS());
                break;
            default:
                die("Unknown option: '%c'\n", option);
        }
    } NONOPT {
        printf("This ain't familia!: '%s'\n", nonopt);
    }
} ENDOPT;
```
The example is pretty self-explanatory. I will explain in detail in the next section.
I really suck in explaining things though so it might be better for you to open an issue and ask if you don't get the docs.

---

## API Docs or something like that
Some terms that I use are:
> `longopt(s)` - long option. bork identify a string as long  option if it starts with two dashes, eg: `--help`.
> `shortopt(s)` - short option. bork identify a string as short option if it starts with ONLY one dash, eg: `-h`.

There are only a few macros:
```c
STARTOPT
LONGOPT
SHORTOPT
NONOPT
ENDOPT
GETS()
```


### `STARTOPT`
This macro marks the start argument parsing. It must be followed by a code block and ended with `ENDOPT`.


### `LONGOPT`
This macro processes longopts.
This macro should always be present before `SHORTOPT` and `NONOPT`. It can be empty, eg: `LONGOPT {}`
> This macro exposes two variables:
> ```c
> char* option;  // the name of the option, without thedashes
> ```


### `SHORTOPT`
This macro processes shortopts.
This macro cannot appear independently - It MUST APPEAR AFTER `LONGOPT`. It can be empty too.
Note that bork don't care how "long" a shortopt is - as long it starts with only one dash, then it's automatically here.
This macro works almost the same as `LONGOPT`. Thus, the same rules apply.
> This macro exposes two variables:
> ```c
> char option;   // the name of the option
> ```


### `NONOPT`
This macro is used to process non-option arguments and is optional.
> This macro exposes two variables:
> ```c
> char* nonopt;  // the argument
> ```


### `ENDOPT`
This macro marks the end for argument processing. It MUST BE PRESENT and should not be followed by code blocks.


### `GETS()`
This macro is used to get optargs. It can only be called once per option.

## License
Unlicense or MIT-0. See [LICENSE](LICENSE).

## etc
[This website](https://wandbox.org/permlink/tFUsKMIXaQj8hhte) really helped me to debug the macros.
*/

#ifndef __BORK_H__
#define __BORK_H__

#include <string.h>

#define STARTOPT               \
for (int i = 1; i < argc; i++)

#define LONGOPT                               \
if (argv[i][0] == '-' && argv[i][1] == '-') { \
    char* option = argv[i] + 2;

#define SHORTOPT                                     \
} else if (argv[i][0] == '-' && argv[i][1] != '-') { \
    char option = argv[i][1];

#define NONOPT              \
} else {                    \
    char* nonopt = argv[i];

#define ENDOPT }

#define GETS()                 \
(char*)(                       \
    ((i + 1) <= (argc - 1)) && \
    argv[i + 1][0] != '-' &&   \
    argv[i + 1][1] != '-' ?    \
    argv[++i] :                \
    NULL                       \
)

#undef ARRSIZE
#endif

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2020 Takase

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/