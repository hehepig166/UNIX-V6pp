#include <stdio.h>

void main1(int argc, char **argv)
{
    int i;
    printf("argvs: ");
    for (i=0; i<argc; i++) printf("%s ", argv[i]);
    printf("\nWelcome to UnixV6++!\n");
}