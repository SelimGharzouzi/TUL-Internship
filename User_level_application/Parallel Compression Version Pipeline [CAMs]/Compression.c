#include "xtop_parallel_lzw.h"
#include "xparameters.h"
#include "xil_cache.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <xil_printf.h>
#include <xil_types.h>
#include "xscutimer.h"
#include <stdbool.h>
#include <xstatus.h>
#include "ff.h"

#define NUMBERS_FUNCTIONS_PARALLEL 11
#define FILE_INPUT_SIZE 4*1024*1024

static uint8_t input[FILE_INPUT_SIZE];
uint8_t outputs[NUMBERS_FUNCTIONS_PARALLEL][2 * FILE_INPUT_SIZE] = {{0}};

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "input.txt";

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
    XTop_parallel_lzw compressor;
    uint64_t start, end;
    int status;
    int input_length = 0;

    printf("\n-------------------------------------- Test 2 --------------------------------------\n");

    status = ReadSD(input, &input_length);
    if (status != XST_SUCCESS) {
        printf("Failed to read sd card, %d\r\n", status);
        return status;
    }

    Xil_DCacheFlushRange((UINTPTR)input, input_length);
    for (int i = 0; i< NUMBERS_FUNCTIONS_PARALLEL; i++) {
        Xil_DCacheFlushRange((UINTPTR)outputs[i], input_length * 2);
    }


    status = XTop_parallel_lzw_Initialize(&compressor, XPAR_TOP_PARALLEL_LZW_0_BASEADDR);

    XTop_parallel_lzw_Set_input_r(&compressor,  (UINTPTR)input);
    XTop_parallel_lzw_Set_output1(&compressor,  (UINTPTR)outputs[0]);
    XTop_parallel_lzw_Set_output2(&compressor,  (UINTPTR)outputs[1]);
    XTop_parallel_lzw_Set_output3(&compressor,  (UINTPTR)outputs[2]);
    XTop_parallel_lzw_Set_output4(&compressor,  (UINTPTR)outputs[3]);
    XTop_parallel_lzw_Set_output5(&compressor,  (UINTPTR)outputs[4]);
    XTop_parallel_lzw_Set_output6(&compressor,  (UINTPTR)outputs[5]);
    XTop_parallel_lzw_Set_output7(&compressor,  (UINTPTR)outputs[6]);
    XTop_parallel_lzw_Set_output8(&compressor,  (UINTPTR)outputs[7]);
    XTop_parallel_lzw_Set_output9(&compressor,  (UINTPTR)outputs[8]);
    XTop_parallel_lzw_Set_output10(&compressor,  (UINTPTR)outputs[9]);
    XTop_parallel_lzw_Set_output11(&compressor,  (UINTPTR)outputs[10]);

    XTop_parallel_lzw_Set_input_size(&compressor, input_length);

    start = get_global_time();

    XTop_parallel_lzw_Start(&compressor);
    while(!XTop_parallel_lzw_IsDone(&compressor));

    end = get_global_time();

    uint64_t elapsed_cycles = end - start;
    double elapsed_time_sec = (double)elapsed_cycles/ XPAR_CPU_CORE_CLOCK_FREQ_HZ;

    printf("Parallel compression time : %.6f seconds\r\n", elapsed_time_sec);

    uint32_t compression_sizes[NUMBERS_FUNCTIONS_PARALLEL] = {0};

    compression_sizes[0] = XTop_parallel_lzw_Get_compression_size1(&compressor);
    compression_sizes[1] = XTop_parallel_lzw_Get_compression_size2(&compressor);
    compression_sizes[2] = XTop_parallel_lzw_Get_compression_size3(&compressor);
    compression_sizes[3] = XTop_parallel_lzw_Get_compression_size4(&compressor);
    compression_sizes[4] = XTop_parallel_lzw_Get_compression_size5(&compressor);
    compression_sizes[5] = XTop_parallel_lzw_Get_compression_size6(&compressor);
    compression_sizes[6] = XTop_parallel_lzw_Get_compression_size7(&compressor);
    compression_sizes[7] = XTop_parallel_lzw_Get_compression_size8(&compressor);
    compression_sizes[8] = XTop_parallel_lzw_Get_compression_size9(&compressor);
    compression_sizes[9] = XTop_parallel_lzw_Get_compression_size10(&compressor);
    compression_sizes[10] = XTop_parallel_lzw_Get_compression_size11(&compressor);

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        Xil_DCacheInvalidateRange((UINTPTR)outputs[i], compression_sizes[i]);
    }

    uint64_t total_compression_size = 0;

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        printf("Compression size of output number %d is : %lu\n", i+1, (unsigned long)compression_sizes[i]);
        total_compression_size += compression_sizes[i];
    }

    printf("Total compression size = %lu\n", (unsigned long)total_compression_size);
    printf("Compression ratio: %.2f%%\n", 100.0 * (double)total_compression_size / input_length);

    return 0;
}