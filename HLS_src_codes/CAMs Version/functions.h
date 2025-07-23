#ifndef FUNCTION_H
#define FUNCTION_H

/************************** Include Files ******************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/************************** Constant Definitions ******************************/
#define MAX_DICTIONARY_SIZE  4096           // Maximum size of the LZW dictionary (12-bit codes)
#define INVALID_CODE         0xFFFF         // Used to represent an invalid or non-existent code

/**************************** Type Definitions *******************************/
/**
 * @brief Dictionary entry used for LZW compression.
 */
typedef struct {
    uint16_t prefix_code;  // Code of the previous sequence
    uint8_t ext_byte;      // New byte to add to the sequence
    uint16_t code;         // Assigned code for the new sequence
} Dictionary;

/************************** Helper Function Declarations ******************************/
/**
 * @brief Initializes the dictionary with single-byte entries.
 */
void init_dictionary(void);

/**
 * @brief Finds the code for a given prefix + extension entry in the dictionary.
 *
 * @param prefix                The prefix code.
 * @param ext                   The extension byte.
 * @param dictionary_size       Current dicitionary size.
 *
 * @return        The code for the sequence if found, INVALID_CODE otherwise.
 */
uint16_t Dictionary_find(uint16_t prefix, uint8_t ext, uint16_t dictionary_size);

/**
 * @brief Adds a new prefix + extension to the dictionary.
 *
 * @param prefix          Prefix code.
 * @param ext             Extension byte.
 * @param dictionary_size Pointer to current dictionary size (updated internally).
 * @param bit_count       Pointer to current code bit width.
 */
void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count);

/**
 * @brief Writes a code into the output buffer using a specific bit width.
 *
 * @param code       Code to write.
 * @param output     Output buffer.
 * @param bit_count  Current bit width for codes.
 * @param out_index  Pointer to current output bit index.
 */
void write_output(uint16_t code, uint8_t *output, uint8_t bit_count, uint32_t *out_index);

/************************** Main Function Declaration ******************************/

/**
 * @brief LZW compression function for HLS.
 *        Takes an input buffer and writes a compressed output buffer.
 *
 * @param input   Pointer to input data buffer.
 * @param output  Pointer to output buffer (bit-packed).
 * @param input_size    Input data size
 * @param compression_size Compression data
 */
void lzw_compress(uint8_t *input, uint8_t *output, int input_size, uint32_t *compression_size);

#endif
