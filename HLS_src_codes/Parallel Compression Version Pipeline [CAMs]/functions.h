#ifndef FUNCTION_H
#define FUNCTION_H

/************************** Include Files ******************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/************************** Constant Definitions ******************************/
#define MAX_DICTIONARY_SIZE         4096           // Maximum size of the LZW dictionary (12-bit codes)
#define INVALID_CODE                0xFFFF         // Used to represent an invalid or non-existent code
#define NUMBERS_FUNCTIONS_PARALLEL  11             // Used to determine the number of functions implemented to run in parallel 

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
 *
 * @param dictionary Pointer to the dictionary.
 */
void init_dictionary(Dictionary *dictionary);

/**
 * @brief Finds the code for a given prefix + extension entry in the dictionary.
 *
 * @param prefix                The prefix code.
 * @param ext                   The extension byte.
 * @param dictionary_size       Current dicitionary size.
 * @param dictionary            Pointer to the dictionary.
 *
 * @return        The code for the sequence if found, INVALID_CODE otherwise.
 */
uint16_t Dictionary_find(uint16_t prefix, uint8_t ext, uint16_t dictionary_size, Dictionary *dictionary);

/**
 * @brief Adds a new prefix + extension to the dictionary.
 *
 * @param prefix          Prefix code.
 * @param ext             Extension byte.
 * @param dictionary_size Pointer to current dictionary size (updated internally).
 * @param bit_count       Pointer to current code bit width.
 * @param dictionary      Pointer to the dictionary.
 */
void Dictionary_add(uint16_t prefix, uint8_t ext, uint16_t *dictionary_size, uint8_t *bit_count, Dictionary *dictionary);

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

/**************************  Main Parallel Compression Function Declaration ******************************/
/**
 * @brief Top-level parallel LZW compression function for HLS.
 *        Splits the input buffer into three segments and compresses them in parallel.
 *
 * @param input                Pointer to input data buffer.
 * @param input_size           Size of the input buffer.
 * @param outputX              Pointer to the X output buffer.
 * @param compression_sizeX    Pointer to store compressed size of the X segment.
 */
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
);

#endif
