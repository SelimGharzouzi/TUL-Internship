#include "xlzw_compress.h"
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

#define NUMBER_OF_CORES 12
#define FILE_INPUT_SIZE 4*1024*1024
#define COUNTER_CLK_FREQ_HZ XPAR_CPU_CORE_CLOCK_FREQ_HZ/2

static uint8_t input[FILE_INPUT_SIZE];
uint8_t outputs[NUMBER_OF_CORES][2 * (FILE_INPUT_SIZE / NUMBER_OF_CORES)] = {{0}};

UINTPTR base_addrs[NUMBER_OF_CORES] = {
    XPAR_LZW_COMPRESS_0_BASEADDR,
    XPAR_LZW_COMPRESS_1_BASEADDR,
    XPAR_LZW_COMPRESS_2_BASEADDR,
    XPAR_LZW_COMPRESS_3_BASEADDR,
    XPAR_LZW_COMPRESS_4_BASEADDR,
    XPAR_LZW_COMPRESS_5_BASEADDR,
    XPAR_LZW_COMPRESS_6_BASEADDR,
    XPAR_LZW_COMPRESS_7_BASEADDR,
    XPAR_LZW_COMPRESS_8_BASEADDR,
    XPAR_LZW_COMPRESS_9_BASEADDR,
    XPAR_LZW_COMPRESS_10_BASEADDR,
    XPAR_LZW_COMPRESS_11_BASEADDR
};

FIL fil;
FATFS fatfs;
static const TCHAR *Path = "0:";
static char finput[32] = "input.txt";
static char foutput[32] = "output.bin";

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

int WriteSD(uint8_t outputs[NUMBER_OF_CORES][2 * (FILE_INPUT_SIZE / NUMBER_OF_CORES)], uint32_t compression_sizes[NUMBER_OF_CORES]) {
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
    header_len += snprintf(header + header_len, sizeof(header) - header_len, "%d", NUMBER_OF_CORES);
    for (int i = 0; i < NUMBER_OF_CORES; i++) {
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

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
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

void print_decimal(const uint8_t* data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        printf("%u ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main() {
    XLzw_compress compressors[NUMBER_OF_CORES];

    uint64_t start, end;
    int status;
    int input_length = 0;

    printf("\n-------------------------------------- Test 1 - 200 MHz - 12 IPs --------------------------------------\n");

    status = ReadSD(input, &input_length);
    if (status != XST_SUCCESS) {
        printf("Failed to read sd card, %d\r\n", status);
    }

    int part_size = input_length / NUMBER_OF_CORES;
    int remainder = input_length % NUMBER_OF_CORES;
    int sizes[NUMBER_OF_CORES];
    int offsets[NUMBER_OF_CORES];

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
        sizes[i] = part_size + (i < remainder ? 1 : 0);
        offsets[i] = (i == 0) ? 0 : offsets[i-1] + sizes[i-1];
    }

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
        status = XLzw_compress_Initialize(&compressors[i], base_addrs[i]);
        if (status != XST_SUCCESS) {
            printf("Failed to initialize Lzw_compress HW, %d\r\n", status);
            return 1;
        }

        Xil_DCacheFlushRange((UINTPTR)input + offsets[i], sizes[i]);
        Xil_DCacheFlushRange((UINTPTR)outputs[i], sizes[i] * 2 );

        XLzw_compress_Set_input_r(&compressors[i], (UINTPTR)input + offsets[i]);
        XLzw_compress_Set_output_r(&compressors[i], (UINTPTR)outputs[i]);
        XLzw_compress_Set_input_size(&compressors[i], sizes[i]);
    }

	// for (int i = 0; i < NUMBER_OF_CORES; i++) {
	//     printf("Input data of chunk %d (%d bytes):\n", i, sizes[i]);
	//     print_decimal(input+offsets[i], sizes[i]);       
	// }

    start = get_global_time();

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
        XLzw_compress_Start(&compressors[i]);
    }

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
        while (!XLzw_compress_IsDone(&compressors[i]));
    }

    end = get_global_time();

    uint64_t elapsed_cycles = end - start;
    double elapsed_time_sec = (double)elapsed_cycles / COUNTER_CLK_FREQ_HZ;

    printf("Total compression time: %.6f seconds\r\n", elapsed_time_sec);

    uint32_t compression_sizes[NUMBER_OF_CORES];
    uint64_t total_compression_size = 0;

    for (int i = 0; i < NUMBER_OF_CORES; i++) {
        compression_sizes[i] = XLzw_compress_Get_compression_size(&compressors[i]);
        //printf("Compression size of core %d is : %u\n", i, compression_sizes[i]);
        Xil_DCacheInvalidateRange((UINTPTR)outputs[i], compression_sizes[i]);
        total_compression_size += compression_sizes[i];
    }

    printf("Total compression size = %lu\n", (unsigned long)total_compression_size);
    printf("Compression ratio: %.2f%%\n", 100.0 * (double)total_compression_size / input_length);

    status = WriteSD(outputs, compression_sizes);
    if (status != XST_SUCCESS){
        printf("WriteSD failed, error code %d\n", status);
    }

    // for (int i = 0; i < NUMBER_OF_CORES; i++) {
    //     printf("Core %d compressed output (%u bytes, decimal values):\n", i, compression_sizes[i]);
    //     print_decimal(outputs[i], compression_sizes[i]);
    // }

    return 0;
}
