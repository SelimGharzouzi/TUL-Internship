#include "functions.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{   
    int size = 9;
    uint8_t input[] = "ABAABAAAB";
    uint8_t outputs[NUMBERS_FUNCTIONS_PARALLEL][32] = {{0}};
    uint32_t compression_sizes[NUMBERS_FUNCTIONS_PARALLEL] = {0};

    top_parallel_lzw(
        input, size,
        outputs[0], &compression_sizes[0],
        outputs[1], &compression_sizes[1],
        outputs[2], &compression_sizes[2]
    );

    printf("\n");
    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; ++i) {
        printf("Compression_size[%d] = %u\n", i + 1, compression_sizes[i]);
    }
    printf("\n");

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; ++i) {
        printf("Output of %dth lzw_compress:\n", i + 1);
        for (uint32_t j = 0; j < compression_sizes[i]; ++j) {
            printf("output%d[%u] = %u\n", i + 1, j, outputs[i][j]);
        }
        printf("----------------------------------------------------------------\n");
    }

    return 0;
}