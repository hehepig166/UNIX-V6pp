#include "Utility.h"

#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

int Utility::GetTime() {
    return (int)time(NULL);
}

void Utility::Panic(const char *content) {
    printf("%s\n", content);
    exit(-1);
}

void Utility::LogError(const char *content) {
    printf("%s\n", content);
}

void Utility::Sleep(int secs) {
    Sleep(secs);
}

void Utility::ByteCopy(const char *src, char *dst, int count) {
    memcpy(dst, src, count);
}

void Utility::DWordCopy(const int *src, int *dst, int count) {
    while (count--) {
        *dst++ = *src++;
    }
    return;
}

int Utility::Min(const int x, const int y) {
    return x<y ? x : y;
}