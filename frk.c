#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Declara constantes do codigo
#define PAGE_TABLE_SIZE 256
#define PAGE_SIZE 256
#define MEMORY_SIZE 128
#define INVALID_STATUS 0
#define TLB_SIZE 16
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

typedef struct {
    int page;
    int frame;
    int status;
}tlb_t;

//Defini variaveis de entrada
char *fileName;
char *subFrame;
char *subTLB;

//SUB STRUCTES
int fifoMM = 0;
int lruSize = 0;
int fifoTLB = 0;
int tlbSize = 0;
int tlbHits = 0;

//Define estados da aplicação
int totalValidated = 0;
int totalPageFault = 0;
double pageFaultRate = 0.0;
double tlbHitsRate = 0.0;

// Inicializa estrutura de dados
pageTable_t pageTable[PAGE_TABLE_SIZE];
frame_t frames[MEMORY_SIZE];
lruFrame_t lruMemory[MEMORY_SIZE];
tlb_t tlb[TLB_SIZE];

FILE * correctFile;
FILE * bs;

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
int containsTLB(int page);
int getFreeTLBFifo();
int addToTLB(int page, int frame);
int getFreeTLB(int page);
int updateLruTLB(int page);
void printTLB();

int main(int argc, char **argv) {
    if(argc < 4) {
        printf("Número de argumentos invalidos, digite o arquivo e os metodos que deseja executar.");
        return 0;
    }
    // Define parameters
    fileName = argv[1];
    subFrame = argv[2];
    subTLB = argv[3]; 

    if(strcmp(subFrame, "lru") && strcmp(subFrame, "fifo")) {
        printf("Frame must be lru or fifo");
        return 0;
    }
    if(strcmp(subTLB, "lru") && strcmp(subTLB, "fifo")) {
        printf("TLB must be lru or fifo");
        return 0;
    }
    //Clear all indexes
    clearStructs();

    correctFile = fopen("correct.txt", "wb");
    FILE *file = fopen(fileName, "r");
    if(file == NULL) {
        printf("ERROR TO OPEN file %s!", fileName);
        return 0;
    }
    bs = fopen("BACKING_STORE.bin", "r");
    if(bs == NULL) {
        printf("Error on open file BACKING STORE");
        return 0;
    }
    char n[100];
    //Numero de enderecos que serão lidos!
    while(fgets(n, 60, file) != NULL) {
        int address = atoi(n);
        unsigned char offset = address & 0xFF;
        unsigned char page = address >> 8;
        int frame = -1;
        if(!strcmp(subFrame, "lru")) {
            frame = updateLRU(page);
        }
        checkValue(address, page, offset, frame);
        totalValidated += 1;
    }
    pageFaultRate = (double)totalPageFault / totalValidated;
    tlbHitsRate = (double) tlbHits / totalValidated;
    fprintf(correctFile, "Number of Translated Addresses = %d\n", totalValidated);
    fprintf(correctFile, "Page Faults = %d\n", totalPageFault);
    fprintf(correctFile, "Page Fault Rate = %.3f\n", pageFaultRate);
    fprintf(correctFile, "TLB Hits = %d\n", tlbHits);
    fprintf(correctFile, "TLB Hit Rate = %.3f\n", tlbHitsRate);
    
    fclose(correctFile);
    fclose(file);
    fclose(bs);
    return 0;
}

void checkValue(int address, int page, int offset, int lruFrame) {
    if(isPageFault(page)) {
        totalPageFault += 1;
        int frame = findInBackStore(page, offset, address, lruFrame);
        checkValue(address, page, offset, lruFrame);
    }else {
        frame_t frame;
        int containsTlbBool = containsTLB(page);
        if(containsTlbBool > -1) {
            frame = frames[tlb[containsTlbBool].frame];
            tlbHits += 1;
        }else {
            frame = *pageTable[page].frame;
            if(!strcmp(subTLB, "fifo")) {
                addToTLB(page, frame.index);
            }
        }
        if(!strcmp(subTLB, "lru")) {
            updateLruTLB(page);
        }

        int pysAddress = (frame.index * PAGE_SIZE) + offset;
        fprintf(correctFile, "Virtual address: %d Physical address: %d Value: %d\n", address, pysAddress, frame.value[offset]);
    }
}
int findInBackStore(int page, int offset, int vA, int lruFrame) {
    long startAt = page * PAGE_SIZE;
    frame_t *f = (frame_t*)malloc(sizeof(frame_t));
    f->using = VALID_STATUS;
    f->value = malloc(PAGE_SIZE * sizeof(signed char));
    fseek(bs, startAt, SEEK_SET);
    fread(f->value, sizeof(signed char), PAGE_SIZE, bs);

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
        pageTable[oldPageFromFrame].status = INVALID_STATUS;
    }
    pageTable[page] = *pt;

    return nextFrame;
}
int isPageFault(int page) {
    int containsTlb = containsTLB(page);
    if((containsTlb == -1 || (containsTlb > -1 && tlb[containsTlb].status == INVALID_STATUS)) && pageTable[page].status == INVALID_STATUS) {
        return 1;
    }else{
        return 0;
    }
}
int addToTLB(int page, int frame) {
    tlb_t *tlbInst = (tlb_t *)malloc(sizeof(tlb_t));
    tlbInst->frame = frame;
    tlbInst->page = page;
    tlbInst->status = VALID_STATUS;
    int tlbIndex = getFreeTLB(page);
    tlb[tlbIndex] = *tlbInst;
    return tlbIndex;
}
int getFreeFrame(int page) {
    if(!strcmp(subFrame, "fifo")) {
        return getFrameByFifo();
    }
    if(!strcmp(subFrame, "lru")) {
        return lruMemory[containsLRU(page)].frameIndex;
    }
    return -1;
}

int getFreeTLB(int page) {
    if(!strcmp(subTLB, "fifo")) {
        return getFreeTLBFifo();
    }
    if(!strcmp(subTLB, "lru")) {
        int ind = containsTLB(page);
        if(ind > -1) {
            return ind;
        }
        return updateLruTLB(page);
    }
    return -1;
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

    tlb_t *tlbAloc = (tlb_t *)malloc(TLB_SIZE * sizeof(tlb_t));
    tlbAloc->frame = -1;
    tlbAloc->page = -1;
    tlbAloc->status = INVALID_STATUS;
    for(int i = 0; i < TLB_SIZE; i++) {
        tlb[i] = *tlbAloc;
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
    int alocAt = -1;
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
int containsTLB(int page) {
    int index = -1;
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb[i].page == page) {
            return i;
        }
    }
    return index;
}
int getFreeTLBFifo() {
    int index = fifoTLB;
    fifoTLB += 1;
    if(fifoTLB == TLB_SIZE) {
        fifoTLB = 0;
    }
    return index;
}
int updateLruTLB(int page) {
    tlb_t *alocFrame = (tlb_t *)malloc(sizeof(tlb_t));
    int alocAt = -1;
    int curframe = pageTable[page].frame->index;
    int contains = containsTLB(page);
    
    if(tlbSize < TLB_SIZE) {
        if(contains > -1) {
            printf("tlb n cheia e pag %d ja existe frame %d\n", page, contains);
            tlb_t oldLru = tlb[contains];
            alocFrame->frame = curframe;
            alocFrame->page = page;
            for(int i = contains; i < TLB_SIZE; i++){
                if(tlb[i + 1].frame > -1) {
                    tlb[i].frame = tlb[i + 1].frame;
                    tlb[i].page = tlb[i + 1].page;
                    alocAt = i + 1;
                }else {
                    break;
                }
            }
        }else {
            alocFrame->frame = curframe;
            alocFrame->page = page;
            alocAt = tlbSize;
            tlbSize += 1;
        }
        tlb[alocAt] = *alocFrame;

        return alocAt;
    }
    //procura pagina ja existente no array
    int pageFoundAt = containsTLB(page);
    // printf("TLB CHEIA PAG %d exi? %d");
    if(pageFoundAt > -1) {
        //Atualiza todos os indexes a partir do index que ele estava
        alocFrame->frame = tlb[pageFoundAt].frame;
        alocFrame->page = tlb[pageFoundAt].page;
    }else {
        alocFrame->frame = curframe;
        alocFrame->page = page;
        pageFoundAt = 0;
    }
    for(int i = pageFoundAt; i < TLB_SIZE; i++){
        tlb[i].frame = tlb[i + 1].frame;
        tlb[i].page = tlb[i + 1].page;
    }
    tlb[TLB_SIZE - 1] = *alocFrame;

    return TLB_SIZE - 1;
}
void printTLB() {
    for(int i = 0; i < TLB_SIZE; i++){
        printf("%d - %d | %d\n", i, tlb[i].page, tlb[i].frame);
    }
}