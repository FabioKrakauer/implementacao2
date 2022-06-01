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
char *subFrame;
char *subTBL;

int fifoMM = 0;

//STATS
int totalValidated = 0;
int totalPageFault = 0;
double pageFaultRate = 0.0;


pageTable_t pageTable[PAGE_TABLE_SIZE];
frame_t frames[MEMORY_SIZE];

FILE * correctFile;

void clearStructs();
int isPageFault(int page);
void checkValue(int address, int page, int offset);
int findInBackStore(int page, int offset, int vA);
int getFreeFrame();
int findPageFrameIndex(int frameIndex);
int getFrameByFifo();

int main(int argc, char **argv) {
    if(argc < 4) {
        printf("Número de argumentos invalidos, digite o arquivo e os metodos que deseja executar.");
        return 0;
    }
    // Define parameters
    fileName = argv[1];
    subFrame = argv[2];
    subTBL = argv[3]; 

    //Clear all indexes
    clearStructs();

    correctFile = fopen("correct.txt", "wb");
    FILE *file = fopen(fileName, "r");
    if(file == NULL) {
        printf("ERROR TO OPEN file %s!", fileName);
        return 0;
    }

    char n[100];
    int i = 1000;
    while(i > 0 && fgets(n, 60, file) != NULL) {
        int address = atoi(n);
        unsigned char offset = address & 0xFF;
        unsigned char page = address >> 8;
        checkValue(address, page, offset);
        totalValidated += 1;
        i--;
    }
    pageFaultRate = (double)totalPageFault / totalValidated;
    fprintf(correctFile, "Number of Translated Addresses = %d\n", totalValidated);
    fprintf(correctFile, "Page Faults = %d\n", totalPageFault);
    fprintf(correctFile, "Page Fault Rate = %.3f", pageFaultRate);
    printf("\ntotal page fault %d\n", totalPageFault);
    return 0;
}

void checkValue(int address, int page, int offset) {
    // fprintf(correctFile, "Validando %d pagina %d offset %d\n", address, page, offset);
    if(isPageFault(page)) {
        totalPageFault += 1;
        // fprintf(correctFile, "Page fault %d ad %d\n", page, address);
        int frame = findInBackStore(page, offset, address);
        checkValue(address, page, offset);
    }else {
        frame_t frame = *pageTable[page].frame;
        int pysAddress = frame.index * PAGE_SIZE + offset;
        // 26443 = x * 256 + 75
        fprintf(correctFile, "Virtual address: %d Physical address: %d Value: %d\n", address, pysAddress, frame.value[offset]);
    }
}
int findInBackStore(int page, int offset, int vA) {
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
    fread(f->value, 1, PAGE_SIZE, bs);

    int nextFrame = getFreeFrame();
    int oldPageFromFrame = findPageFrameIndex(nextFrame);

    f->index = nextFrame;
    frames[nextFrame] = *f;

    pageTable_t *pt = (pageTable_t *)malloc(sizeof(pageTable_t));
    pt->frame = f;
    pt->status = VALID_STATUS;
    if(oldPageFromFrame > -1) {
        pageTable_t old = pageTable[oldPageFromFrame];
        old.status = INVALID_STATUS;
        pageTable[oldPageFromFrame] = old;
    }
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
    int i = getFrameByFifo();
    // printf("FRAME %d\n", i);
    return i;
    // for(int i = 0; i < MEMORY_SIZE; i++) {
    //     if(frames[i].using == INVALID_STATUS){
    //         free = i;
    //         break;
    //     }
    // }
    // if(free == -1 && strcmp(subFrame, "fifo") == 0) {
    //     free = getFrameByFifo();
    // }
    // return free;
}
void clearStructs() {
    frame_t *frame = (frame_t *)malloc(sizeof(frame_t));
    frame->using = INVALID_STATUS;
    frame->value = NULL;
    frame->index = -1;
    for(int i = 0; i < MEMORY_SIZE; i++) {
        frames[i] = *frame;
    }
    pageTable_t *pt = (pageTable_t *)malloc(PAGE_TABLE_SIZE * sizeof(pageTable_t));
    pt->status = INVALID_STATUS;
    pt->frame = frame;
    for(int i = 0; i < PAGE_TABLE_SIZE; i++) {
        pageTable[i] = *pt;
    }
}
int getFrameByFifo() {
    int frame = fifoMM;
    fifoMM++;
    if(fifoMM == MEMORY_SIZE) {
        fifoMM = 0;
    }
    return frame;
}
int findPageFrameIndex(int frameIndex) {
    int page = -1;
    for(int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if(pageTable[i].frame->index == frameIndex && pageTable[i].status == VALID_STATUS) {
            page = i;
            break;
        }
    }
    return page;
}