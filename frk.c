#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_TABLE_SIZE 256
#define PAGE_SIZE 256
#define MEMORY_SIZE 128
#define INVALID_STATUS 0
#define VALID_STATUS 1

typedef struct {
    signed char *value;
    int index;
    int using;
}frame_t;

typedef struct {
    int status;
    frame_t *frame;
}pageTable_t;

char *fileName;
char *subPage;
char *subTBL;
int totalPageFault = 0;

pageTable_t pageTable[PAGE_TABLE_SIZE];
frame_t frames[MEMORY_SIZE];

void clearStructs();
int isPageFault(int page);
void checkValue(int address, int page, int offset);
int findInBackStore(int page, int offset);
int getFreeFrame();

int main(int argc, char **argv) {
    if(argc < 4) {
        printf("Número de argumentos invalidos, digite o arquivo e os metodos que deseja executar.");
        return 0;
    }
    // Define parameters
    fileName = argv[1];
    subPage = argv[2];
    subTBL = argv[3];

    //Clear all indexes
    clearStructs();

    FILE *file = fopen(fileName, "r");
    if(file == NULL) {
        printf("ERROR TO OPEN file %s!", fileName);
        return 0;
    }

    char n[100];
    int i = 1000;
    while(fgets(n, 60, file) != NULL) {
        unsigned char offset = atoi(n) & 0xFF;
        unsigned char page = atoi(n) >> 8;
        checkValue(atoi(n), page, offset);
        i--;
    }
    printf("\n total page fault %d\n", totalPageFault);
    return 0;
}

void checkValue(int address, int page, int offset) {
    if(isPageFault(page)) {
        totalPageFault += 1;
        int frame = findInBackStore(page, offset);
        checkValue(address, page, offset);
    }else {
        frame_t frame = *pageTable[page].frame;
        int pysAddress = frame.index * PAGE_SIZE + offset;
        printf("Virtual address: %d Physical address: %d Value: %d\n", address, pysAddress, frame.value[offset]);
    }
}
int findInBackStore(int page, int offset) {
    FILE *bs = fopen("BACKING_STORE.bin", "r");
    if(bs == NULL) {
        printf("Error on open file BACKING STORE");
        return 0;
    }
    long startAt = page * PAGE_SIZE;
    frame_t *f = (frame_t*)malloc(sizeof(frame_t));
    f->using = VALID_STATUS;
    f->value = malloc(PAGE_SIZE * sizeof(signed char));
    fseek(bs, startAt, SEEK_SET);
    int8_t pageData[PAGE_SIZE];
    fread(f->value, 1, PAGE_SIZE, bs);
    int nextFrame = getFreeFrame();
    f->index = nextFrame;
    frames[nextFrame] = *f;
    pageTable_t *pt = (pageTable_t *)malloc(sizeof(pageTable_t));
    pt->frame = f;
    pt->status = VALID_STATUS;
    pageTable[page] = *pt;
    return nextFrame;
}
int isPageFault(int page) {
    if(pageTable[page].status == INVALID_STATUS) {
        return 1;
    }else{
        return 0;
    }
}
int getFreeFrame() {
    int free = -1;
    for(int i = 0; i < MEMORY_SIZE; i++) {
        if(frames[i].using == INVALID_STATUS){
            free = i;
            break;
        }
    }
    return free;
}
void clearStructs() {
    pageTable_t *pt = (pageTable_t *)malloc(PAGE_TABLE_SIZE * sizeof(pageTable_t));
    pt->status = INVALID_STATUS;
    for(int i = 0; i < PAGE_TABLE_SIZE; i++) {
        pageTable[i] = *pt;
    }
    frame_t *frame = (frame_t *)malloc(sizeof(frame_t));
    frame->using = INVALID_STATUS;
    frame->value = NULL;
    for(int i = 0; i < MEMORY_SIZE; i++) {
        frames[i] = *frame;
    }
}