#include "function.h"

// -------------------------------------------------------------------------------------
/*
 *                              Global variables for Compression
 */
// -------------------------------------------------------------------------------------

DictionaryCompression dictionary_Compression[MAX_DICT_SIZE];
uint16_t dict_size_actual_Compression = 0;

uint8_t bitstream[MAX_DICT_SIZE * 12] = {0};
size_t bitstream_index_compression = 0;

int bit_count_Compression = 8;

// -------------------------------------------------------------------------------------
/*
 *                              Global variables for Decompression
 */
// -------------------------------------------------------------------------------------

DictionaryDecompression dictionary_Decompression[MAX_DICT_SIZE];
uint16_t dict_size_actual_Decompression = 0;

uint8_t output[MAX_DICT_SIZE * 12] = {0};
size_t output_index = 0;
size_t bitstream_index_decompression = 0;

int bit_count_Decompression = 8;


// -------------------------------------------------------------------------------------
/*
 *                              Functions for Compression
 */
// -------------------------------------------------------------------------------------

void Dictionary_init_Compression() {
    for (uint16_t i = 0; i < 256 ; i++) {
        dictionary_Compression[i].code = i;
        dictionary_Compression[i].prefix_code = i;
        dictionary_Compression[i].ext_byte = INVALID_CODE1;
        dict_size_actual_Compression++;
    }
}

uint32_t hash(uint16_t prefix, uint8_t ext) {
    return ((prefix << 5) ^ ext) % MAX_DICT_SIZE;
}

uint16_t Dictionary_find_compression(uint16_t prefix, uint8_t ext) {
    uint32_t h = hash(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICT_SIZE; i++) {
        uint32_t idx = (h + i) % MAX_DICT_SIZE;
        if (dictionary_Compression[idx].prefix_code == prefix && dictionary_Compression[idx].ext_byte == ext)
            return dictionary_Compression[idx].code;
    }
    return INVALID_CODE;
}

void Dictionary_add_compression(uint16_t prefix, uint8_t ext) {
    if(dict_size_actual_Compression>= MAX_DICT_SIZE) return;

    if(dict_size_actual_Compression >= (1 << bit_count_Compression) && bit_count_Compression < log2(MAX_DICT_SIZE)) {
        bit_count_Compression++;
    }
    dictionary_Compression[dict_size_actual_Compression].prefix_code = prefix;
    dictionary_Compression[dict_size_actual_Compression].ext_byte = ext;
    dictionary_Compression[dict_size_actual_Compression].code = dict_size_actual_Compression;
    dict_size_actual_Compression++;
}

void write_to_bitstream_compression(uint16_t code) {
    for (int i = (bit_count_Compression - 1); i >= 0; i--) {
        bool bit = (code >> i) & 1;
        bitstream[bitstream_index_compression / 8] |= (bit << (7 - (bitstream_index_compression % 8)));
        bitstream_index_compression++;
    }
}

void compress(const uint8_t *input, size_t input_len) {
    uint16_t prefix = input[0];
    uint8_t ext;

    for (size_t i = 1; i < input_len; i++) {
        ext = input[i];
        uint16_t code = Dictionary_find_compression(prefix, ext);
        if (code != INVALID_CODE) {
            prefix = code;
        } else {
            write_to_bitstream_compression(prefix);
            Dictionary_add_compression(prefix, ext);
            prefix = ext;
        }
    }
    write_to_bitstream_compression(prefix);

    while (bitstream_index_compression % 8 != 0) {
        bitstream[bitstream_index_compression / 8] |= (0 << (7 - (bitstream_index_compression % 8)));
        bitstream_index_compression++;
    }
}

void print_bitstream_compression() {
    printf("Compressed Bitstream:\n");
    for (size_t i = 0; i < bitstream_index_compression; i++) {
        printf("%d", (bitstream[i / 8] >> (7 - (i % 8))) & 1);
    }
    printf("\n");
}

void print_bitsteam_file_compression(FILE *foutput) {
    for (size_t i = 0; i < bitstream_index_compression; i++) {
        fprintf(foutput, "%d", (bitstream[i / 8] >> (7 - (i % 8))) & 1);
    }
}

void Dictionary_print_compression() {
    printf("\nCompression Dictionary:\n");
    for (uint16_t i = 256; i < dict_size_actual_Compression; i++) {
        printf("Code: %u, Prefix: %u, Ext: %u\n", dictionary_Compression[i].code, dictionary_Compression[i].prefix_code,
               dictionary_Compression[i].ext_byte);
    }
}

// -------------------------------------------------------------------------------------
/*
 *                              Functions for Decompression
 */
// -------------------------------------------------------------------------------------

uint16_t Binary(const int bit_count, const uint16_t *bin) {
    uint16_t result = 0;
    for (int i = 0; i < bit_count; i++) {
        result <<= 1;
        if (bin[i] == '1') {
            result |= 1;
        }
    }
    return result;
}

void toBinary8_Decompression(uint8_t value, FILE *foutput) {
    char binary[9];
    binary[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        binary[i] = (value & 1) ? '1' : '0';
        value >>= 1;
    }
    fwrite(binary, sizeof(char), 8, foutput);
}

void Dictionary_init_Decompression() {
    for (uint16_t i = 0; i < 256; i++) {
        dictionary_Decompression[i].code = i;
        dictionary_Decompression[i].sequence[0] = (uint8_t)i;
        dictionary_Decompression[i].length = 1;
        dict_size_actual_Decompression++;
    }
}

DictionaryDecompression* Dictionary_find_Decompression(uint16_t code) {
    for (uint16_t i = 0; i < dict_size_actual_Decompression; i++) {
        if (dictionary_Decompression[i].code == code) {
            return &dictionary_Decompression[i];
        }
    }
    return NULL;
}

void Dictionary_add_Decompression(const uint8_t *sequence, uint16_t seq_len) {
    if (seq_len > MAX_SEQUENCE_LENGTH || dict_size_actual_Decompression >= MAX_DICT_SIZE) {
        return;
    }

    if (dict_size_actual_Decompression >= (1 << bit_count_Decompression) && bit_count_Decompression < log2(MAX_DICT_SIZE)) {
        bit_count_Decompression++;
    }

    dictionary_Decompression[dict_size_actual_Decompression].code = dict_size_actual_Decompression;
    memcpy(dictionary_Decompression[dict_size_actual_Decompression].sequence, sequence, seq_len);
    dictionary_Decompression[dict_size_actual_Decompression].length = seq_len;
    dict_size_actual_Decompression++;
}

void decompress(const uint8_t *inputb, size_t len, FILE *foutput) {
    uint16_t input[100000];
    int inputIndex = 0;
    int inputbIndex = 0;
    uint8_t prev_sequence[MAX_SEQUENCE_LENGTH] = {0};
    uint16_t prev_length = 0;
    uint8_t new_sequence[MAX_SEQUENCE_LENGTH] = {0};
    uint16_t chunk[MAX_SEQUENCE_LENGTH];
    int chunkIndex = 0;
    int current_bit_count = bit_count_Decompression;
    bool is_first = true;

    while (len > 0) {
        len -= current_bit_count;

        for (size_t j = 0; j < current_bit_count; j++) {
            if (inputb[j + inputbIndex] == '0' || inputb[j + inputbIndex] == '1') {
                chunk[chunkIndex++] = inputb[j + inputbIndex];
                if (chunkIndex == current_bit_count) {
                    chunk[current_bit_count] = '\0';
                    input[inputIndex++] = Binary(current_bit_count, chunk);
                    chunkIndex = 0;
                }
                } else {
                    return;
                }
        }

        if (chunkIndex != 0) {
            while (chunkIndex < current_bit_count) {
                chunk[chunkIndex++] = '0';
            }
            chunk[current_bit_count] = '\0';
            input[inputIndex++] = Binary(current_bit_count, chunk);
        }

        uint16_t curr_code = input[inputIndex - 1];
        DictionaryDecompression *entry = Dictionary_find_Decompression(curr_code);
        const uint8_t *curr_sequence;
        uint16_t curr_length;

        if (entry != NULL) {
            curr_sequence = entry->sequence;
            curr_length = entry->length;
        } else {
            // KwKwK case
            memcpy(new_sequence, prev_sequence, prev_length);
            new_sequence[prev_length] = prev_sequence[0];
            curr_sequence = new_sequence;
            curr_length = prev_length + 1;
        }

        for (uint16_t j = 0; j < curr_length; j++) {
            toBinary8_Decompression(curr_sequence[j], foutput);
        }

        if ((inputIndex - 1) > 0) {
            memcpy(new_sequence, prev_sequence, prev_length);
            new_sequence[prev_length] = curr_sequence[0];
            Dictionary_add_Decompression(new_sequence, prev_length + 1);
        }

        memcpy(prev_sequence, curr_sequence, curr_length);
        prev_length = curr_length;
        inputbIndex += current_bit_count;

        if (is_first) {
            current_bit_count++;
            is_first = false;
        }
    }
}

void Dictionary_print_Decompression() {
    printf("\nDecompression Dictionary:\n");
    for (uint16_t i = 256; i < dict_size_actual_Decompression; i++) {
        printf("Code: %u, Sequence: ", dictionary_Decompression[i].code);
        for (uint16_t j = 0; j < dictionary_Decompression[i].length; j++) {
            printf("%u ", dictionary_Decompression[i].sequence[j]);
        }
        printf("\n");
    }
}


// -------------------------------------------------------------------------------------
/*
 *                                  Functions for main
 */
// -------------------------------------------------------------------------------------

uint16_t read_from_bitstream() {
    uint16_t code = 0;
    for (int i = 0; i < bit_count_Decompression; i++) {
        bool bit = (bitstream[bitstream_index_decompression / 8] >> (7 - (bitstream_index_decompression % 8))) & 1;
        code = (code << 1) | bit;
        (bitstream_index_decompression)++;
    }
    return code;
}

void write_to_bitstream_decompression(uint8_t sequence) {
    for (int i = 7; i >= 0; i--) {
        bool bit = (sequence >> i) & 1;
        output[output_index / 8] |= (bit << (7 - (output_index % 8)));
        output_index++;
    }
}

void decompress1() {
    uint8_t prev_sequence[MAX_SEQUENCE_LENGTH] = {0};
    uint16_t prev_length = 0;
    uint8_t new_sequence[MAX_SEQUENCE_LENGTH] = {0};
    size_t bitstream_size_in_bits = bitstream_index_compression;
    bool is_first = true;

    do {
        bitstream_size_in_bits -= bit_count_Decompression;
        uint16_t curr_code = read_from_bitstream();
        DictionaryDecompression *entry = Dictionary_find_Decompression(curr_code);
        const uint8_t *curr_sequence;
        uint16_t curr_length;

        if (entry != NULL) {
            curr_sequence = entry->sequence;
            curr_length = entry->length;
        } else {
            // KwKwK case
            memcpy(new_sequence, prev_sequence, prev_length);
            new_sequence[prev_length] = prev_sequence[0];
            curr_sequence = new_sequence;
            curr_length = prev_length + 1;
        }

        for (uint16_t j = 0; j < curr_length; j++) {
            write_to_bitstream_decompression(curr_sequence[j]);
        }

        if (!is_first) {
            memcpy(new_sequence, prev_sequence, prev_length);
            new_sequence[prev_length] = curr_sequence[0];
            Dictionary_add_Decompression(new_sequence, prev_length + 1);
        }

        memcpy(prev_sequence, curr_sequence, curr_length);
        prev_length = curr_length;

        if (is_first){
            bit_count_Decompression++;
            is_first = false;
        }
    } while (bitstream_size_in_bits > bit_count_Decompression);
}

void print_bitstream_decompression() {
    printf("Decompressed Bitstream:\n");
    for (size_t i = 0; i < output_index; i++) {
        printf("%d", (output[i / 8] >> (7 - (i % 8))) & 1);
    }
    printf("\n");
}