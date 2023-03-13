#include "MemoryDescriptor.h"
#include "Kernel.h"
#include "PageManager.h"
#include "Machine.h"
#include "PageDirectory.h"
#include "Video.h"

void MemoryDescriptor::Initialize()
{
	KernelPageManager& kernelPageManager = Kernel::Instance().GetKernelPageManager();
}

void MemoryDescriptor::Release()
{
	KernelPageManager& kernelPageManager = Kernel::Instance().GetKernelPageManager();
}

unsigned int MemoryDescriptor::MapEntry(PageTable* pUserPageTable, unsigned long virtualAddress, unsigned int size, unsigned long phyPageIdx, bool isReadWrite) {
	unsigned long address = virtualAddress - USER_SPACE_START_ADDRESS;
	
	//�����pagetable����һ����ַ��ʼӳ��
	unsigned long startIdx = address >> 12;
	unsigned long cnt = ( size + (PageManager::PAGE_SIZE - 1) )/ PageManager::PAGE_SIZE;

	PageTableEntry* entrys = (PageTableEntry*)pUserPageTable;
	for ( unsigned int i = startIdx; i < startIdx + cnt; i++, phyPageIdx++ )
	{
		entrys[i].m_Present = 0x1;
		entrys[i].m_ReadWriter = isReadWrite;
		entrys[i].m_PageBaseAddress = phyPageIdx;
	}
	return phyPageIdx;
}

unsigned long MemoryDescriptor::GetTextStartAddress()
{
	return this->m_TextStartAddress;
}
unsigned long MemoryDescriptor::GetTextSize()
{
	return this->m_TextSize;
}
unsigned long MemoryDescriptor::GetDataStartAddress()
{
	return this->m_DataStartAddress;
}
unsigned long MemoryDescriptor::GetDataSize()
{
	return this->m_DataSize;
}
unsigned long MemoryDescriptor::GetStackSize()
{
	return this->m_StackSize;
}

bool MemoryDescriptor::EstablishUserPageTable( unsigned long textVirtualAddress, unsigned long textSize, unsigned long dataVirtualAddress, unsigned long dataSize, unsigned long stackSize )
{
	User& u = Kernel::Instance().GetUser();

	/* �������������û��������8M�ĵ�ַ�ռ����� */
	if ( textSize + dataSize + stackSize  + PageManager::PAGE_SIZE > USER_SPACE_SIZE - textVirtualAddress)
	{
		u.u_error = User::ENOMEM;
		Diagnose::Write("u.u_error = %d\n",u.u_error);
		return false;
	}

	// дҳ��
	this->MapToPageTable();

	return true;
}


void MemoryDescriptor::MapToPageTable()
{
	User& u = Kernel::Instance().GetUser();
	PageTable* pUserPageTable = Machine::Instance().GetUserPageTableArray();	// ָ������ҳ��

	unsigned int textAddress;			// ����������׵�ַ
	if ( u.u_procp->p_textp != NULL ) textAddress = u.u_procp->p_textp->x_caddr;

	// ���ҳ��
	for (unsigned int i=0; i<Machine::USER_PAGE_TABLE_CNT; i++) {
		for (unsigned int j=0; j<PageTable::ENTRY_CNT_PER_PAGETABLE; j++) {
			pUserPageTable[i].m_Entrys[j].m_Present = 0;
			pUserPageTable[i].m_Entrys[j].m_ReadWriter = 0;
			pUserPageTable[i].m_Entrys[j].m_UserSupervisor = 1;
			pUserPageTable[i].m_Entrys[j].m_PageBaseAddress = 0;
		}
	}

	unsigned int x_idx = textAddress>>12;				// ֻ����������Σ�
	unsigned int p_idx = (u.u_procp->p_addr>>12)+1;		// ��д�������ݶΣ�

	// �����
	x_idx = MapEntry(pUserPageTable, m_TextStartAddress, m_TextSize - m_RDataSize, x_idx, false);

	// ֻ�����ݶ�
	x_idx = MapEntry(pUserPageTable, m_RDataStartAddress, m_RDataSize, x_idx, false);

	// ���ݶΣ�ע������÷ֳ������֣�
	int tmpSize = m_RDataStartAddress - m_DataStartAddress;
	p_idx = MapEntry(pUserPageTable, m_DataStartAddress, tmpSize, p_idx, true);
	p_idx = MapEntry(pUserPageTable, m_RDataStartAddress + m_RDataSize, m_DataSize - tmpSize, p_idx, true);

	// ��ջ��
	unsigned long stackStartAddress = (USER_SPACE_START_ADDRESS + USER_SPACE_SIZE - m_StackSize) & 0xFFFFF000;
	p_idx = MapEntry(pUserPageTable, stackStartAddress, m_StackSize, p_idx, true);
	
	// 0#�߼�ҳ�� ӳ�䵽 0#ҳ��������ں˻���õ��û�̬����
	pUserPageTable[0].m_Entrys[0].m_Present = 1;
	pUserPageTable[0].m_Entrys[0].m_ReadWriter = 1;
	pUserPageTable[0].m_Entrys[0].m_PageBaseAddress = 0;

	// ˢ�� TLB
	FlushPageDirectory();
}
