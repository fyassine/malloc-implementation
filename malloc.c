#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#define META_SIZE sizeof(struct block_meta)
void *global_base = NULL;

struct block_meta {
    size_t size;
    struct block_meta *next;
    int free;
};

struct block_meta *find_free_blocks(struct block_meta **last, size_t size) {
    struct block_meta *current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

struct block_meta *request_space(struct block_meta *last, size_t size) {
    struct block_meta *block;
    block = sbrk(0);
    void *request = sbrk(size + META_SIZE);
    if (request == (void*) -1) {
        return NULL; // whenever sbrk fails to allocate space
    }

    if (last) {
        last->next = block;
    }

    block->size = size;
    block->next = NULL;
    block->free = 0;
    
    return block;
}

void *malloc(size_t size) {
    struct block_meta *block;
    
    if (size <= 0) {
        return NULL;
    }

    if (!global_base) {
        block = request_space(NULL, size);
        if (!block) {
            return NULL;
        }
        global_base = block;
    } else {
        struct block_meta *last = global_base;
        block = find_free_blocks(&last, size);
        if (!block) { // no free block allocated
            block = request_space(last, size);
            if(!block) {
                return NULL;
            }
        } else { // found free block
            block->free = 0;
        }
    }

    return(block+1);
}

struct block_meta *get_block_ptr(void *ptr) {
    return (struct block_meta*) ptr - 1;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    struct block_meta* block_ptr = get_block_ptr(ptr);
    block_ptr->free = 1;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    struct block_meta *block_ptr = get_block_ptr(ptr);
    if (block_ptr->size >= size) {
        return ptr;
    }

    
    // allocate new space using malloc and free old space.
    // ẃe need to copy old data to new space.
    void *new_ptr;
    new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, (*block_ptr).size);
    free(ptr);
    return new_ptr;
}

void *calloc(size_t nelem, size_t elsize) {
  size_t size = nelem * elsize; 
  void *ptr = malloc(size);
  memset(ptr, 0, size);
  return ptr;
}

int main(void) {
	char* c = (char*) malloc(4 * sizeof(char));
    if (!c) {
        printf("error allocating space");
    }
    else {
        *c = 'a';
        *(c+1) = 'b';
        *(c+2) = 'c';
        *(c+3) = 0;
        printf("%s \n", c);
    }
    return 0;
}