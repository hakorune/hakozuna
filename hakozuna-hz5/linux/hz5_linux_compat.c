#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct Hz5LinuxAlignedHdr {
  void* mapping;
  size_t mapping_size;
} Hz5LinuxAlignedHdr;

static size_t hz5_linux_page_size(void) {
  long value = sysconf(_SC_PAGESIZE);
  return value > 0 ? (size_t)value : (size_t)4096;
}

void* _aligned_malloc(size_t size, size_t alignment) {
  if (alignment < sizeof(void*)) {
    alignment = sizeof(void*);
  }
  if ((alignment & (alignment - 1u)) != 0) {
    errno = EINVAL;
    return NULL;
  }

  size_t page_size = hz5_linux_page_size();
  size_t overhead = alignment + sizeof(Hz5LinuxAlignedHdr);
  if (size > SIZE_MAX - overhead) {
    errno = ENOMEM;
    return NULL;
  }
  size_t request = size + overhead;
  size_t mapping_size = (request + page_size - 1u) & ~(page_size - 1u);

  void* mapping =
      mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)mapping + sizeof(Hz5LinuxAlignedHdr);
  uintptr_t aligned = (raw + alignment - 1u) & ~(uintptr_t)(alignment - 1u);
  Hz5LinuxAlignedHdr* hdr =
      (Hz5LinuxAlignedHdr*)(aligned - sizeof(Hz5LinuxAlignedHdr));
  hdr->mapping = mapping;
  hdr->mapping_size = mapping_size;
  return (void*)aligned;
}

void _aligned_free(void* ptr) {
  if (!ptr) {
    return;
  }
  Hz5LinuxAlignedHdr* hdr =
      (Hz5LinuxAlignedHdr*)((uintptr_t)ptr - sizeof(Hz5LinuxAlignedHdr));
  if (hdr->mapping && hdr->mapping_size) {
    munmap(hdr->mapping, hdr->mapping_size);
  }
}
