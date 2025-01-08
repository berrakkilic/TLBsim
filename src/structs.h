//
// Created by emreo on 22/06/2024.
//

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stddef.h>
// Correct include for SystemC

struct Request {
    uint32_t addr;
    uint32_t data;
    int we;
};

struct Result {
    size_t cycles;
    size_t misses;
    size_t hits;
    size_t primitiveGateCount;
};


#endif
