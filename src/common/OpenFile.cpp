#include "OpenFile.h"
#include "Kernel.h"
#include "Utility.h"
#include "Inode.h"


File::File() {
    f_count     = 0;
    f_flag      = 0;
    f_ino       = -1;
    f_offset    = 0;
}

Inode* File::GetInode() {
    if (f_ino < 0) return NULL;
    return &Kernel::Instance().GetInodeTable().m_Inode[f_ino];
}


OpenFiles::OpenFiles() {
    for (int i=0; i<OpenFiles::NOFILES; i++)
        ProcessOpenFileTable[i] = NULL;
}


int OpenFiles::AllocFreeSlot() {
    for (int i=0; i<OpenFiles::NOFILES; i++) {
        if (ProcessOpenFileTable[i] == NULL) {
            return i;
        }
    }
    return -1;
}


File* OpenFiles::GetF(int fd) {
    if (fd < 0 || fd >= OpenFiles::NOFILES) {
        return NULL;
    }
    return ProcessOpenFileTable[fd];
}


void OpenFiles::SetF(int fd, File *pFile) {
    if (fd < 0 || fd >= OpenFiles::NOFILES) {
        return;
    }
    ProcessOpenFileTable[fd] = pFile;
}


//=======================================================
// OpenFileTable 是全局共享的
//=======================================================


void OpenFileTable::Reset() {
    for (int i=0; i<OpenFileTable::NFILE; i++)
        m_File[i] = File();
}



File* OpenFileTable::FAlloc() {
    int fd;
    User &u = Kernel::Instance().GetUser();

    fd = u.u_ofiles.AllocFreeSlot();

    if (fd < 0) {
        return NULL;
    }

    Kernel::Instance().GetUser().u_tmp = fd;

    for (int i=0; i<OpenFileTable::NFILE; i++) {
        // f_count == 0 说明空闲
        if (m_File[i].f_count == 0) {
            u.u_ofiles.SetF(fd, &m_File[i]);
            m_File[i].f_count++;
            m_File[i].f_offset = 0;
            return (&this->m_File[i]);
        }
    }

    Utility::LogError("No Free File Struct..");
    return NULL;
}


void OpenFileTable::CloseF(File *pFile) {

    if (pFile->f_count <= 1) {
        Kernel::Instance().GetInodeTable().IPut(pFile->GetInode());
    }

    pFile->f_count--;
}