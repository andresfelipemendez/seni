#ifndef SENI_EMBED_H
#define SENI_EMBED_H

/* Embeds a header file's bytes into the binary at build time and exports
   them as `const char* seni_layout`. The hot-reloader reads this symbol from
   the currently-loaded dll to obtain the layout it was compiled with -- the
   "old header" for diffing -- since by reload time the header file on disk
   has already been overwritten with the new layout.

   Usage (file scope, once per dll):
       #include "seni_embed.h"
       SENI_EMBED_LAYOUT("path/to/structs.h");

   The path is resolved by the assembler relative to the compiler's working
   directory. Bytes are byte-identical to the file -- no escaping, no codegen. */

#if defined(_WIN32)
#define SENI_EMBED_EXPORT __declspec(dllexport)
#define SENI_EMBED_SECTION ".rdata"
#else
#define SENI_EMBED_EXPORT
#define SENI_EMBED_SECTION ".rodata"
#endif

#define SENI_EMBED_LAYOUT(path) \
    __asm__(".section " SENI_EMBED_SECTION "\n" \
            "seni_layout_data:\n" \
            ".incbin \"" path "\"\n" \
            ".byte 0\n" \
            ".text\n"); \
    extern const char seni_layout_data[]; \
    SENI_EMBED_EXPORT const char* seni_layout = seni_layout_data

#endif
