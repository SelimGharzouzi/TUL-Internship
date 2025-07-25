#include "functions.h"

/**************************  Helper Functions Declarations ******************************/
void init_dictionary(Dictionary *dictionary){
    #pragma HLS INLINE off
    for (uint16_t i = 0; i < 256; i++){
        #pragma HLS PIPELINE II=1
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
    }
}

uint32_t hash(uint16_t prefix, uint8_t ext) {
    #pragma HLS INLINE
    return ((prefix << 5) ^ ext) % MAX_DICTIONARY_SIZE;
}

uint16_t Dictionary_find(Dictionary *dictionary, uint16_t prefix, uint8_t ext) {
    #pragma HLS INLINE off
    uint32_t h = hash(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        uint32_t idx = (h + i) % MAX_DICTIONARY_SIZE;

        if (dictionary[idx].prefix_code == prefix && dictionary[idx].ext_byte == ext) return dictionary[idx].code;
    }

    return INVALID_CODE;
}

void Dictionary_add(Dictionary *dictionary, uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count) {
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
    Dictionary_add(dictionary, prefix, ext, &dictionary_size, &bit_count);
    prefix = ext;

    for (int i = 2; i < input_size; i++){ 
        uint8_t ext = input[i];
        uint16_t code = Dictionary_find(dictionary, prefix, ext);
        if (code != INVALID_CODE){
            prefix = code;
        } else {
            write_output(prefix, output, bit_count, &out_index);
            Dictionary_add(dictionary, prefix, ext, &dictionary_size, &bit_count);
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
    uint8_t* output2, uint32_t* compression_size2
    ) {
    #pragma HLS INTERFACE m_axi depth=input_size port=input offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output1 offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output2 offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE s_axilite port=input   bundle=control
    #pragma HLS INTERFACE s_axilite port=output1  bundle=control
    #pragma HLS INTERFACE s_axilite port=output2 bundle=control
    #pragma HLS INTERFACE s_axilite port=return  bundle=control
    #pragma HLS INTERFACE s_axilite port=input_size    bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size1 bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size2 bundle=control

    #pragma HLS DATAFLOW
    
    lzw_compress(input, output1, input_size/2, compression_size1);
    lzw_compress(input + input_size/2, output2, input_size - input_size/2, compression_size2);
}