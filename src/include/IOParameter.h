#ifndef HEHEPIG_IOPARAMETER_H
#define HEHEPIG_IOPARAMETER_H

#include "Parameters.h"

struct IOParameter {
    char*   m_Base;     // 当前目标区域的首地址
    int     m_Offset;   // 当前字节偏移量
    int     m_Count;    // 当前还剩余的字节数量

    IOParameter(): m_Base(NULL), m_Offset(0), m_Count(0) {}
};



#endif