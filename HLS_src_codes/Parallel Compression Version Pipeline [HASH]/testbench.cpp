#include "functions.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{   
    int size = 9;
    uint8_t input[] = "ABAABAAAB";
    uint8_t outputs[NUMBER_PARALLEL_FUNCTIONS][32] = {{0}};
    uint32_t compression_sizes[NUMBER_PARALLEL_FUNCTIONS] = {0};

    int part_size = size / NUMBER_PARALLEL_FUNCTIONS;
    int remainder = size % NUMBER_PARALLEL_FUNCTIONS;
    int sizes[NUMBER_PARALLEL_FUNCTIONS];
    int offsets[NUMBER_PARALLEL_FUNCTIONS];

    for (int i = 0; i < NUMBER_PARALLEL_FUNCTIONS; i++) {
        sizes[i] = part_size + (i < remainder ? 1 : 0);
        offsets[i] = (i == 0) ? 0 : offsets[i-1] + sizes[i-1];
    }

    top_parallel_lzw(
        input + offsets[0], sizes[0],
        input + offsets[1], sizes[1], 
        outputs[0], &compression_sizes[0],
        outputs[1], &compression_sizes[1]
    );

    printf("\n");
    for (int i = 0; i < NUMBER_PARALLEL_FUNCTIONS; ++i) {
        printf("Compression_size[%d] = %u\n", i + 1, compression_sizes[i]);
    }
    printf("\n");

    for (int i = 0; i < NUMBER_PARALLEL_FUNCTIONS; ++i) {
        printf("Output of %dth lzw_compress:\n", i + 1);
        for (uint32_t j = 0; j < compression_sizes[i]; ++j) {
            printf("output%d[%u] = %u\n", i + 1, j, outputs[i][j]);
        }
        printf("----------------------------------------------------------------\n");
    }

    return 0;
}