#include "functions.h"
#include <stdio.h>

int main(void)
{   
    int size = 9;
    uint8_t input[] = "ABAABAAAB";
    uint8_t output[32] = {0};
    uint32_t compression_size = 0;

    lzw_compress(input, output, size, &compression_size);

    printf("Compression size = %u\n", compression_size);

    for (uint32_t i = 0; i < compression_size; i++) {
        printf("output[%u] = %u\n", i, output[i]);
    }

    return 0;
}
