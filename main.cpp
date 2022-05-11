#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

void log(unsigned char *msg, int len, const char *label) {

  printf("%s:", label);

  for(int i = 0; i < len; i++) {
    printf(" %02x", msg[i]);
  }
  
  printf("\n");
}

unsigned long long expandBit(int x, int y) {
    unsigned long long result = x;
    result <<= y;
    return result;
}


unsigned long long getPointer(unsigned char *array, int offset) {
    unsigned long long result = array[offset];
    result |= expandBit(array[1 + offset], 8);
    result |= expandBit(array[2 + offset], 16);
    result |= expandBit(array[3 + offset], 24);
    result |= expandBit(array[4 + offset], 32);
    result |= expandBit(array[5 + offset], 40);
    result |= expandBit(array[6 + offset], 48);
    result |= expandBit(array[7 + offset], 56);
    return result;
}

void get(unsigned char *array, unsigned char *key) {
    unsigned char *branch = (unsigned char*)(getPointer(array, 0));
    unsigned int level = 0;
    unsigned int branchSize = branch[0];
    unsigned int offset = 0;

    for (;;) {
        if (offset > branchSize) {
            printf("End (End of Branch)\n");
            break;
        }
        int ptrOffset = 1 + (offset * 9);
        if (branch[ptrOffset] == key[level]) {
            unsigned long long ptr = getPointer(branch, ptrOffset + 1);
            if (ptr == 0) {
                printf("End (Pointer Invalid)\n");
                break;
            } else {
                printf("Found at Level %u\n", level);
                level++;
                branch = (unsigned char*)(ptr);
                offset = 0;
                branchSize = branch[0];
                printf("New Branch Size: %u\n", branchSize);
            }
        } else {
            offset += 1;
        }
    }
}

int getBranchSize(int size) {
    return 10 + (size * 9);
}

void printSpace(int repeat) {
    for (int iA = 0; iA < repeat; iA++) {
        printf(" ");
    }
}

void printTree(unsigned char *branch, int repeat) {
    unsigned int branchSize = branch[0] + 1;

    printSpace(repeat);
    printf("Branch Size: %u\n", branchSize);
    
    for (int i = 0; i < branchSize; i++) {
        printSpace(repeat);
        int offset = 1 + (i * 9);
        unsigned long long ptr = getPointer(branch, offset + 1);
        log(branch + offset, 8, "Branch");
        if (ptr != 0 && repeat < 31) {
            printTree((unsigned char*)(ptr), repeat+1);
        }
    }
}

void set(unsigned char *array, unsigned char *key, unsigned char* value) {
    unsigned int parentPtr = 0;
    unsigned char *parent = array;
    unsigned char *branch = (unsigned char*)(getPointer(array, 0));
    unsigned int level = 0;
    unsigned int branchSize = branch[0];
    unsigned int offset = 0;

    for (;;) {
        if (offset > branchSize) {
            int newPtr = getBranchSize(branchSize);
            branch = (unsigned char *)realloc(branch, newPtr + 9);
            branch[0] = branchSize + 1;

            branch[newPtr] = key[level];
            if (level == 31) {
                memcpy(branch + newPtr + 1, &value, 8);
                break;
            } else {
                unsigned char* sub = (unsigned char*)malloc(9);
            
                for (int i = 0; i < 9; i++) {
                    sub[i] = 0;
                }
                sub[1] = key[level + 1];

                memcpy(branch + newPtr + 1, &sub, 8);

                level++;
                parent = branch;
                parentPtr = newPtr + 1;
                branch = sub;
                offset = 0;
                branchSize = branch[0];
            }

            memcpy(parent + parentPtr, &branch, 8);
            printf("End (End of Branch)\n");
            continue;
        }
        int ptrOffset = 1 + (offset * 9);
        if (branch[ptrOffset] == key[level]) {
            unsigned long long ptr = getPointer(branch, ptrOffset + 1);
            if (ptr == 0) {
                branch[ptrOffset] = key[level];
                if (level == 31) {
                    memcpy(branch + ptrOffset + 1, &value, 8);
                    break;
                } else {
                    unsigned char* sub = (unsigned char*)malloc(9);
                
                    for (int i = 0; i < 9; i++) {
                        sub[i] = 0;
                    }
                    sub[1] = key[level + 1];

                    memcpy(branch + ptrOffset + 1, &sub, 8);

                    level++;
                    parent = branch;
                    parentPtr = ptrOffset + 1;
                    branch = sub;
                    offset = 0;
                    branchSize = branch[0];
                }

                printf("End (Pointer Invalid)\n");
                continue;
            } else {
                printf("Found at Level %u\n", level);
                level++;
                parent = branch;
                parentPtr = ptrOffset + 1;
                branch = (unsigned char*)(ptr);
                offset = 0;
                branchSize = branch[0];
                printf("New Branch Size: %u\n", branchSize);
            }
        } else {
            offset += 1;
        }
    }
}

unsigned char TestKey[] = {0x9F, 0xE5, 0x8E, 0x1A, 0x6F, 0x99, 0xD1, 0xD8, 0x31, 0xB4, 0xFE, 0xDC, 0xF8, 0xDD, 0x3D, 0x9C, 0xE1, 0xAA, 0x5A, 0x44, 0x84, 0x3E, 0x97, 0x6B, 0x40, 0xF7, 0x6E, 0xE5, 0xB7, 0xDC, 0xCD, 0xD8};
unsigned char TestValue[] = "Hello";

unsigned char TestKey2[] = {0x9F, 0xE5, 0x8E, 0x1A, 0x6F, 0x99, 0xD1, 0xD8, 0x31, 0xB4, 0xFE, 0xDC, 0xF8, 0xDD, 0x3D, 0x9C, 0xE1, 0xAA, 0x56, 0x44, 0x84, 0x3E, 0x97, 0x6B, 0x40, 0xF7, 0x6E, 0xE5, 0xB7, 0xDC, 0xCD, 0xD8};


int main() {
    unsigned char* rootPtr = (unsigned char*)malloc(8);
    unsigned char* root = (unsigned char*)malloc(9);
    for (int i = 0; i < 9; i++) {
        root[i] = 0;
    }
    
    unsigned char* value = (unsigned char*)malloc(8);
    memcpy(rootPtr, &root, 8);

    set(rootPtr, TestKey, TestValue);
    set(rootPtr, TestKey2, TestValue);

    printTree((unsigned char*)(getPointer(rootPtr, 0)), 0);
    return 0;
}
