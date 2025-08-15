#include "xlzw_compress.h"
#include "input.h"
#include "xparameters.h"
#include "xil_cache.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <xil_types.h>
#include <stdbool.h>
#include <xstatus.h>

#define MAX_DICTIONARY_SIZE 4096
#define INVALID_CODE 0xFFFF
#define FILE_INPUT_SIZE 4*1024*1024

static uint8_t output[2 * FILE_INPUT_SIZE] = {0};
static uint8_t output_sw[2 * FILE_INPUT_SIZE] = {0};

typedef struct {
    uint16_t prefix_code;
    uint8_t ext_byte;     
    uint16_t code;         
} DictionaryEntry;

static DictionaryEntry dictionary[MAX_DICTIONARY_SIZE];
static bool dictionary_used[MAX_DICTIONARY_SIZE];

static void init_dictionary(void){
    memset(dictionary_used, 0, sizeof(dictionary_used));
    for (uint16_t i = 0; i < 256; i++){
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
        dictionary_used[i] = true;
    }
}

void Dictionary_reset(uint16_t *dictionary_size, uint8_t *bit_count) {
    (*dictionary_size) = 256;
    (*bit_count) = 8;
    init_dictionary();
}

uint32_t hash1(uint16_t prefix, uint8_t ext) {
    return ((prefix << 8) ^ ext) & (MAX_DICTIONARY_SIZE - 1);
}

uint32_t hash2(uint16_t prefix, uint8_t ext) {
    return (((prefix << 5) ^ (ext * 7)) & (MAX_DICTIONARY_SIZE - 1)) | 1;
}

uint16_t Dictionary_find(uint16_t prefix, uint8_t ext) {
    uint32_t h1 = hash1(prefix, ext);
    uint32_t h2 = hash2(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        uint32_t idx = (h1 + i * h2) & (MAX_DICTIONARY_SIZE - 1);
        if (!dictionary_used[idx]) return INVALID_CODE;
        if (dictionary[idx].prefix_code == prefix && dictionary[idx].ext_byte == ext)
            return dictionary[idx].code;
    }
    return INVALID_CODE;
}

void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count) {
    if (*dictionary_size >= MAX_DICTIONARY_SIZE) Dictionary_reset(dictionary_size, bit_count);
    if (*dictionary_size >= (1u << *bit_count)) (*bit_count)++;

    uint32_t h1 = hash1(prefix, ext);
    uint32_t h2 = hash2(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
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

static void write_output(uint16_t code, uint8_t *output, uint8_t bit_count, uint32_t *out_index){
    uint32_t idx = *out_index;
    uint32_t byte_index = idx / 8;
    uint32_t bit_offset = idx % 8;

    uint32_t bits_left = bit_count;
    while (bits_left > 0) {
        uint8_t bits_in_this_byte = 8 - bit_offset;
        if (bits_in_this_byte > bits_left) bits_in_this_byte = bits_left;
        uint8_t mask = (code >> (bits_left - bits_in_this_byte)) & ((1U << bits_in_this_byte) - 1);
        output[byte_index] |= mask << (8 - bit_offset - bits_in_this_byte);
        bits_left -= bits_in_this_byte;
        bit_offset = 0;
        byte_index++;
    }
    *out_index += bit_count;
}

void lzw_compress_sw(uint8_t *input, uint8_t *output, int input_size, uint32_t *compression_size){
    uint16_t dictionary_size = 256;
    uint8_t bit_count = 8;
    uint32_t out_index = 0;

    memset(output, 0, input_size * 2);

    init_dictionary();

    uint16_t prefix = input[0];
    for (int i = 1; i < input_size; i++) {
        uint8_t ext = input[i];
        uint16_t code = Dictionary_find(prefix, ext);
        if (code != INVALID_CODE) {
            prefix = code;
        } else {
            write_output(prefix, output, bit_count, &out_index);
            Dictionary_add(prefix, ext, &dictionary_size, &bit_count);
            prefix = ext;
        }
    }
    write_output(prefix, output, bit_count, &out_index);

    *compression_size = (out_index + 7) / 8;
}

uint32_t read_counter_frequency(void) {
    uint32_t val;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

uint64_t read_counter_value(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

void print_decimal(const uint8_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        printf("%u ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main() {
    XLzw_compress compressor;
    uint64_t start_sw, end_sw, start_hw, end_hw;
    int status;
    uint32_t compression_size_sw = 0;

    int input_length = input_txt_len;

    printf("\n-------------------------------------- Test 1 --------------------------------------\n");

    uint32_t freq = read_counter_frequency();

    printf("\n-------------------------------------- SW --------------------------------------\n");

    start_sw = read_counter_value();

    lzw_compress_sw(input_txt, output_sw, input_length, &compression_size_sw);

    end_sw = read_counter_value();

    uint64_t elapsed_cycles_sw = end_sw - start_sw;
    double elapsed_time_sw = (double)elapsed_cycles_sw / freq;

    printf("SW compression time: %.6f seconds\r\n", elapsed_time_sw);

    printf("\n-------------------------------------- HW --------------------------------------\n");

    Xil_DCacheFlushRange((UINTPTR)input_txt, input_length);
    Xil_DCacheFlushRange((UINTPTR)output, input_length*2);

    status = XLzw_compress_Initialize(&compressor, XPAR_LZW_COMPRESS_0_BASEADDR);
    if (status != XST_SUCCESS) {
        printf("Failed to initialize Lzw_compress HW, %d\r\n", status);
        return 1;
    }

    XLzw_compress_Set_input_r(&compressor, (UINTPTR)input_txt);
    XLzw_compress_Set_output_r(&compressor, (UINTPTR)output);
    XLzw_compress_Set_input_size(&compressor, input_length);
    
    start_hw = read_counter_value();

    XLzw_compress_Start(&compressor);
    while (!XLzw_compress_IsDone(&compressor));

    end_hw = read_counter_value();

    uint64_t elapsed_cycles = end_hw - start_hw;
    double elapsed_time_sec = (double)elapsed_cycles / freq;

    printf("HW compression time: %.6f seconds\r\n", elapsed_time_sec);

    printf("\n-------------------------------------- Final Results --------------------------------------\n");

    uint32_t compression_size = XLzw_compress_Get_compression_size(&compressor);

    Xil_DCacheInvalidateRange((UINTPTR)output, input_length);

    if (compression_size > (uint32_t)(input_length*2)) {
        printf("Warning: compression_size (%lu) exceeds buffer size\n", (unsigned long)compression_size);
        compression_size = (uint32_t)(input_length*2);
    } else if (compression_size < (uint32_t)input_length){
        printf("Compression size (%lu) smaller than initial input size (%d)\n", (unsigned long)compression_size, input_length);
    } else {
        printf("Compression size (%lu) is larger than initial input size (%d)\n", (unsigned long)compression_size, input_length);
    }

    printf("Compression ratio: %.2f%%\n", 100.0 * (double)compression_size / input_length);

    if (compression_size == compression_size_sw &&
        memcmp(output, output_sw, compression_size) == 0) {
        printf("Outputs match.\n");
    } else {
        printf("Outputs do not match.\n");
    }

    if (elapsed_time_sw > elapsed_time_sec) {
        printf("Hardware was %.2f%% faster\n", 100.0 * (elapsed_time_sw - elapsed_time_sec) / elapsed_time_sw);
    } else if (elapsed_time_sw < elapsed_time_sec) {
        printf("Software was %.2f%% faster\n", 100.0 * (elapsed_time_sec - elapsed_time_sw) / elapsed_time_sec);
    } else {
        printf("Software was as fast as Hardware\n");
    }

    //print_decimal(output, compression_size);

    return 0;
}
