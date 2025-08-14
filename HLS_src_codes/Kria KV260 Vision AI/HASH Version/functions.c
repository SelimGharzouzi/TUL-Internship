#include "functions.h"

/**************************  Global Variables Declarations ******************************/
static Dictionary dictionary[MAX_DICTIONARY_SIZE];
static bool dictionary_used[MAX_DICTIONARY_SIZE];

/**************************  Helper Functions Declarations ******************************/
void init_dictionary(void){
    #pragma HLS INLINE off
    memset(dictionary_used, 0, sizeof(dictionary_used));
    for (uint16_t i = 0; i < 256; i++){
        #pragma HLS PIPELINE II=1
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
        dictionary_used[i] = true;
    }
}

void Dictionary_reset(uint16_t *dictionary_size, uint8_t *bit_count) {
    #pragma HLS INLINE off
    (*dictionary_size) = 256;
    (*bit_count) = 8;
    init_dictionary();
}

uint32_t hash1(uint16_t prefix, uint8_t ext) {
    #pragma HLS INLINE
    return ((prefix << 8) ^ ext) & (MAX_DICTIONARY_SIZE - 1);
}

uint32_t hash2(uint16_t prefix, uint8_t ext) {
    #pragma HLS INLINE
    return (((prefix << 5) ^ (ext * 7)) & (MAX_DICTIONARY_SIZE - 1)) | 1;
}

uint16_t Dictionary_find(uint16_t prefix, uint8_t ext) {
    #pragma HLS INLINE off
    uint32_t h1 = hash1(prefix, ext);
    uint32_t h2 = hash2(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        uint32_t idx = (h1 + i * h2) & (MAX_DICTIONARY_SIZE - 1);

        if (!dictionary_used[idx]) return INVALID_CODE;

        if (dictionary[idx].prefix_code == prefix && dictionary[idx].ext_byte == ext) return dictionary[idx].code;
    }
    return INVALID_CODE;
}

void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count) {
    #pragma HLS INLINE off
    if (*dictionary_size >= MAX_DICTIONARY_SIZE) Dictionary_reset(dictionary_size, bit_count);
    if (*dictionary_size >= (1u << *bit_count)) (*bit_count)++;

    uint32_t h1 = hash1(prefix, ext);
    uint32_t h2 = hash2(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        uint32_t idx = (h1 + i * h2) & (MAX_DICTIONARY_SIZE - 1);

        if (!dictionary_used[idx]) {
            dictionary[idx].prefix_code = prefix;
            dictionary[idx].ext_byte = ext;
            dictionary[idx].code = *dictionary_size;
            dictionary_used[idx] = true;
            (*dictionary_size)++;
            return;
        }
    }
}

void write_output(uint16_t code, uint8_t *output, uint8_t bit_count, uint32_t *out_index) {
    #pragma HLS INLINE off
    uint32_t idx = *out_index;
    uint32_t byte_index = idx / 8;
    uint32_t bit_offset = idx % 8;

    uint32_t bits_left = bit_count;
    while (bits_left > 0) {
        #pragma HLS PIPELINE II=1
        uint8_t space_in_byte = 8 - bit_offset;
        uint8_t bits_to_write = (bits_left < space_in_byte) ? bits_left : space_in_byte;
        uint8_t mask = ((code >> (bits_left - bits_to_write)) & ((1U << bits_to_write) - 1));

        if (bit_offset == 0) output[byte_index] = 0;

        output[byte_index] |= mask << (space_in_byte - bits_to_write);
        bit_offset += bits_to_write;
        if (bit_offset == 8) {
            bit_offset = 0;
            byte_index++;
        }
        bits_left -= bits_to_write;
    }
    *out_index += bit_count;
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

    uint16_t dictionary_size = 256;
    uint8_t bit_count = 8;
    uint32_t out_index = 0;
    
    if (input_size == 0) {
        *compression_size = 0;
        return;
    }
    
    if (input_size == 1) {
        write_output(input[0], output, bit_count, &out_index);
        *compression_size = (out_index + 7) /8;
        return;
    }

    init_dictionary();

    uint16_t prefix = input[0];
    uint8_t ext = input[1];

    write_output(prefix, output, bit_count, &out_index);
    Dictionary_add(prefix, ext, &dictionary_size, &bit_count);
    prefix = ext;

    for (int i = 2; i < input_size; i++){ 
        uint8_t ext = input[i];
        uint16_t code = Dictionary_find(prefix, ext);
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