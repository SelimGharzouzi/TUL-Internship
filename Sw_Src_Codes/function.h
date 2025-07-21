#ifndef FUNCTION_H
#define FUNCTION_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define MAX_SEQUENCE_LENGTH 256
#define MAX_DICT_SIZE 4096
#define INVALID_CODE 0xFFFF
#define INVALID_CODE1 0xFF

typedef struct {
    uint16_t prefix_code;
    uint8_t ext_byte;
    uint16_t code;
} DictionaryCompression;

typedef struct {
    uint16_t code;
    uint8_t sequence[MAX_SEQUENCE_LENGTH];
    uint16_t length;
} DictionaryDecompression;

// ------------------------------------------------------------------------------------
/*
 *                              Functions for Compression
 */
// ------------------------------------------------------------------------------------

void Dictionary_init_Compression();
uint16_t Dictionary_find_compression(uint16_t prefix, uint8_t ext);
void Dictionary_add_compression(uint16_t prefix, uint8_t ext);
void Dictionary_print_compression();
void compress(const uint8_t *input, size_t input_len);
void print_bitstream_compression();
void write_to_bitstream_compression(uint16_t code);
void print_bitsteam_file_compression(FILE *foutput);

// ------------------------------------------------------------------------------------
/*
 *                              Functions for Decompression
 */
// ------------------------------------------------------------------------------------

void Dictionary_init_Decompression();
DictionaryDecompression* Dictionary_find_Decompression(uint16_t code);
void Dictionary_add_Decompression(const uint8_t *sequence, uint16_t seq_len);
void decompress(const uint8_t *inputb, size_t len, FILE *foutput);
void Dictionary_print_Decompression();
uint16_t Binary(int bit_count, const uint16_t *bin);


// -------------------------------------------------------------------------------------
/*
 *                               Functions for main
 */
// -------------------------------------------------------------------------------------

uint16_t read_from_bitstream();
void write_to_bitstream_decompression(uint8_t sequence);
void decompress1();
void print_bitstream_decompression();





#endif
