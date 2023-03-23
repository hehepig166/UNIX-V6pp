#include "DeviceManager.h"
#include "BufferManager.h"
#include "Buf.h"
#include "Kernel.h"
#include "Utility.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

bool IS_ROOT_PROG;

char strbuf[1024];





void test_default()
{
    
    auto &fs = Kernel::Instance().GetFileSystem();
    auto &bm = Kernel::Instance().GetBufferManager();
    Buf     *pBuf = NULL;
    Inode   *pnode = NULL;
    int     t1, t2;


//==================================================
// test block alloc/free

    cout <<"\n=================test alloc/free=================" <<endl;

    pBuf = fs.Alloc(0);
    memset(strbuf, '1', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    bm.RlsBlk(pBuf);
    cout <<"alloc and fill 1 in block " <<(t1 = pBuf->b_blkno) <<endl;

    pBuf = fs.Alloc(0);
    memset(strbuf, '2', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    bm.RlsBlk(pBuf);
    cout <<"alloc and fill 2 in block " <<(t2 = pBuf->b_blkno) <<endl;

    fs.Free(0, t1);
    cout <<"free blk " <<t1 <<endl;

    pBuf = Kernel::Instance().GetFileSystem().Alloc(0);
    memset(strbuf, '#', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 128);
    bm.RlsBlk(pBuf);
    cout <<"alloc and write 128 # in block " <<(t1 = pBuf->b_blkno) <<endl;

    fs.Free(0, t1);
    cout <<"free blk " <<t1 <<endl;

    fs.Free(0, t2);
    cout <<"free blk " <<t2 <<endl;



//===================================================
// test inode alloc/free

    cout <<"\n=================test ialloc/ifree=================" <<endl;

    pnode = fs.IAlloc(0);
    pnode->Unlock();
    t1 = pnode->i_number;
    cout <<"ialloc " <<t1 <<endl;

    pnode = fs.IAlloc(0);
    pnode->Unlock();
    t2 = pnode->i_number;
    cout <<"ialloc " <<t2 <<endl;

    fs.IFree(0, t1);
    cout <<"ifree " <<t1 <<endl;

    pnode = fs.IAlloc(0);
    pnode->Unlock();
    t1 = pnode->i_number;
    cout <<"ialloc " <<t1 <<endl;

    fs.IFree(0, t1);
    cout <<"ifree " <<t1 <<endl;

    fs.IFree(0, t2);
    cout <<"ifree " <<t2 <<endl;

}


void test()
{
    auto &fs = Kernel::Instance().GetFileSystem();
    auto &dm = Kernel::Instance().GetDeviceManager();
    auto &bm = Kernel::Instance().GetBufferManager();
    auto &fm = Kernel::Instance().GetFileManager();
    auto &u = Kernel::Instance().GetUser();
    string op;
    string str, str1, str2;
    Buf *pBuf;
    Inode *pnode;
    int t, t1, t2, tt;

    while (1) {
        cout <<"\n[" <<u.u_curdirstr <<"]$ ";
        fflush(stdout);

        if (!IS_ROOT_PROG) Kernel::Instance().GetDeviceManager().ReOpen();

        cin >>op;

        if (op == "exit") {
            break;
        }


        else if (op == "ls") {
            auto flist = fm.GetDirList(u.u_cdir);
            for (auto &[dent, offset]: flist) if (*dent.m_name) {
                cout <<dent.m_name <<endl;
            }
        }
        else if (op == "create") {
            cin >>str;
            t = fm.Create(str.c_str(), FileManager::CREATE);
            if (t >= 0) fm.Close(t);
            cout <<t <<endl;
        }
        else if (op == "upload") {
            int mode = (Inode::IRWXU|Inode::IRWXG|Inode::IRWXO);
            int tot = 0;
            cin >>str1 >>str2;
            t1 = open(str1.c_str(), O_RDONLY);
            if (t1 < 0) {
                cout <<"faild to open " <<str1 <<endl;
                continue;
            }
            t2 = fm.Create(str2.c_str(), mode);
            if (t2 < 0) {
                cout <<"failed to open " <<str2 <<endl;
                close(t1);
                continue;
            }
            cout <<"create [" <<t2 <<"]" <<endl;
            while ((t = read(t1, strbuf, 1024)) > 0) {
                while (t > 0) {
                    //cout <<"..." <<endl;
                    tt = fm.Write(t2, strbuf, t);
                    if (tt > 0) {
                        t -= tt;
                    }
                    tot += tt;
                    //cout <<tt <<"bytes, tot " <<tot <<endl;
                    // if (tot == 6097920) {
                    //     int dfsafsadf = 1;
                    //     getchar();
                    //     cout <<"getchar" <<endl;
                    //     getchar();
                    // }
                }
            }
            cout <<tot <<" bytes" <<endl;
            close(t1);
            cout <<"close [" <<t2 <<"] return " <<fm.Close(t2) <<endl;
        }
        else if (op == "download") {
            int mode = (Inode::IRWXU|Inode::IRWXG|Inode::IRWXO);
            int tot = 0;
            cin >>str1 >>str2;
            t1 = fm.Open(str1.c_str(), mode);
            if (t1 < 0) {
                cout <<"faild to open " <<str1 <<endl;
                continue;
            }
            t2 = open(str2.c_str(), O_RDWR|O_CREAT);
            if (t2 < 0) {
                cout <<"failed to open " <<str2 <<endl;
                fm.Close(t1);
                continue;
            }
            cout <<"open [" <<t1 <<"]" <<endl;
            while ((t = fm.Read(t1, strbuf, 1024)) > 0) {
                while (t > 0) {
                    //cout <<"..." <<endl;
                    tt = write(t2, strbuf, t);
                    if (tt > 0) {
                        t -= tt;
                    }
                    tot += tt;
                    //cout <<tt <<"bytes, tot " <<(tot+=tt) <<endl;
                }
            }
            cout <<tot <<" bytes";
            close(t2);
            cout <<"close [" <<t1 <<"] return " <<fm.Close(t1) <<endl;
        }

        else if (op == "alloc") {
            cin >> t;
            pBuf = fs.Alloc(t);
            if (pBuf == NULL) {
                cout <<"failed." <<endl;
                continue;
            }
            cout <<"blkno = " <<pBuf->b_blkno <<endl;
            bm.RlsBlk(pBuf);
        }
        else if (op == "free") {
            cin >>t1 >>t2;
            fs.Free(t1, t2);
        }
        else if (op == "ialloc") {
            cin >> t;
            pnode = fs.IAlloc(t);
            pnode->Unlock();
            t1 = pnode->i_number;
            cout <<"ino = " <<t1 <<endl;
        }
        else if (op == "ifree") {
            cin >>t1 >>t2;
            fs.IFree(t1, t2);
        }


        else if (op == "adddev") {
            cin >>str;
            cout <<dm.Add(str.c_str()) <<endl;
        }
        else if (op == "devs") {
            int ndev = dm.GetNblkDev();
            cout <<ndev <<endl;
            for (int i=0; i<ndev; i++) {
                cout <<"[" <<i <<"] " <<dm.GetBlockDeviceName(i) <<endl;
            }
        }

        
    }


}



int main(int argc, char **argv)
{
    IS_ROOT_PROG = (argc == 2 && argv[1][0]=='r');
    //IS_ROOT_PROG = 1;

    if (IS_ROOT_PROG) {
        Kernel::Instance().Initialize("disk.img");
    }
    else {
        Kernel::Instance().ConnectInitialize("disk.img");
    }


    cout <<"sizeof(DeviceManager)   " <<sizeof(DeviceManager) <<endl;
    cout <<"sizeof(BufferManager)   " <<sizeof(BufferManager) <<endl;
    cout <<"sizeof(Inode)           " <<sizeof(Inode) <<endl;
    cout <<"sizeof(SuperBlock)      " <<sizeof(SuperBlock) <<endl;
    
    //test_default();
    test();

    Kernel::Instance().Shutdown();
}