#include "functions.h"

// -------------------------------------------------------------------------------------
/*
 *                                   Global variables  
 */
// -------------------------------------------------------------------------------------

Dictionary dictionary[MAX_DICT_SIZE];
bool dictionary_used[MAX_DICT_SIZE];
uint16_t dict_size_actual = 0;

uint8_t bitstream[MAX_DICT_SIZE * 12] = {0};
size_t bitstream_index = 0;

int bit_count = 8;

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "test.txt";
static char foutput[32] = "inputd.bin";

uint8_t data[100000] = {0};
size_t data_len = 0;


// -------------------------------------------------------------------------------------
/*
 *                                      Functions
 */
// -------------------------------------------------------------------------------------

void Dictionary_init(void) {
    memset(dictionary_used, 0, sizeof(dictionary_used));
    for (uint16_t i = 0; i < 256 ; i++) {
        dictionary[i].code = i;
        dictionary[i].prefix_code = INVALID_CODE;
        dictionary[i].ext_byte = (uint8_t)i;
        dictionary_used[i] = true;
        dict_size_actual++;
    }
}

uint32_t hash(uint16_t prefix, uint8_t ext) {
    return ((prefix << 5) ^ ext) % MAX_DICT_SIZE;
}

uint16_t Dictionary_find(uint16_t prefix, uint8_t ext) {
    uint32_t h = hash(prefix, ext);
    for (uint32_t i = 0; i < MAX_DICT_SIZE; i++) {
        uint32_t idx = (h + i) % MAX_DICT_SIZE;
        if (dictionary[idx].prefix_code == prefix && dictionary[idx].ext_byte == ext)
            return dictionary[idx].code;
    }
    return INVALID_CODE;
}

void Dictionary_add(uint16_t prefix, uint8_t ext) {
    if (dict_size_actual >= MAX_DICT_SIZE) return;
    if(dict_size_actual >= (1u << bit_count) && bit_count< (int)log2(MAX_DICT_SIZE)) bit_count++;

    uint32_t h = hash(prefix, ext);
        for (uint32_t i = 0; i < MAX_DICT_SIZE; i++) {
        uint32_t idx = (h + i) % MAX_DICT_SIZE;
         if (!dictionary_used[idx]) {
            dictionary[idx].prefix_code = prefix;
            dictionary[idx].ext_byte = ext;
            dictionary[idx].code = dict_size_actual;
            dictionary_used[idx] = true;
            dict_size_actual++;
            return;
        }
    }
}

void write_to_bitstream(uint16_t code) {
    for (int i = (bit_count - 1); i >= 0; i--) {
        bool bit = (code >> i) & 1;
        bitstream[bitstream_index / 8] |= (bit << (7 - (bitstream_index % 8)));
        bitstream_index++;
    }
}

void compress() {
    if (data_len == 0) return;

    if (data_len == 1) {
        write_to_bitstream(data[0]);
        return;
    }

    uint16_t prefix = data[0];
    uint8_t ext;

    for (size_t i = 1; i < data_len; i++) {
        ext = data[i];
        uint16_t code = Dictionary_find(prefix, ext);
        if (code != INVALID_CODE) {
            prefix = code;
        } else {
            write_to_bitstream(prefix);
            Dictionary_add(prefix, ext);
            prefix = ext;
        }
    }
    write_to_bitstream(prefix);

    while (bitstream_index % 8 != 0) {
        bitstream[bitstream_index / 8] |= (0 << (7 - (bitstream_index % 8)));
        bitstream_index++;
    }
}

void print_bitstream(void) {
    printf("Compressed Bitstream:\n");
    for (size_t i = 0; i < bitstream_index; i++) {
        printf("%d", (bitstream[i / 8] >> (7 - (i % 8))) & 1);
    }
    printf("\n");
}

void Dictionary_print(void) {
    printf("\nCompression Dictionary:\n");

    for (uint32_t i = 0; i < MAX_DICT_SIZE; i++) {
        if (dictionary_used[i] && dictionary[i].code >= 256) {
            printf("Code: %u, Prefix: %u, Ext: %u\n",
                   dictionary[i].code,
                   dictionary[i].prefix_code,
                   dictionary[i].ext_byte);
        }
    }
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
        printf("Open failed\n");
        return XST_FAILURE;
    }

    size_t byte_len = (bitstream_index + 7) / 8;
    
    Res = f_write(&fil, bitstream, byte_len, &NumBytesWritten);
    if(Res != FR_OK || NumBytesWritten != byte_len){
        printf("Write failed\n");
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil); 
    return XST_SUCCESS;
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
        return XST_FAILURE;
    }

    Res = f_read(&fil, data, sizeof(data), &NumBytesRead);
    if (Res != FR_OK){
        printf("Read failed\n");
        printf("Res = %d\n", Res);
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);

    data_len = NumBytesRead - 1;

    printf("%zu bytes read from the file\n", data_len);

    return XST_SUCCESS;
}

