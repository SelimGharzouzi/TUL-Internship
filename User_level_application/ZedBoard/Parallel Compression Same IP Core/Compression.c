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

#define NUMBERS_FUNCTIONS_PARALLEL 10
#define FILE_INPUT_SIZE 4*1024*1024
#define COUNTER_CLK_FREQ_HZ XPAR_CPU_CORE_CLOCK_FREQ_HZ/2

static uint8_t input[FILE_INPUT_SIZE];
uint8_t outputs[NUMBERS_FUNCTIONS_PARALLEL][2 * (FILE_INPUT_SIZE / NUMBERS_FUNCTIONS_PARALLEL)] = {{0}};

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "input.txt";
static char foutput[32] = "output.txt";

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

int WriteSD(uint8_t outputs[NUMBERS_FUNCTIONS_PARALLEL][2 * (FILE_INPUT_SIZE / NUMBERS_FUNCTIONS_PARALLEL)], uint32_t compression_sizes[NUMBERS_FUNCTIONS_PARALLEL]) {
    FRESULT Res;
    UINT NumBytesWritten;
    UINT TotalNumBytesWritten = 0;

    Res = f_mount(&fatfs, Path, 0);
    if (Res != FR_OK){
        printf("Mount failed, error code %d\n", Res);
        return XST_FAILURE;
    }

    Res = f_open(&fil, foutput, FA_CREATE_ALWAYS | FA_WRITE);
    if (Res != FR_OK){
        printf("Open failed, error code %d\n", Res);
        return XST_FAILURE;
    }
    
    char header[128] = {0};
    UINT header_len = 0;
    header_len += snprintf(header + header_len, sizeof(header) - header_len, "%d", NUMBERS_FUNCTIONS_PARALLEL);
    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        header_len += snprintf(header + header_len, sizeof(header) - header_len, " %u", compression_sizes[i]);
    }
    header_len += snprintf(header + header_len, sizeof(header) - header_len, "\n");

    Res = f_write(&fil, header, header_len, &NumBytesWritten);
    if (Res != FR_OK || NumBytesWritten != header_len) {
        printf("Header write failed, error code %d\n", Res);
        f_close(&fil);
        return XST_FAILURE;
    }

    TotalNumBytesWritten += NumBytesWritten;

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        Res = f_write(&fil, outputs[i], compression_sizes[i], &NumBytesWritten);
        if (Res != FR_OK || NumBytesWritten != compression_sizes[i]) {
            printf("Data write failed at core %d\n", i);
            f_close(&fil);
            return XST_FAILURE;
        }
        TotalNumBytesWritten += NumBytesWritten;
    }

    printf("Wrote %d bytes to the SD card\n", TotalNumBytesWritten);

    f_close(&fil); 
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

    printf("\n-------------------------------------- Test 1 - 10 Functions - 200MHz --------------------------------------\n");

    status = ReadSD(input, &input_length);
    if (status != XST_SUCCESS) {
        printf("Failed to read sd card, %d\r\n", status);
        return status;
    }

    int part_size = input_length / NUMBERS_FUNCTIONS_PARALLEL;
    int remainder = input_length % NUMBERS_FUNCTIONS_PARALLEL;
    int sizes[NUMBERS_FUNCTIONS_PARALLEL];
    int offsets[NUMBERS_FUNCTIONS_PARALLEL];

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        sizes[i] = part_size + (i < remainder ? 1 : 0);
        offsets[i] = (i == 0) ? 0 : offsets[i-1] + sizes[i-1];
    }

    for (int i = 0; i< NUMBERS_FUNCTIONS_PARALLEL; i++) {
        Xil_DCacheFlushRange((UINTPTR)input + offsets[i], sizes[i]);
        Xil_DCacheFlushRange((UINTPTR)outputs[i], input_length * 2);
    }

    status = XTop_parallel_lzw_Initialize(&compressor, XPAR_TOP_PARALLEL_LZW_0_BASEADDR);
    if (status != XST_SUCCESS) {
        printf("Failed to initiliaze IP core, error code %d\n", status);
        return 1;
    }

    XTop_parallel_lzw_Set_input1(&compressor, (UINTPTR)input + offsets[0]);
    XTop_parallel_lzw_Set_input2(&compressor, (UINTPTR)input + offsets[1]);
    XTop_parallel_lzw_Set_input3(&compressor, (UINTPTR)input + offsets[2]);
    XTop_parallel_lzw_Set_input4(&compressor, (UINTPTR)input + offsets[3]);
    XTop_parallel_lzw_Set_input5(&compressor, (UINTPTR)input + offsets[4]);
    XTop_parallel_lzw_Set_input6(&compressor, (UINTPTR)input + offsets[5]);
    XTop_parallel_lzw_Set_input7(&compressor, (UINTPTR)input + offsets[6]);
    XTop_parallel_lzw_Set_input8(&compressor, (UINTPTR)input + offsets[7]);
    XTop_parallel_lzw_Set_input9(&compressor, (UINTPTR)input + offsets[8]);
    XTop_parallel_lzw_Set_input10(&compressor, (UINTPTR)input + offsets[9]);

    XTop_parallel_lzw_Set_input_size1(&compressor, (UINTPTR)sizes[0]);
    XTop_parallel_lzw_Set_input_size2(&compressor, (UINTPTR)sizes[1]);
    XTop_parallel_lzw_Set_input_size3(&compressor, (UINTPTR)sizes[2]);
    XTop_parallel_lzw_Set_input_size4(&compressor, (UINTPTR)sizes[3]);
    XTop_parallel_lzw_Set_input_size5(&compressor, (UINTPTR)sizes[4]);
    XTop_parallel_lzw_Set_input_size6(&compressor, (UINTPTR)sizes[5]);
    XTop_parallel_lzw_Set_input_size7(&compressor, (UINTPTR)sizes[6]);
    XTop_parallel_lzw_Set_input_size8(&compressor, (UINTPTR)sizes[7]);
    XTop_parallel_lzw_Set_input_size9(&compressor, (UINTPTR)sizes[8]);
    XTop_parallel_lzw_Set_input_size10(&compressor, (UINTPTR)sizes[9]);

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

    start = get_global_time();

    XTop_parallel_lzw_Start(&compressor);
    while(!XTop_parallel_lzw_IsDone(&compressor));

    end = get_global_time();

    uint64_t elapsed_cycles = end - start;
    double elapsed_time_sec = (double)elapsed_cycles/COUNTER_CLK_FREQ_HZ;

    printf("Parallel compression time : %.6f seconds\r\n", elapsed_time_sec);

    uint32_t compression_sizes[NUMBERS_FUNCTIONS_PARALLEL] = {0};
    uint64_t total_compression_size = 0;

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

    for (int i = 0; i < NUMBERS_FUNCTIONS_PARALLEL; i++) {
        Xil_DCacheInvalidateRange((UINTPTR)outputs[i], compression_sizes[i]);
        printf("Compression size of output number %d is : %lu\n", i+1, (unsigned long)compression_sizes[i]);
        total_compression_size += compression_sizes[i];
    }

    printf("Total compression size = %lu\n", (unsigned long)total_compression_size);
    printf("Compression ratio: %.2f%%\n", 100.0 * (double)total_compression_size / input_length);

    status = WriteSD(outputs, compression_sizes);
    if (status != XST_SUCCESS){
        printf("WriteSD failed, error code %d\n", status);
    }

    return 0;

}
