#include "function.h"

int main() {

    Dictionary_init_Compression();
    Dictionary_init_Decompression();

    uint8_t input[100000];

    printf("Enter your input string: ");
    fgets((char*)input, sizeof(input), stdin);

    size_t input_len = strlen((char*)input);

    if (input[input_len - 1] == '\n') {
        input[input_len - 1] = '\0';
        input_len--;
    }

    compress(input, input_len);

    print_bitstream_compression();
    Dictionary_print_compression();
    decompress1();

    printf("\n");

    print_bitstream_decompression();
    Dictionary_print_Decompression();

    return 0;
}

