#include "functions.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{   
    int size = 9;
    uint8_t input[] = "ABAABAAAB";
    uint8_t output1[32] = {0};
    uint8_t output2[32] = {0};
    uint32_t compression_size1 = 0;
    uint32_t compression_size2 = 0;

    top_parallel_lzw(input, size, output1, &compression_size1, output2, &compression_size2);

    printf("Compression_size[1] = %u\n", compression_size1);
    printf("Compression_size[2] = %u\n\n", compression_size2);

    printf("Output of first lzw_compress:\n");
    for (uint32_t i = 0; i < compression_size1; i++) {
        printf("output1[%u] = %u\n", i, output1[i]);
    }

    printf("----------------------------------------------------------------\n");
    printf("Output of second lzw_compress:\n");
    for (uint32_t i = 0; i < compression_size2; i++) {
        printf("output2[%u] = %u\n", i, output2[i]);
    }

    return 0;
}