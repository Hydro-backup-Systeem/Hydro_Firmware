#include <new>

#include "FreeRTOS.h"

// C memory overrides

#include <cstring>    // memcpy

// ——— Header prefix to track each block’s size ———
struct MallocHeader {
  size_t size;
};

// ——— C overrides, malloc/free/calloc/realloc ———

extern "C" void* malloc(size_t sz) {
  // Allocate header + user data
  MallocHeader* hdr = (MallocHeader*)pvPortMalloc(sizeof(MallocHeader) + sz);
  configASSERT(hdr);
  hdr->size = sz;
  // Return pointer just past the header
  return (void*)(hdr + 1);
}

extern "C" void free(void* ptr) {
  if (!ptr) return;
  // Back up to the header, then free the whole block
  MallocHeader* hdr = ((MallocHeader*)ptr) - 1;
  vPortFree(hdr);
}

extern "C" void* calloc(size_t num, size_t size) {
  size_t total = num * size;
  void* p     = malloc(total);
  if (p) {
    memset(p, 0, total);
  }
  return p;
}

extern "C" void* realloc(void* ptr, size_t newSize) {
  if (!ptr) {
    return malloc(newSize);
  }

  if (newSize == 0) {
    free(ptr);
    return nullptr;
  }

  MallocHeader* oldHdr = ((MallocHeader*)ptr) - 1;
  size_t oldSize       = oldHdr->size;

  void* newPtr = malloc(newSize);
  if (!newPtr) {
    // malloc failed; original ptr still valid
    return nullptr;
  }

  memcpy(newPtr, ptr, (oldSize < newSize) ? oldSize : newSize);

  free(ptr);
  return newPtr;
}

// C++ memory overrides

void* operator new(size_t size) { return malloc(size); }
void* operator new[](size_t size) { return malloc(size); }

void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }

void* operator new(size_t size, const std::nothrow_t&) noexcept { return malloc(size); }
void* operator new[](size_t size, const std::nothrow_t&) noexcept { return malloc(size); }

void operator delete(void* ptr, const std::nothrow_t&) noexcept { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept { free(ptr); }
