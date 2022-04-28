#pragma once

#include <vector>
#include <stdint.h>
#include <stdio.h>

#define PORT 8080

inline void printUintVector(std::vector<uint8_t> &vec) {
    for (auto i = 0; i < vec.size(); ++i) {
        uint8_t a = vec.at(i);
            printf("%d; ", a);
    }
    printf("\n");
};

inline void printByteArray(uint8_t *array, int size = 50) {
    for (int i = 0; i < size; i++) {
        printf("%d ;", array[i]);
    }
    printf("\n");
    // printf("%s\n", buffer);
};

