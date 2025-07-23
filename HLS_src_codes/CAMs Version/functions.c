#include "functions.h"

/**************************  Global Variables Declarations ******************************/
Dictionary dictionary[MAX_DICTIONARY_SIZE];

/**************************  Helper Functions Declarations ******************************/
void init_dictionary(void){
    #pragma HLS INLINE off
    for (uint16_t i = 0; i < 256; i++){
        #pragma HLS UNROLL factor=16
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
    }
}

uint16_t Dictionary_find(uint16_t prefix, uint8_t ext, uint16_t dictionary_size) {
    #pragma HLS INLINE off
    for (uint16_t i = 0; i < dictionary_size; i++) {
    #pragma HLS UNROLL factor=8
        if (dictionary[i].prefix_code == prefix && dictionary[i].ext_byte == ext) {
            return dictionary[i].code;
        }
    }

    return INVALID_CODE;
}

void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count) {
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

/**************************  Main Function Declaration ******************************/
void lzw_compress(uint8_t *input, uint8_t *output, int input_size, uint32_t *compression_size){
    #pragma HLS INTERFACE m_axi depth=input_size port=input offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE m_axi depth=input_size port=output offset=slave bundle=AXIM_A
    #pragma HLS INTERFACE s_axilite port=input   bundle=control
    #pragma HLS INTERFACE s_axilite port=output  bundle=control
    #pragma HLS INTERFACE s_axilite port=return  bundle=control
    #pragma HLS INTERFACE s_axilite port=input_size    bundle=control
    #pragma HLS INTERFACE s_axilite port=compression_size bundle=control

    #pragma HLS bind_storage variable=dictionary type=RAM_1P impl=bram

    uint16_t dictionary_size = 256;
    uint8_t bit_count = 8;
    uint32_t out_index = 0;

    for(int i = 0; i < (input_size*2); i++){
        #pragma HLS PIPELINE II=1
        output[i] = 0; 
    }

    init_dictionary();

    uint16_t prefix = input[0];
    uint8_t ext = input[1];

    write_output(prefix, output, bit_count, &out_index);
    Dictionary_add(prefix, ext, &dictionary_size, &bit_count);
    prefix = ext;

    for (int i = 2; i < input_size; i++){ 
        uint8_t ext = input[i];
        uint16_t code = Dictionary_find(prefix, ext, dictionary_size);
        if (code != INVALID_CODE){
            prefix = code;
        } else {
            write_output(prefix, output, bit_count, &out_index);
            Dictionary_add(prefix, ext, &dictionary_size, &bit_count);
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
