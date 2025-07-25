#ifndef FUNCTION_H
#define FUNCTION_H

#include "ff.h"
#include <stdio.h>

#define MAX_SEQUENCE_LENGTH 256
#define MAX_DICT_SIZE 4096
#define INVALID_CODE 0xFFFF
#define INVALID_SYMBOL 0xFF


// ------------------------------------------------------------------------------------
/*
 *                               Structure and global variables
 */
// ------------------------------------------------------------------------------------

extern uint8_t data[100000];
extern size_t data_len;
extern size_t bit_count;


typedef struct {
    uint16_t code;
    uint8_t sequence[MAX_SEQUENCE_LENGTH];
    uint16_t length;
} Dictionary;

// ------------------------------------------------------------------------------------
/*
 *                                        Functions
 */
// ------------------------------------------------------------------------------------

void Dictionary_init();
Dictionary* Dictionary_find(uint16_t code);
void Dictionary_add(const uint8_t *sequence, uint16_t seq_len);
int ReadSD(void);
uint16_t ReadCode(void);
void decompress(void);
int WriteSD(void);
void Dictionary_print(void);

#endif