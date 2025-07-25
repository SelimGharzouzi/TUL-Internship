#ifndef FUNCTION_H
#define FUNCTION_H

#include <stdlib.h>
#include "ff.h"
#include "xil_cache.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <xstatus.h>

#define MAX_DICT_SIZE 4096
#define INVALID_CODE 0xFFFF
#define INVALID_SYMBOL 0xFF

// ------------------------------------------------------------------------------------
/*
 *                             Structure and global variables 
 */
// ------------------------------------------------------------------------------------

extern uint8_t data[100000];
extern size_t data_len;

typedef struct {
    uint16_t prefix_code;
    uint8_t ext_byte;
    uint16_t code;
} Dictionary;

// ------------------------------------------------------------------------------------
/*
 *                                      Functions
 */
// ------------------------------------------------------------------------------------

void Dictionary_init(void);
uint32_t hash(uint16_t prefix, uint8_t ext);
uint16_t Dictionary_find(uint16_t prefix, uint8_t ext);
void Dictionary_add(uint16_t prefix, uint8_t ext);
void write_to_bitstream(uint16_t code);
void compress();
void print_bitstream(void) ;
void Dictionary_print(void);
int WriteSD(void);
int ReadSD(void);




#endif