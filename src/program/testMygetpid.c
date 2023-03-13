#include <sys.h>
#include <stdio.h>

int main1()
{
    int pid0 = getpid();
    int pid1 = mygetpid();
    printf("getpid()   = %d\n", pid0);
    printf("mygetpid() = %d\n", pid1);

    return 0;
}