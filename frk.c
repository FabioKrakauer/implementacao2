#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Declara constantes do codigo
#define PAGE_TABLE_SIZE 256
#define PAGE_SIZE 256
#define MEMORY_SIZE 128
#define INVALID_STATUS 0
#define VALID_STATUS 1

// Defini estruturas
typedef struct {
    signed char *value;
    int index;
    int using;
}frame_t;

typedef struct {
    int status;
    frame_t *frame;
}pageTable_t;

typedef struct {
    int pageNumber;
    int frameIndex;
}lruFrame_t;

//Defini variaveis de entrada
char *fileName;
char *subFrame;
char *subTBL;

//SUB STRUCTES
int fifoMM = 0;
int lruSize = 0;

//Define estados da aplicação
int totalValidated = 0;
int totalPageFault = 0;
double pageFaultRate = 0.0;

// Inicializa estrutura de dados
pageTable_t pageTable[PAGE_TABLE_SIZE];
frame_t frames[MEMORY_SIZE];
lruFrame_t lruMemory[MEMORY_SIZE];

FILE * correctFile;

// Declara funções
void clearStructs();
int isPageFault(int page);
void checkValue(int address, int page, int offset, int lruFrame);
int findInBackStore(int page, int offset, int vA, int lruFrame);
int getFreeFrame(int page);
int findPageFrameIndex(int frameIndex);
int getFrameByFifo();
int updateLRU();
int getLRUFrame(int page);
int containsLRU(int page);

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
    //Numero de enderecos que serão lidos!
    while(fgets(n, 60, file) != NULL) {
        int address = atoi(n);
        unsigned char offset = address & 0xFF;
        unsigned char page = address >> 8;
        if(!strcmp(subFrame, "lru")) {
            int frame = updateLRU(page);
            checkValue(address, page, offset, frame);
        }else{
            checkValue(address, page, offset, -1);
        }
        totalValidated += 1;
    }
    pageFaultRate = (double)totalPageFault / totalValidated;
    fprintf(correctFile, "Number of Translated Addresses = %d\n", totalValidated);
    fprintf(correctFile, "Page Faults = %d\n", totalPageFault);
    fprintf(correctFile, "Page Fault Rate = %.3f", pageFaultRate);
    
    fclose(correctFile);
    fclose(file);
    return 0;
}

void checkValue(int address, int page, int offset, int lruFrame) {
    if(isPageFault(page)) {
        totalPageFault += 1;
        int frame = findInBackStore(page, offset, address, lruFrame);
        checkValue(address, page, offset, lruFrame);
    }else {
        frame_t frame = *pageTable[page].frame;
        int pysAddress = frame.index * PAGE_SIZE + offset;
        fprintf(correctFile, "Virtual address: %d Physical address: %d Value: %d\n", address, pysAddress, frame.value[offset]);
    }
}
int findInBackStore(int page, int offset, int vA, int lruFrame) {
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

    int nextFrame = -1;
    nextFrame = getFreeFrame(page);
    int oldPageFromFrame = findPageFrameIndex(nextFrame);
    if(oldPageFromFrame == page) {
        oldPageFromFrame = -1;
    }

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
int getFreeFrame(int page) {
    int free = -1;
    if(!strcmp(subFrame, "fifo")) {
        return getFrameByFifo();
    }
    if(!strcmp(subFrame, "lru")) {
        return lruMemory[containsLRU(page)].frameIndex;
    }
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

    if(!strcmp(subFrame, "lru")) {
        lruFrame_t *lft = (lruFrame_t *)malloc(sizeof(lruFrame_t));
        lft->frameIndex = -1;
        lft->pageNumber = -1;
        for(int i = 0; i < MEMORY_SIZE; i++) {
            lruMemory[i] = *lft;
        }
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
int updateLRU(int page) {
    lruFrame_t *alocFrame = (lruFrame_t *)malloc(sizeof(lruFrame_t));
    int alocAt = 0;
    if(lruSize < MEMORY_SIZE) {
        int contains = containsLRU(page);
        if(contains > -1) {
            lruFrame_t oldLru = lruMemory[contains];
            alocFrame->frameIndex = oldLru.frameIndex;
            alocFrame->pageNumber = oldLru.pageNumber;
            for(int i = contains; i < MEMORY_SIZE; i++){
                if(lruMemory[i + 1].frameIndex > -1) {
                    lruMemory[i].frameIndex = lruMemory[i + 1].frameIndex;
                    lruMemory[i].pageNumber = lruMemory[i + 1].pageNumber;
                    alocAt = i + 1;
                }else {
                    break;
                }
            }
        }else {
            alocFrame->frameIndex = lruSize;
            alocFrame->pageNumber = page;
            alocAt = lruSize;
            lruSize += 1;
        }
        lruMemory[alocAt] = *alocFrame;
        return alocFrame->frameIndex;
    }
    //procura pagina ja existente no array
    int pageFoundAt = containsLRU(page);
    if(pageFoundAt > -1) {
        //Atualiza todos os indexes a partir do index que ele estava
        alocFrame->frameIndex = lruMemory[pageFoundAt].frameIndex;
        alocFrame->pageNumber = lruMemory[pageFoundAt].pageNumber;
    }else {
        alocFrame->frameIndex = lruMemory[0].frameIndex;
        alocFrame->pageNumber = page;
        pageFoundAt = 0;
    }
    for(int i = pageFoundAt; i < MEMORY_SIZE; i++){
        lruMemory[i].frameIndex = lruMemory[i + 1].frameIndex;
        lruMemory[i].pageNumber = lruMemory[i + 1].pageNumber;
    }
    lruMemory[MEMORY_SIZE - 1] = *alocFrame;
    return alocFrame->frameIndex;
}
int containsLRU(int page) {
    int index = -1;
    for(int i = 0; i < MEMORY_SIZE; i++) {
        if(lruMemory[i].pageNumber == page) {
            index = i;
            break;
        }
    }
    return index;
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