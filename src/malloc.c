/**
 * malloc.c
 * Heap Memory Allocator
 */

#include "malloc.h"
#include <stddef.h>
#include "types.h"

static const u8 sEwramHeapName[12] = "HEAP_SYSTEM";

static const u8 sIwramHeapName[16] = "HEAPCPU_SYSTEM";

static Heap sEwramHeap;
static Heap sIwramHeap;

void HeapUnlinkFreeBlock(HeapBlock* block) {
    block->prevFree->nextFree = block->nextFree;
    block->nextFree->prevFree = block->prevFree;
}

u8 HeapContains(const void* ptr, Heap* heap) {
    if (ptr != NULL && (u8*)ptr > (u8*)heap->start && (u8*)ptr < (u8*)heap->end) {
        return TRUE;
    }

    return FALSE;
}

HeapBlock* HeapFindFreeBlock(s32 size, Heap* heap) {
    HeapBlock* block;

    block = heap->start->nextFree;

    while (block != NULL && block != heap->end) {
        if (block->size >= size) {
            return block;
        }

        block = block->nextFree;
    }

    return NULL;
}

void HeapInit(void* addr, u32 size, Heap* heap) {
    HeapBlock* head;
    HeapBlock* tail;
    HeapBlock* first;
    const void* name;
    u32 last;

    size &= ~31;
    head = addr;

    // fakematch
    do {
        last = size - 32;
    } while (0);

    tail = (HeapBlock*)((u8*)addr + last);
    heap->start = head;
    heap->end = tail;
    first = head + 1;
    head->size = -32;
    head->prevFree = NULL;
    head->nextFree = first;
    head->prev = NULL;
    head->next = first;
    name = heap->name;
    head->name = name;
    head->self = head;
    tail->size = -1;
    tail->prevFree = first;
    tail->nextFree = NULL;
    tail->prev = first;
    tail->next = NULL;
    tail->name = name;
    tail->self = tail;
    first->size = size - 64;
    first->prevFree = head;
    first->nextFree = tail;
    first->prev = head;
    first->next = tail;
    first->name = name;
    first->self = first;
    heap->allocFlag = FALSE;
}

void EwramHeapInit(void* addr, u32 size) {
    SetEwramHeapName(sEwramHeapName);
    HeapInit(addr, size, &sEwramHeap);
}

void IwramHeapInit(void* addr, u32 size) {
    SetIwramHeapName(sIwramHeapName);
    HeapInit(addr, size, &sIwramHeap);
}

void* HeapAlloc(u32 size, Heap* heap) {
    HeapBlock* block;
    HeapBlock* prev;
    s32 rem;

    if (size == 0) {
        return NULL;
    }

    size = (size + 63) & ~31;
    block = HeapFindFreeBlock(size, heap);

    if (block == NULL) {
        return NULL;
    }

    if (block->size < (s32)(size + 64)) {
        size = block->size;
        HeapUnlinkFreeBlock(block);
    } else {
        prev = block;
        rem = prev->size - size;
        prev->size = rem;
        block = (HeapBlock*)((u8*)prev + rem);
        block->next = prev->next;
        block->prev = prev;
        prev->next = block;
        block->next->prev = block;
    }

    block->size = -size;
    block->prevFree = NULL;
    block->nextFree = NULL;

    if (heap->allocFlag) {
        block->allocFlag = TRUE;
    } else {
        block->allocFlag = FALSE;
    }

    block->name = heap->name;
    block->self = block;

    return block + 1;
}

void* EwramAlloc(u32 size) {
    return HeapAlloc(size, &sEwramHeap);
}

void* IwramAlloc(u32 size) {
    return HeapAlloc(size, &sIwramHeap);
}

void HeapFree(const void* ptr, Heap* heap) {
    HeapBlock* block;
    HeapBlock* neighbor;
    HeapBlock* head;
    s32 size;

    if (ptr == NULL) {
        return;
    }

    block = (HeapBlock*)ptr - 1;

    if (block->self != block) {
        return;
    }

    if (!HeapContains(ptr, heap)) {
        return;
    }

    size = -block->size;

    if (size < 0) {
        return;
    }

    block->size = size;
    neighbor = block->prev;

    if (neighbor->size > 0) {
        HeapUnlinkFreeBlock(neighbor);
        neighbor->size += size;
        neighbor->next = block->next;
        block->next->prev = neighbor;
        block->prev = NULL;
        block->next = NULL;
        block = neighbor;
    }

    neighbor = block->next;

    if (neighbor->size > 0) {
        HeapUnlinkFreeBlock(neighbor);
        block->size += neighbor->size;
        block->next = neighbor->next;
        neighbor->next->prev = block;
        neighbor->prev = NULL;
        neighbor->next = NULL;
    }

    head = heap->start;
    block->prevFree = head;
    block->nextFree = head->nextFree;
    head->nextFree->prevFree = block;
    head->nextFree = block;
    block->self = NULL;
}

void EwramFree(const void* ptr) {
    HeapFree(ptr, &sEwramHeap);
}

void IwramFree(const void* ptr) {
    HeapFree(ptr, &sIwramHeap);
}

s32 HeapGetBlockSize(void* ptr, Heap* heap) {
    s32 size;

    if (HeapContains(ptr, heap)) {
        size = -((HeapBlock*)ptr - 1)->size;

        if (size > 0) {
            return size;
        }
    }

    return 0;
}

s32 EwramGetBlockSize(void* ptr) {
    return HeapGetBlockSize(ptr, &sEwramHeap);
}

s32 IwramGetBlockSize(void* ptr) {
    return HeapGetBlockSize(ptr, &sIwramHeap);
}

s32 HeapGetFreeTotal(Heap* heap) {
    HeapBlock* block;
    s32 total;

    block = heap->start->nextFree;
    total = 0;

    while (block != NULL && block->size > 0) {
        total += block->size;
        block = block->nextFree;
    }

    return total;
}

s32 EwramGetFreeTotal() {
    return HeapGetFreeTotal(&sEwramHeap);
}

s32 IwramGetFreeTotal() {
    return HeapGetFreeTotal(&sIwramHeap);
}

void func_08000A60(Heap* heap) {
    HeapBlock* block;

    for (block = heap->start; block != NULL; block = block->next) {
        *(volatile s32*)&block->size;
    }
}

void func_08000A70() {
    func_08000A60(&sEwramHeap);
}

void func_08000A80() {
    func_08000A60(&sIwramHeap);
}

void SetEwramHeapAllocFlag(u8 flag) {
    sEwramHeap.allocFlag = flag;
}

void SetIwramHeapAllocFlag(u8 flag) {
    sIwramHeap.allocFlag = flag;
}

void func_08000AA8(Heap* heap) {
    HeapBlock* block;

    for (block = heap->start; block != NULL; block = block->next) {
        *(volatile s32*)&block->size;
    }
}

void func_08000AB8() {
    func_08000AA8(&sEwramHeap);
}

void func_08000AC8() {
    func_08000AA8(&sIwramHeap);
}

void SetEwramHeapName(const void* name) {
    sEwramHeap.name = name;
}

void SetIwramHeapName(const void* name) {
    sIwramHeap.name = name;
}

const void* GetEwramHeapName() {
    return sEwramHeap.name;
}

const void* GetIwramHeapName() {
    return sIwramHeap.name;
}

#ifdef PLATFORM_PC
/* port_rollback.c's test: the heap block holding an address (its name, the offset in its data), for a difference
   found in EWRAM or IWRAM. NULL outside both heaps. */
const char* PortHeapBlockAt(const void* p, u32* offset) {
    Heap* heaps[2] = { &sEwramHeap, &sIwramHeap };
    int i;
    for (i = 0; i < 2; i++) {
        HeapBlock* block;
        if (heaps[i]->start == NULL || (const u8*)p < (const u8*)heaps[i]->start ||
            (const u8*)p >= (const u8*)heaps[i]->end)
            continue;
        for (block = heaps[i]->start; block != NULL && block != heaps[i]->end; block = block->next)
            if ((const u8*)p >= (const u8*)block && (const u8*)p < (const u8*)block->next) {
                *offset = (u32)((const u8*)p - (const u8*)(block + 1));
                return block->size < 0 && block->name ? (const char*)block->name : "free";
            }
    }
    return NULL;
}
#endif
