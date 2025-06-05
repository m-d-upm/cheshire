#ifndef IOMMU_TESTS_H
#define IOMMU_TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define PAGE_SIZE           0x1000ULL     // 4kiB

// iDMA modules generate an interrupt after completing a transfer
#define IDMA_IRQ_EN     (0)

// The IOMMU IP supports MSI translation
// If set, DCs are configured in extended format
#define MSI_TRANSLATION (0)

// Min and max device IDs of DMA devices in the platform.
// DDT entries are created for device IDs in [DID_MIN, DID_MAX]
#define DID_MIN             (1)
#define DID_MAX             (15)

// Interrupt vectors
#define CQ_INT_VECTOR       (0x03ULL)
#define FQ_INT_VECTOR       (0x02ULL)
#define HPM_INT_VECTOR      (0x01ULL)

// Interrupt pending bits
#define CIP_MASK            (1UL << 0)
#define FIP_MASK            (1UL << 1)
#define PMIP_MASK           (1UL << 2)

#define N_MAPPINGS          (32)

typedef uint64_t pte_t;

static inline uint64_t read64(uintptr_t addr){
    return *((volatile uint64_t*) addr);
}

static inline uint32_t read32(uintptr_t addr){
    return *((volatile uint32_t*) addr);
}

static inline uint16_t read16(uintptr_t addr){
    return *((volatile uint16_t*) addr);    
}

static inline uint8_t read8(uintptr_t addr){
    return *((volatile uint8_t*) addr);    
}

static inline void write64(uintptr_t addr, uint64_t val){
    *((volatile uint64_t*) addr) = val;
}

static inline void write32(uintptr_t addr, uint32_t val){
    *((volatile uint32_t*) addr) = val;
}

static inline void write16(uintptr_t addr, uint16_t val){
    *((volatile uint16_t*) addr) = val;    
}

static inline void write8(uintptr_t addr, uint8_t val){
    *((volatile uint8_t*) addr) = val;    
}

static inline void fence_i() {
    asm volatile("fence.i" ::: "memory");
}

#endif /* IOMMU_TESTS_H */
