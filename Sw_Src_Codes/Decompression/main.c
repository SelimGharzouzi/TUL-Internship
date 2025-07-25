#include "function.h"


int main(void){
    Dictionary_init();

    int status = ReadSD();
    if (status != XST_SUCCESS){
        printf("ReadSD failed\n");
        return 1;
    }
    
    decompress();
    
    status = WriteSD();
    if (status != XST_SUCCESS)
        printf("WriteSD failed\n");

    Dictionary_print();
    printf("Program Ended\n");
    return 0;
}
