#include "function.h"

int main()
{
    Dictionary_init_Decompression();

    FILE *finput = fopen("input_Decompression.bin", "rb");
    FILE *foutput = fopen("output.bin", "wb");


    if (!finput || !foutput) {
        perror("Error opening file");
        return 1;
    }

    uint8_t inputb[100000];
    size_t bite_read = fread(inputb, 1, sizeof(inputb) - 1, finput);


    if (bite_read == 0 && ferror(finput)) {
        perror("Error reading input file");
        fclose(finput);
        fclose(foutput);
        return 1;
    }
    else if(bite_read == sizeof(inputb) - 1) {
        printf("Input file too large, truncating to fit buffer.\n");
    }
    else{
        decompress(inputb, bite_read, foutput);
    }

    Dictionary_print_Decompression();

    fclose(finput);
    fclose(foutput);
    return 0;
}