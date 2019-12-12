#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int c = 0;
    int i = 0;
    int isFirstByte = 1;
    
    if (argc != 4) {
        fprintf(stderr, "usage: %s <firmware-file> <output-headerfile> <array-name>\n", argv[0]);
        fprintf(stderr, "Generate a C header file with an array <array-name>\n");
        fprintf(stderr, "from the specified binary firmware file (bin, dat).\n");
        return -1;
    }
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "failed to open firmware input file\n");
        return -1;
    }
    
    FILE *b = fopen(argv[2], "w");
    if (!b) {
        fprintf(stderr, "failed to open firmware output file\n");
        return -1;
    }

    fprintf(b, "// Firmware BLOB - automatically generated\n");
    fprintf(b, "\n");
    fprintf(b, "#ifndef __FW_BLOB_%s_H__\n", argv[3]);
    fprintf(b, "#define __FW_BLOB_%s_H__ 1\n", argv[3]);
    fprintf(b, "\n");

    fprintf(b, "uint8_t %s[] = {\n", argv[3]);
    while (EOF != (c = fgetc(f))) {
        if (i == 0) {
            fprintf(b, "   ");
        }
        if (i > 0 || !isFirstByte) {
            fprintf(b,",");
        }
        fprintf(b, " 0x%02x", c);
        i++;
        if (i == 16) {
            fprintf(b, "\n");
            i = 0;
        }
        isFirstByte = 0;
    }
    fprintf(b, "};\n");

    fprintf(b, "\n");
    fprintf(b, "#endif // __FW_BLOB_%s_H__\n", argv[3]);

    fclose(b);
    fclose(f);
    return 0;
}
