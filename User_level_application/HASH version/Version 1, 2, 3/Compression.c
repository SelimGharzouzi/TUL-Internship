#include "xlzw_compress.h"
#include "xparameters.h"
#include "xil_cache.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <xil_types.h>
#include "xscutimer.h"
#include <stdbool.h>
#include <xstatus.h>
#include "ff.h"

#define MAX_DICTIONARY_SIZE 4096
#define INVALID_CODE 0xFFFF
#define FILE_INPUT_SIZE 4*1024*1024

static uint8_t input[FILE_INPUT_SIZE];
static uint8_t output[2 * FILE_INPUT_SIZE] = {0};
static uint8_t output_sw[2 * FILE_INPUT_SIZE] = {0};

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "input.txt";

typedef struct {
    uint16_t prefix_code;
    uint8_t ext_byte;     
    uint16_t code;         
} DictionaryEntry;

static DictionaryEntry dictionary[MAX_DICTIONARY_SIZE];
static bool dictionary_used[MAX_DICTIONARY_SIZE];

static inline uint32_t hash(uint16_t prefix, uint8_t ext) {
    return ((prefix << 5) ^ ext) % MAX_DICTIONARY_SIZE;
}

static void init_dictionary(void){
    memset(dictionary_used, 0, sizeof(dictionary_used));
    for (uint16_t i = 0; i < 256; i++){
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = i;
        dictionary_used[i] = true;
    }
}

static uint16_t Dictionary_find(uint16_t prefix, uint8_t ext) {
    uint32_t h = hash(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        uint32_t idx = (h + i) % MAX_DICTIONARY_SIZE;
        if (!dictionary_used[idx]) return INVALID_CODE;
        if (dictionary[idx].prefix_code == prefix && dictionary[idx].ext_byte == ext)
            return dictionary[idx].code;
    }
    return INVALID_CODE;
}

static void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count) {
    if (*dictionary_size >= MAX_DICTIONARY_SIZE) return;
    if (*dictionary_size >= (1u << *bit_count)) (*bit_count)++;
    uint32_t h = hash(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICTIONARY_SIZE; i++) {
        uint32_t idx = (h + i) % MAX_DICTIONARY_SIZE;
        if (!dictionary_used[idx]) {
            dictionary[idx].prefix_code = prefix;
            dictionary[idx].ext_byte = ext;
            dictionary[idx].code = *dictionary_size;
            dictionary_used[idx] = true;
            (*dictionary_size)++;
            break;
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

int ReadSD(uint8_t *input, int *input_length){
    FRESULT Res;
    UINT NumBytesRead;

    Res = f_mount(&fatfs, Path, 0);
    if (Res != FR_OK) {
        printf("Mount failed\n");
        return XST_FAILURE;
    }

    Res = f_open(&fil, finput, FA_READ);
    if (Res != FR_OK) {
        printf("Open failed\n");
        return XST_FAILURE;
    }

    Res = f_read(&fil, input, sizeof(input[0]) * (4 * FILE_INPUT_SIZE), &NumBytesRead);
    if (Res != FR_OK){
        printf("Read failed\n");
        printf("Res = %d\n", Res);
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);

    printf("Read %u bytes from SD card.\n", NumBytesRead);
    (* input_length) = NumBytesRead;

    return XST_SUCCESS;
}

static inline uint64_t get_global_time() {
    volatile uint32_t *timer_lo = (volatile uint32_t *)(XPAR_PS7_GLOBALTIMER_0_BASEADDR);
    volatile uint32_t *timer_hi = (volatile uint32_t *)(XPAR_PS7_GLOBALTIMER_0_BASEADDR + 4);
    uint32_t hi1, lo, hi2;
    do {
        hi1 = *timer_hi;
        lo = *timer_lo;
        hi2 = *timer_hi;
    } while (hi1 != hi2);
    return ((uint64_t)hi1 << 32) | lo;
}

int main() {
    XLzw_compress compressor;
    uint64_t start_sw, end_sw, start_hw, end_hw;
    int status;
    uint32_t compression_size_sw = 0;

    int input_length = 0;

    printf("\n-------------------------------------- Test 6 --------------------------------------\n");

    status = ReadSD(input, &input_length);
    if (status != XST_SUCCESS) {
        printf("Failed to read sd card, %d\r\n", status);
    }

    printf("\n-------------------------------------- SW --------------------------------------\n");

    start_sw = get_global_time();

    lzw_compress_sw(input, output_sw, input_length, &compression_size_sw);

    end_sw = get_global_time();

    uint64_t elapsed_cycles_sw = end_sw - start_sw;
    double elapsed_time_sw = (double)elapsed_cycles_sw / XPAR_CPU_CORE_CLOCK_FREQ_HZ;

    printf("SW compression time: %.6f seconds\r\n", elapsed_time_sw);

    printf("\n-------------------------------------- HW --------------------------------------\n");

    Xil_DCacheFlushRange((UINTPTR)input, input_length);
    Xil_DCacheFlushRange((UINTPTR)output, input_length*2);

    status = XLzw_compress_Initialize(&compressor, XPAR_LZW_COMPRESS_0_BASEADDR);
    if (status != XST_SUCCESS) {
        printf("Failed to initialize Lzw_compress HW, %d\r\n", status);
        return 1;
    }

    XLzw_compress_Set_input_r(&compressor, (UINTPTR)input);
    XLzw_compress_Set_output_r(&compressor, (UINTPTR)output);
    XLzw_compress_Set_input_size(&compressor, input_length);

    start_hw = get_global_time();

    XLzw_compress_Start(&compressor);
    while (!XLzw_compress_IsDone(&compressor));

    end_hw = get_global_time();

    uint64_t elapsed_cycles = end_hw - start_hw;
    double elapsed_time_sec = (double)elapsed_cycles / XPAR_CPU_CORE_CLOCK_FREQ_HZ;

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

    return 0;
}
