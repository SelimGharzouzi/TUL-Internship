#include "functions.h"

/**************************  Helper Functions Declarations ******************************/
void init_dictionary(Dictionary *dictionary) {
    #pragma HLS INLINE off
    for (uint16_t i = 0; i < 256; i++){
        #pragma HLS PIPELINE II=1
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
    }
}

uint16_t Dictionary_find(uint16_t prefix, uint8_t ext, uint16_t dictionary_size, Dictionary *dictionary) {
    #pragma HLS INLINE off
    for (uint16_t i = 0; i < dictionary_size; i++) {
    #pragma HLS UNROLL factor=8
        if (dictionary[i].prefix_code == prefix && dictionary[i].ext_byte == ext) {
            return dictionary[i].code;
        }
    }

    return INVALID_CODE;
}

void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count, Dictionary *dictionary) {
    #pragma HLS INLINE off
    if (*dictionary_size >= MAX_DICTIONARY_SIZE) return;
    if (*dictionary_size >= (1u << *bit_count)) (*bit_count)++;

    dictionary[*dictionary_size].prefix_code = prefix;
    dictionary[*dictionary_size].ext_byte = ext;
    dictionary[*dictionary_size].code = *dictionary_size;

    (*dictionary_size)++;
}

void write_output(uint16_t code, uint8_t *output, uint8_t bit_count, uint32_t *out_index){
    #pragma HLS INLINE off
    for (int i = bit_count - 1; i >= 0; i--){
        #pragma HLS PIPELINE II=1
        bool bit = (code >> i) & 1;
        uint32_t byte_index = (*out_index)/8;
        uint8_t bit_pos = 7 - ((*out_index)%8);

        output[byte_index] |= (bit << bit_pos);
        (*out_index)++;
    }
}

/**************************  Main Compression Function Declaration ******************************/
void lzw_compress(uint8_t *input, uint8_t *output, int input_size, uint32_t *compression_size){
    #pragma HLS INLINE off

    Dictionary dictionary[MAX_DICTIONARY_SIZE];
    #pragma HLS bind_storage variable=dictionary type=RAM_1P impl=bram

    uint16_t dictionary_size = 256;
    uint8_t bit_count = 8;
    uint32_t out_index = 0;

    for(int i = 0; i < (input_size*2); i++){
        #pragma HLS PIPELINE II=1
        output[i] = 0; 
    }

    init_dictionary(dictionary);

    uint16_t prefix = input[0];
    uint8_t ext = input[1];

    write_output(prefix, output, bit_count, &out_index);
    Dictionary_add(prefix, ext, &dictionary_size, &bit_count, dictionary);
    prefix = ext;

    for (int i = 2; i < input_size; i++){ 
        uint8_t ext = input[i];
        uint16_t code = Dictionary_find(prefix, ext, dictionary_size, dictionary);
        if (code != INVALID_CODE){
            prefix = code;
        } else {
            write_output(prefix, output, bit_count, &out_index);
            Dictionary_add(prefix, ext, &dictionary_size, &bit_count, dictionary);
            prefix = ext;
        }
    }
    write_output(prefix, output, bit_count, &out_index);

    while (out_index % 8 != 0) {
        output[out_index / 8] |= (0 << (7 - out_index % 8));
        out_index++;
    }
    
    *compression_size = (out_index + 7) /8;
}

/**************************  Main Parallel Compression Function Declaration ******************************/
void top_parallel_lzw(
    uint8_t* input, int input_size,
    uint8_t* output1, uint32_t* compression_size1,
    uint8_t* output2, uint32_t* compression_size2,
    uint8_t* output3, uint32_t* compression_size3,
    uint8_t* output4, uint32_t* compression_size4,
    uint8_t* output5, uint32_t* compression_size5,
    uint8_t* output6, uint32_t* compression_size6,
    uint8_t* output7, uint32_t* compression_size7,
    uint8_t* output8, uint32_t* compression_size8,
    uint8_t* output9, uint32_t* compression_size9,
    uint8_t* output10, uint32_t* compression_size10,
    uint8_t* output11, uint32_t* compression_size11
    ) {
    #pragma HLS INTERFACE m_axi depth=input_size port=input     offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output1   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output2   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output3   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output4   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output5   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output6   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output7   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output8   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output9   offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output10  offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output11  offset=slave bundle=AXIM_A

    #pragma HLS INTERFACE s_axilite port=input      bundle=control
    #pragma HLS INTERFACE s_axilite port=output1    bundle=control
    #pragma HLS INTERFACE s_axilite port=output2    bundle=control
    #pragma HLS INTERFACE s_axilite port=output3    bundle=control
    #pragma HLS INTERFACE s_axilite port=output4    bundle=control
    #pragma HLS INTERFACE s_axilite port=output5    bundle=control
    #pragma HLS INTERFACE s_axilite port=output6    bundle=control
    #pragma HLS INTERFACE s_axilite port=output7    bundle=control
    #pragma HLS INTERFACE s_axilite port=output8    bundle=control
    #pragma HLS INTERFACE s_axilite port=output9    bundle=control
    #pragma HLS INTERFACE s_axilite port=output10   bundle=control
    #pragma HLS INTERFACE s_axilite port=output11   bundle=control

    #pragma HLS INTERFACE s_axilite port=return         bundle=control
    #pragma HLS INTERFACE s_axilite port=input_size     bundle=control

    #pragma HLS INTERFACE s_axilite port=compression_size1 bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size2  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size3  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size4  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size5  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size6  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size7  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size8  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size9  bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size10 bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size11 bundle=control

    int part_size = input_size / NUMBERS_FUNCTIONS_PARALLEL;
    int remainder = input_size % NUMBERS_FUNCTIONS_PARALLEL;
    int sizes[NUMBERS_FUNCTIONS_PARALLEL];
    int offsets[NUMBERS_FUNCTIONS_PARALLEL];

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        sizes[i] = part_size + (i < remainder ? 1 : 0);
        offsets[i] = (i == 0) ? 0 : offsets[i-1] + sizes[i-1];
    }

    #pragma HLS DATAFLOW 
    lzw_compress(input + offsets[0], output1, sizes[0], compression_size1);
    lzw_compress(input + offsets[1], output2, sizes[1], compression_size2);
    lzw_compress(input + offsets[2], output3, sizes[2], compression_size3);
    lzw_compress(input + offsets[3], output4, sizes[3], compression_size4);
    lzw_compress(input + offsets[4], output5, sizes[4], compression_size5);
    lzw_compress(input + offsets[5], output6, sizes[5], compression_size6);
    lzw_compress(input + offsets[6], output7, sizes[6], compression_size7);
    lzw_compress(input + offsets[7], output8, sizes[7], compression_size8);
    lzw_compress(input + offsets[8], output9, sizes[8], compression_size9);
    lzw_compress(input + offsets[9], output10, sizes[9], compression_size10);
    lzw_compress(input + offsets[10], output11, sizes[10], compression_size11);
}