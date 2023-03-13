#include <stdio.h>

int main1()
{
    unsigned long a = 0x3ff000u + 0xc0000000u;
    printf("%x\n", a);
    printf("%x\n", sizeof(unsigned long));
}