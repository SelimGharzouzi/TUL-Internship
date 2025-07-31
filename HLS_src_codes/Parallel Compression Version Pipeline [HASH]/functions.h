#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/************************** Include Files ******************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>

/************************** Constant Definitions ******************************/
#define MAX_DICTIONARY_SIZE         4096           // Maximum size of the LZW dictionary (12-bit codes)
#define INVALID_CODE                0xFFFF         // Used to represent an invalid or non-existent code
#define NUMBER_PARALLEL_FUNCTIONS   2              // Used to determine the number of functions implemented to run in parallel

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
 * @param dictionary        Pointer to the dictionary.
 * @param dictionary_used   Pointer to a table that indicated if an entry with a certain index is used or not.
 */
void init_dictionary(Dictionary *dictionary, bool *dictionary_used);

/**
 * @brief Reset the dictionary to its initial state.
 *
 * @param dictionary        Pointer to the dictionary.
 * @param dictionary_used   Pointer to a table that indicated if an entry with a certain index is used or not.
 * @param dictionary_size   Pointer to current dictionary size.
 * @param bit_count         Pointer to current code bit width.
 */
void Dictionary_reset(Dictionary *dictionary, bool *dictionary_used, uint16_t *dictionary_size, uint8_t *bit_count);

/**
 * @brief Computes hash functions for dictionary indexing.
 *
 * @param prefix        The prefix code.
 * @param ext           The extension byte.
 *
 * @return Hash value for dictionary indexing.
 */
uint32_t hash1(uint16_t prefix, uint8_t ext);
uint32_t hash2(uint16_t prefix, uint8_t ext);

/**
 * @brief Finds the code for a given prefix + extension entry in the dictionary.
 *
 * @param dictionary            Pointer to the dictionary.
 * @param dictionary_used       Pointer to a table that indicated if an entry with a certain index is used or not.
 * @param prefix                The prefix code.
 * @param ext                   The extension byte.
 *
 * @return The code for the sequence if found, INVALID_CODE otherwise.
 */
uint16_t Dictionary_find(Dictionary *dictionary, bool *dictionary_used, uint16_t prefix, uint8_t ext);

/**
 * @brief Adds a new prefix + extension to the dictionary.
 *
 * @param dictionary      Pointer to the dictionary.
 * @param dictionary_used Pointer to a table that indicated if an entry with a certain index is used or not.
 * @param prefix          Prefix code.
 * @param ext             Extension byte.
 * @param dictionary_size Pointer to current dictionary size (updated internally).
 * @param bit_count       Pointer to current code bit width.
 */
void Dictionary_add(
    Dictionary *dictionary, bool *dictionary_used,
    uint16_t prefix, uint8_t ext,
    uint16_t *dictionary_size, uint8_t *bit_count
);

/**
 * @brief Writes a code into the output stream using a specific bit width.
 *
 * @param code           Code to write.
 * @param output         Output stream (FIFO).
 * @param bit_count      Current bit width for codes.
 * @param out_index      Reference to current output bit index.
 * @param pending_byte   Reference to byte buffer that stores bits before flushing.
 * @param pending_bits   Reference to current number of bits in pending_byte.
 */
void write_output(uint16_t code, uint8_t *output, uint8_t bit_count, uint32_t *out_index);

/************************** Main Function Declaration ******************************/
/**
 * @brief LZW compression function for HLS using streaming FIFO interfaces.
 *        Compresses input data from a stream and writes compressed output to another stream.
 *
 * @param input        Input stream of data bytes.
 * @param output       Output stream for compressed (bit-packed) data.
 * @param input_size   Input data size.
 * @param output_size  Reference to store compressed data size (in bytes).
 */
void lzw_compress(uint8_t *input, uint8_t *output, int input_size, uint32_t *compression_size);

/**************************  Main Parallel Compression Function Declaration ******************************/
/**
 * @brief Top-level parallel LZW compression function for HLS.
 *        Splits the input buffer into three segments and compresses them in parallel.
 *
 * @param inputX                Pointer to X input data buffer.
 * @param input_sizeX           Size of the X input buffer.
 * @param outputX               Pointer to the X output buffer.
 * @param compression_sizeX     Pointer to store compressed size of the X segment.
 */
void top_parallel_lzw(
    uint8_t* input1, int input_size1,
    uint8_t* input2, int input_size2,
    uint8_t* output1, uint32_t* compression_size1,
    uint8_t* output2, uint32_t* compression_size2
);


#endif