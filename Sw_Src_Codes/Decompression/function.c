#include "function.h"
#include <stddef.h>
#include <stdio.h>
#include <xil_printf.h>

// -------------------------------------------------------------------------------------
/*
 *                                      Global variables
 */
// -------------------------------------------------------------------------------------

Dictionary dictionary[MAX_DICT_SIZE];
uint16_t dict_size_actual = 0;

uint8_t output[MAX_DICT_SIZE * 12] = {0};
size_t output_index = 0;
size_t bit_count = 8;

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "inputd.bin";
static char foutput[32] = "output.txt";

uint8_t data[100000] = {0};
size_t data_len = 0;

size_t bit_position = 0;
size_t byte_position = 0;

// -------------------------------------------------------------------------------------
/*
 *                                      Functions
 */
// -------------------------------------------------------------------------------------

void Dictionary_init(void) {
    dict_size_actual = 0;
    for (uint16_t i = 0; i < 256; i++) {
        dictionary[i].code = i;
        dictionary[i].sequence[0] = (uint8_t)i;
        dictionary[i].length = 1;
        dict_size_actual++;
    }
}

Dictionary* Dictionary_find(uint16_t code) {
    for (uint16_t i = 0; i < dict_size_actual; i++) {
        if (dictionary[i].code == code) {
            return &dictionary[i];
        }
    }
    return NULL;
}

void Dictionary_add(const uint8_t *sequence, uint16_t seq_len) {
    if(dict_size_actual >= MAX_DICT_SIZE) return;
    if (seq_len > MAX_SEQUENCE_LENGTH) return;

    dictionary[dict_size_actual].code = dict_size_actual;
    memcpy(dictionary[dict_size_actual].sequence, sequence, seq_len);
    dictionary[dict_size_actual].length = seq_len;
    dict_size_actual++;
}

int ReadSD(void){
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
        printf("Res = %d\n", Res);
        return XST_FAILURE;
    }
    
    Res = f_read(&fil, data, sizeof(data), &NumBytesRead);
    if (Res != FR_OK){
        printf("Read failed\n");
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);
    data_len = NumBytesRead;

    return XST_SUCCESS;
}

int WriteSD(void){
    FRESULT Res;
    UINT NumBytesWritten;

    Res = f_mount(&fatfs, Path, 0);
    if (Res != FR_OK){
        printf("Mount failed\n");
        return XST_FAILURE;
    }

    Res = f_open(&fil, foutput, FA_CREATE_ALWAYS | FA_WRITE);
    if (Res != FR_OK){
        printf("Open failed: %d\n", Res);
        return XST_FAILURE;
    }

    Res = f_write(&fil, output, output_index, &NumBytesWritten);
    if (Res != FR_OK || NumBytesWritten != output_index) {
        printf("Write failed: %d\n", Res);
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);

    return XST_SUCCESS; 
}


uint16_t ReadCode(void) {
    uint16_t code = 0;

    for (size_t i = 0; i < bit_count; i++) {
        uint8_t bit = (data[byte_position] >> (7 - bit_position)) & 1;
        code = (code << 1) | bit;

        bit_position++;
        if (bit_position == 8) {
            bit_position = 0;
            byte_position++;
        }
    }

    return code;
}

void decompress(void){
    uint8_t prev_sequence[MAX_SEQUENCE_LENGTH] = {0};
    uint16_t prev_length = 0;
    uint8_t new_sequence[MAX_SEQUENCE_LENGTH] = {0};
    size_t len = data_len*8;

    uint16_t first_code = ReadCode();
    Dictionary *entry = Dictionary_find(first_code);

    if (entry != NULL) {
        memcpy(prev_sequence, entry->sequence, entry->length);
        prev_length = entry->length;
        memcpy(output + output_index, prev_sequence, prev_length);
        output_index += prev_length;
    }
    bit_count++;


    while(len > bit_count  && len <= data_len*8){

        if (dict_size_actual >= (1 << bit_count) && bit_count < 12) 
            bit_count++;

        uint16_t curr_code = ReadCode();
        Dictionary *entry = Dictionary_find(curr_code);
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

        memcpy(output + output_index, curr_sequence, curr_length);
        output_index += curr_length;


        if (prev_length + 1 <= MAX_SEQUENCE_LENGTH) {
            memcpy(new_sequence, prev_sequence, prev_length);
            new_sequence[prev_length] = curr_sequence[0];
            Dictionary_add(new_sequence, prev_length + 1);
        }

        memcpy(prev_sequence, curr_sequence, curr_length);
        prev_length = curr_length;
        len -= bit_count;        
    }
    if (output[output_index-1] == 0){
        output_index--;
    }
}

void Dictionary_print(void) {
    printf("\nDecompression Dictionary:\n");
    for (uint16_t i = 256; i < dict_size_actual; i++) {
        printf("Code: %u, Sequence: ", dictionary[i].code);
        for (uint16_t j = 0; j < dictionary[i].length; j++) {
            printf("%u ", dictionary[i].sequence[j]);
        }
        printf("\n");
    }
}





