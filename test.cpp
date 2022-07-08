#include <stdio.h>
#include <stdlib.h>

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

unsigned char countBitsRTL(unsigned char byte, int len) {
    unsigned char count = 0;

    for (int i = 0; i < len; i++) {
        if ((byte & (1 << i)) != 0) {
            count++;
        }
    }

    return count;
}

unsigned short getOffset(unsigned char byte, unsigned char *list, bool getRemainder) {
    setupBitCountMap();
    unsigned char byteOffset = byte >> 3; // Optimized way to do (byte / 8).
    unsigned char bitOffset = byte & 7; // Optimized way to do (byte % 8).

    unsigned short count = 0;
    
    for (int i = 0; i < byteOffset; i++) {
        //count += countBitsLTR(list[i], 8, getRemainder);
        count += bitCountMap[list[i]];
    }

    count += countBitsLTR(list[byteOffset], bitOffset, getRemainder);

    if (getRemainder) {
        for (int i = byteOffset + 1; i < 32; i++) {
            //count += (countBitsLTR(list[i], 8, false) << 8);
            count += (bitCountMap[list[i]] << 8);
        }
    }

    return count;
}

unsigned short getOffset_old(unsigned char byte, unsigned char *list, bool getRemainder) {
    unsigned char byteOffset = byte >> 3;
    unsigned char bitOffset = byte & 7;

    unsigned short count = 0;
    
    for (int i = 0; i < byteOffset; i++) {
        count += countBitsLTR(list[i], 8, getRemainder);
    }

    count += countBitsLTR(list[byteOffset], bitOffset, getRemainder);

    if (getRemainder) {
        for (int i = byteOffset + 1; i < 32; i++) {
            count += (countBitsLTR(list[i], 8, false) << 8);
        }
    }

    return count;
}

bool isSet(unsigned char byte, unsigned char *list) {
    unsigned char byteOffset = byte >> 3;
    unsigned char bitOffset = byte & 7;

    return (list[byteOffset] & (128 >> bitOffset)) != 0;
}

void log(unsigned char *msg, int len, const char *label) {

  printf("%s:", label);

  for(int i = 0; i < len; i++) {
    printf(" %02x", msg[i]);
  }
  
  printf("\n");
}

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

int mainA() {
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
        getOffset(255, TestKeys[i], true);
    }
    
    end = clock();
    printf("%fus per Set\n", ((double) ((end - start) * 1000)) / CLOCKS_PER_SEC / i * 1000);
}

int main() {
    unsigned char list[32] = { 0xff };
    for (int i = 0; i < 32; i++) {
        list[i] = 0b11000000;
    }
    log(list, 32, "List");
    unsigned short a = getOffset(1, list, true);
    printf("Is Set? %u\n",isSet(1, list));
    printf("Offset: %u\n", a & 0xff);
    printf("Remainder: %u\n", a >> 8);
}