#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

bool debug = false;

int ptrSize = sizeof(uintptr_t);
int fastTable_elementSize = 1 + ptrSize;
int fastTable_defaultSize = 1 + fastTable_elementSize;

int orderedFastTable_elementSize = ptrSize;
int orderedFastTable_defaultSize = 32 + ptrSize;

unsigned char bitCountMap[256] = { 0xaf, 0xfa };

void setupBitCountMap() {
    if (!bitCountMap[0] == 0xaf) return; // Check if needs to calculate.
    if (!bitCountMap[1] == 0xfa) return; // Check if needs to calculate.

    for (int e = 0; e < 256; e++) {
        unsigned char count = 0;

        for (unsigned char i = 0; i < 8; i++) {
            if ((e & (128 >> i)) != 0) {
                count++;
            }
        }

        bitCountMap[e] = count;
    }
}

void log(unsigned char *msg, int len, const char *label) {

  printf("%s:", label);

  for(int i = 0; i < len; i++) {
    printf(" %02hhX", msg[i]);
  }
  
  printf("\n");
}

void logChar(unsigned char *msg, int len, const char *label) {
  printf("%s: ", label);

  if (msg == NULL) {
      printf("NULL");
  } else {
    for(int i = 0; i < len; i++) {
        printf("%c", msg[i]);
    }
  }
  
  printf("\n");
}

uintptr_t getPointer(unsigned char *array, int offset) {
    uintptr_t pointer;
    memcpy(&pointer, array + offset, ptrSize);
    return pointer;
}

unsigned short countBitsLTR(unsigned char byte, int len, bool isOffset) {
    unsigned short count = 0;
    unsigned short leftoverCount = 0;

    for (unsigned char i = 0; i < len; i++) {
        if ((byte & (128 >> i)) != 0) {
            count++;
        }
    }

    if (isOffset) {
        for (unsigned char i = len + 1; i < 8; i++) {
            if ((byte & (128 >> i)) != 0) {
                leftoverCount++;
            }
        }
    }

    return (leftoverCount << 8) | count;
}

unsigned short getOffset(unsigned char byte, unsigned char *list, bool getRemainder) {
    unsigned char byteOffset = byte >> 3; // Optimized way to do (byte / 8).
    unsigned char bitOffset = byte & 7; // Optimized way to do (byte % 8).

    unsigned short count = 0;
    
    for (int i = 0; i < byteOffset; i++) {
        count += bitCountMap[list[i]];
    }

    //count += countBitsLTR(list[byteOffset], bitOffset, getRemainder);
    count += bitCountMap[list[byteOffset] >> ( 8 - bitOffset )];
    count += bitCountMap[(list[byteOffset] << (bitOffset + 1)) & 0xff] << 8;

    if (getRemainder) {
        for (int i = byteOffset + 1; i < 32; i++) {
            count += (bitCountMap[list[i]] << 8);
        }
    }

    return count;
}

unsigned char *getBranches(unsigned char *branch) {
    unsigned char *resp = (unsigned char *)malloc(2 + 256);
    unsigned int count = 0;

    for (int i = 0; i < 32; i++) {
        for (unsigned char i2 = 0; i2 < 8; i2++) {
            if ((branch[i] & (128 >> i2)) != 0) {
                resp[2 + count] = (i * 8) + i2;
                count++;
            }
        }
    }

    resp[0] = count >> 8;
    resp[1] = count & 0xff;

    return resp;
}

bool checkSet(unsigned char byte, unsigned char *list) {
    unsigned char byteOffset = byte >> 3; // Optimized way to do (byte / 8).
    unsigned char bitOffset = byte & 7; // Optimized way to do (byte % 8).

    return (list[byteOffset] & (128 >> bitOffset)) != 0;
}

void setListBit(unsigned char byte, unsigned char *list) {
    unsigned char byteOffset = byte >> 3; // Optimized way to do (byte / 8).
    unsigned char bitOffset = byte & 7; // Optimized way to do (byte % 8).

    list[byteOffset] |= (128 >> bitOffset);

    return;
}

unsigned char *getOrdered(unsigned char *array, unsigned char *key, int keyLen) {
    unsigned char *branch = (unsigned char*)(getPointer(array, 0));
    unsigned int level = 0;
    unsigned int branchSize = branch[0];

     for (;;) {
        if (level == keyLen) {
            return (unsigned char*)(getPointer(branch, 32));
            break;
        } else {
            bool isSet = checkSet(key[level], branch);

            if (isSet) {
                unsigned short offset = getOffset(key[level], branch, false);

                int ptrOffset = orderedFastTable_defaultSize + ((offset & 0xff) * 8);

                unsigned long long ptr = getPointer(branch, ptrOffset);

                if (debug) {
                    printf("Found at Level %u\n", level);
                }
                level++;
                branch = (unsigned char*)(ptr);
            } else {
                return NULL;
                break;
            }
        }
        
     }

    return NULL;
}

unsigned char *get(unsigned char *array, unsigned char *key) {
    unsigned char *branch = (unsigned char*)(getPointer(array, 0));
    unsigned int level = 0;
    unsigned int branchSize = branch[0];
    unsigned int offset = 0;

    for (;;) {
        if (offset > branchSize) {
            if (debug) {
                printf("End (End of Branch)\n");
            }
            return NULL;
            break;
        }
        int ptrOffset = 1 + (offset * fastTable_elementSize);
        if (branch[ptrOffset] == key[level]) {
            unsigned long long ptr = getPointer(branch, ptrOffset + 1);
            if (ptr == 0) {
                if (debug) {
                    printf("End (Pointer Invalid)\n");
                }
                return NULL;
                break;
            } else {
                if (debug) {
                    printf("Found at Level %u\n", level);
                }

                if (level == 31) {
                    if (debug) {
                        printf("Found Value: %u\n", offset);
                    }
                    return (unsigned char*)(ptr);
                    break;
                } else {
                    level++;
                    branch = (unsigned char*)(ptr);
                    offset = 0;
                    branchSize = branch[0];
                    if (debug) {
                        printf("New Branch Size: %u\n", branchSize);
                    }
                }
            }
        } else {
            offset += 1;
        }
    }
}

int getBranchSize(int size) {
    return 1 + fastTable_elementSize + (size * fastTable_elementSize);
}

int getOrderedBranchSize(int size) {
    return orderedFastTable_defaultSize + (size * orderedFastTable_elementSize);
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
        int offset = 1 + (i * fastTable_elementSize);
        unsigned long long ptr = getPointer(branch, offset + 1);
        log(branch + offset, fastTable_elementSize, "Branch");
        if (ptr != 0 && repeat < 31) {
            printTree((unsigned char*)(ptr), repeat+1);
        }
    }
}

void printTreeOrdered(unsigned char *branch, int repeat) {
    if (repeat >= 10) return;
    printSpace(repeat);
    log(branch + 32, 8, "Value Pointer");

    unsigned int count = 0;

    for (int i = 0; i < 32; i++) {
        for (unsigned char i2 = 0; i2 < 8; i2++) {
            if ((branch[i] & (128 >> i2)) != 0) {
                int a = (i * 8) + i2;
                printSpace(repeat);
                int offset = orderedFastTable_defaultSize + (count * orderedFastTable_elementSize);
                unsigned long long ptr = getPointer(branch, offset);
                log(branch + offset, orderedFastTable_elementSize, "Branch");
                printSpace(repeat + 1);
                printf("Byte: %02hhX\n", a);
                if (ptr != 0 && repeat < 31) {
                    printTreeOrdered((unsigned char*)(ptr), repeat + 1);
                }
                count++;
            }
        }
    }
}

void setOrdered(unsigned char *array, unsigned char *key, unsigned int keyLen, unsigned char* value) {
    setupBitCountMap(); // Calculate Bit Map if it's not calculated already.

    unsigned int parentPtr = 0;
    unsigned char *parent = array;
    unsigned char *branch = (unsigned char*)(getPointer(array, 0));
    unsigned int level = 0;

    for (;;) {
        if (level == keyLen) {
            memcpy(branch + 32, &value, ptrSize);
            break;
        } else {
            bool isSet = checkSet(key[level], branch);

            if (isSet) {
                unsigned short offset = getOffset(key[level], branch, false);

                int ptrOffset = orderedFastTable_defaultSize + ((offset & 0xff) * 8);

                unsigned long long ptr = getPointer(branch, ptrOffset);

                if (debug) {
                    printf("Found at Level %u\n", level);
                }
                level++;
                parent = branch;
                parentPtr = ptrOffset;
                branch = (unsigned char*)(ptr);
            } else {
                unsigned short offset = getOffset(key[level], branch, true);

                int ptrOffset = orderedFastTable_defaultSize + ((offset & 0xff) * 8);

                int remainderSize = (offset >> 8) * 8;

                branch = (unsigned char *)realloc(branch, ptrOffset + orderedFastTable_elementSize + remainderSize);
                memcpy(parent + parentPtr, &branch, ptrSize);

                memmove (branch + ptrOffset + orderedFastTable_elementSize, branch + ptrOffset, remainderSize);

                setListBit(key[level], branch);

                unsigned char* sub = (unsigned char*)malloc(orderedFastTable_defaultSize);
            
                for (int i = 0; i < orderedFastTable_defaultSize; i++) {
                    sub[i] = 0;
                }

                memcpy(branch + ptrOffset, &sub, ptrSize);

                level++;
                parent = branch;
                parentPtr = ptrOffset;
                branch = sub;
            }
        }
        
     }

    return;
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
            branch = (unsigned char *)realloc(branch, newPtr + fastTable_elementSize);
            branch[0] = branchSize + 1;
            memcpy(parent + parentPtr, &branch, ptrSize);

            branch[newPtr] = key[level];
            if (level == 31) {
                memcpy(branch + newPtr + 1, &value, ptrSize);
                break;
            } else {
                unsigned char* sub = (unsigned char*)malloc(fastTable_defaultSize);
            
                for (int i = 0; i < fastTable_defaultSize; i++) {
                    sub[i] = 0;
                }
                sub[1] = key[level + 1];

                memcpy(branch + newPtr + 1, &sub, ptrSize);

                level++;
                parent = branch;
                parentPtr = newPtr + 1;
                branch = sub;
                offset = 0;
                branchSize = branch[0];
            }

            if (debug) {
                printf("End (End of Branch)\n");
            }
            continue;
        }
        int ptrOffset = 1 + (offset * fastTable_elementSize);
        if (branch[ptrOffset] == key[level]) {
            if (level == 31) {
                memcpy(branch + ptrOffset + 1, &value, ptrSize);
                break;
            } else {
                unsigned long long ptr = getPointer(branch, ptrOffset + 1);
                if (ptr == 0) {
                    branch[ptrOffset] = key[level];
                    unsigned char* sub = (unsigned char*)malloc(fastTable_defaultSize);
                    
                    for (int i = 0; i < fastTable_defaultSize; i++) {
                        sub[i] = 0;
                    }
                    sub[1] = key[level + 1];

                    memcpy(branch + ptrOffset + 1, &sub, ptrSize);

                    level++;
                    parent = branch;
                    parentPtr = ptrOffset + 1;
                    branch = sub;
                    offset = 0;
                    branchSize = branch[0];

                    if (debug) {
                        printf("End (Pointer Invalid)\n");
                    }
                    continue;
                } else {
                    if (debug) {
                        printf("Found at Level %u\n", level);
                    }
                    level++;
                    parent = branch;
                    parentPtr = ptrOffset + 1;
                    branch = (unsigned char*)(ptr);
                    offset = 0;
                    branchSize = branch[0];
                    if (debug) {
                        printf("New Branch Size: %u\n", branchSize);
                    }
                }
            }
        } else {
            offset += 1;
        }
    }
}

unsigned char TestKey[] = {0x9F, 0xE5, 0x8E, 0x1A, 0x6F, 0x99, 0xD1, 0xD8, 0x31, 0xB4, 0xFE, 0xDC, 0xF8, 0xDD, 0x3D, 0x9C, 0xE1, 0xAA, 0x5A, 0x44, 0x84, 0x3E, 0x97, 0x6B, 0x40, 0xF7, 0x6E, 0xE5, 0xB7, 0xDC, 0xCD, 0xD8};
unsigned char TestValue[] = "Hello v1";


unsigned char TestKey2[] = {0x9F, 0xE6, 0x8E, 0x1A, 0x6F, 0x99, 0xD1, 0xD8, 0x31, 0xB4, 0xFE, 0xDC, 0xF8, 0xDD, 0x3D, 0x9C, 0xE1, 0xAA, 0x5A, 0x44, 0x84, 0x3E, 0x97, 0x6B, 0x40, 0xF7, 0x6E, 0xE5, 0xB7, 0xDC, 0xCD, 0xD8};
unsigned char TestValue2[] = "Hello v2";
unsigned char TestValue3[] = "Hello v3";


#include <time.h>
#include <fcntl.h>
#include <unistd.h>

unsigned char *genrandom(int length) {
  unsigned char *bytes = (unsigned char *)malloc(length);
  int fd = open("/dev/urandom", O_RDONLY);
  read(fd, bytes, length);
  close(fd);
  return bytes;
}

int main() {
    // Init Tree
    unsigned char* rootPtr = (unsigned char*)malloc(ptrSize);
    unsigned char* root = (unsigned char*)malloc(orderedFastTable_defaultSize);
    for (int i = 0; i < orderedFastTable_defaultSize; i++) {
        root[i] = 0;
    }
    
    memcpy(rootPtr, &root, ptrSize);

    // Start Test

    clock_t start;
    clock_t end;

    int i;

    unsigned char **TestKeys = (unsigned char**)malloc(sizeof(unsigned char*) * 10000);
    for (i = 0; i < 10000; i++) {
        unsigned char *Key = genrandom(32);
        TestKeys[i] = Key;
    }

    start = clock();

    printf("testing Set performance: ");

    for (i = 0; i < 10000; i++) {
        setOrdered(rootPtr, TestKeys[i], 32, TestValue);
    }
    
    end = clock();
    printf("%fus per Set\n", ((double) ((end - start) * 1000)) / CLOCKS_PER_SEC / i * 1000);

    start = clock();

    printf("testing Get performance: ");

    for (i = 0; i < 10000; i++) {
        unsigned char *value = getOrdered(rootPtr, TestKeys[i], 32);
        if (value == NULL) {
            printf("Error getting Value\n");
            break;
        }
    }
    
    end = clock();
    printf("%fus per Get\n", ((double) ((end - start) * 1000)) / CLOCKS_PER_SEC / i * 1000);

    /*unsigned char* rootPtr = (unsigned char*)malloc(ptrSize);
    unsigned char* root = (unsigned char*)malloc(orderedFastTable_defaultSize);
    for (int i = 0; i < orderedFastTable_defaultSize; i++) {
        root[i] = 0;
    }
    
    memcpy(rootPtr, &root, ptrSize);

    setOrdered(rootPtr, TestKey, 1, TestValue);
    setOrdered(rootPtr, TestKey2, 2, TestValue2);
    setOrdered(rootPtr, TestKey, 3, TestValue3);
    printTreeOrdered((unsigned char*)(getPointer(rootPtr, 0)), 0);

    unsigned char *result1 = getOrdered(rootPtr, TestKey, 1);
    unsigned char *result2 = getOrdered(rootPtr, TestKey2, 2);
    unsigned char *result3 = getOrdered(rootPtr, TestKey, 3);

    logChar(result1, 8, "Result 1");
    logChar(result2, 8, "Result 2");
    logChar(result3, 8, "Result 3");*/
    return 0;
}
