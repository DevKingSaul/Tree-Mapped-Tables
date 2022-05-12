#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

bool debug = false;

int ptrSize = sizeof(uintptr_t);
int fastTable_elementSize = 1 + ptrSize;
int fastTable_defaultSize = 1 + fastTable_elementSize;

void log(unsigned char *msg, int len, const char *label) {

  printf("%s:", label);

  for(int i = 0; i < len; i++) {
    printf(" %02x", msg[i]);
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

unsigned char TestKey2[] = {0x8F, 0xE5, 0x8E, 0x1A, 0x6F, 0x99, 0xD1, 0xD8, 0x31, 0xB4, 0xFE, 0xDC, 0xF8, 0xDD, 0x3D, 0x9C, 0xE1, 0xAA, 0x56, 0x44, 0x84, 0x3E, 0x97, 0x6B, 0x40, 0xF7, 0x6E, 0xE5, 0xB7, 0xDC, 0xCD, 0xD8};
unsigned char TestValue2[] = "Hello v2";


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
    unsigned char* root = (unsigned char*)malloc(fastTable_defaultSize);
    for (int i = 0; i < fastTable_defaultSize; i++) {
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
        set(rootPtr, TestKeys[i], TestValue);
    }
    
    end = clock();
    printf("%fus per Set\n", ((double) ((end - start) * 1000)) / CLOCKS_PER_SEC / i * 1000);

    start = clock();

    printf("testing Get performance: ");

    for (i = 0; i < 10000; i++) {
        unsigned char *value = get(rootPtr, TestKeys[i]);
        if (value == NULL) {
            printf("Error getting Value\n");
            break;
        }
    }
    
    end = clock();
    printf("%fus per Get\n", ((double) ((end - start) * 1000)) / CLOCKS_PER_SEC / i * 1000);

    /*unsigned char* rootPtr = (unsigned char*)malloc(ptrSize);
    unsigned char* root = (unsigned char*)malloc(fastTable_defaultSize);
    for (int i = 0; i < fastTable_defaultSize; i++) {
        root[i] = 0;
    }
    
    memcpy(rootPtr, &root, ptrSize);

    set(rootPtr, TestKey, TestValue);
    set(rootPtr, TestKey2, TestValue2);

    //printTree((unsigned char*)(getPointer(rootPtr, 0)), 0);

    unsigned char *result1 = get(rootPtr, TestKey);
    unsigned char *result2 = get(rootPtr, TestKey2);

    logChar(result1, 8, "Result 1");

    logChar(result2, 8, "Result 2");*/
    return 0;
}
