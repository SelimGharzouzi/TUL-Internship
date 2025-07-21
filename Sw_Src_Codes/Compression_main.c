#include "function.h"

int main() {
    Dictionary_init_Compression();

    FILE *foutput = fopen("input_Decompression.bin", "wb");

    if (!foutput) {
        perror("Error opening output file");
        return 1;
    }

    uint8_t data[1024];

    printf("Enter your input string: ");
    fgets((char*)data, sizeof(data), stdin);

    size_t input_len = strlen((char*)data);

    if (input_len > 0 && data[input_len - 1] == '\n') {
        data[input_len - 1] = '\0';
        input_len--;
    }

    compress(data, input_len);
    print_bitstream_compression();

    print_bitsteam_file_compression(foutput);
    Dictionary_print_compression();

    fclose(foutput);
    return 0;
}
