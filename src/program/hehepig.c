#include <stdio.h>
#include <sys.h>

void test1()
{
    int pid, ppid, tmp;

    pid = getpid();
    ppid = getppid(pid);
    tmp = getppid(999);

    printf("pid:  %d\n", pid);
    printf("ppid: %d\n", ppid);
    printf("tmp:  %d\n", tmp);
}



void test2()
{
    int pid = getpid();
    int res;
    struct t_myinfo myinfo;
    printf("[%d] myinfo addr: %08x\n", pid, &myinfo);
    printf("[%d] { %08x %08x %08x %08x %08x %08x }\n", pid, myinfo.xdxxysb_wldz, myinfo.xdxxysb_wlykh, myinfo.ppdaStartAddress_v, myinfo.ppdaStartAddress_p, myinfo.textStartAddress_v, myinfo.textStartAddress_p);
    while (1) {
        res = mygetinfo(&myinfo);
        //printf("[%d] mygetinfo return %d\n", pid, res);
        printf("[%d] { %08x %08x %08x %08x %08x %08x }\n", pid, myinfo.xdxxysb_wldz, myinfo.xdxxysb_wlykh, myinfo.ppdaStartAddress_v, myinfo.ppdaStartAddress_p, myinfo.textStartAddress_v, myinfo.textStartAddress_p);
        sleep(1);
    }
}

int main1(int argc, char **argv)
{
    test2();
}