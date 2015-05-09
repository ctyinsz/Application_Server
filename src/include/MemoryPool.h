#ifndef MEMORY_POOL
#define MEMORY_POOL
#include	"gas.inc"

typedef unsigned short USHORT;
typedef unsigned long ULONG;

typedef struct _MemoryBlock
{
		USHORT nSize;
		USHORT nFree;
		USHORT nFirst;
//		USHORT nDummyAlign1;
		struct _MemoryBlock* pNext;
		char aData[1];
}MemoryBlock_t;
//nType：一个大块包含的内存单元个数。nUnitSize：内存单元字节数
MemoryBlock_t* MemoryBlock_create(USHORT nTypes , USHORT nUnitSize);
void MemoryBlock_destroy(void *p);

//nUnitSize:内存单元大小，nInitSize:第一个内存块包含的内存单元个数，nGrowSize:非第一个内存块包含的内存单元个数
typedef struct _MemoryPool
{
		MemoryBlock_t*	pBlock;
		USHORT			nUnitSize;
		USHORT			nInitSize;
		USHORT			nGrowSize;
		pthread_mutex_t lock;
}MemoryPool_t;

int MemoryPool_create(MemoryPool_t*,USHORT nUnitSize, USHORT nInitSize , USHORT nGrowSize );
void MemoryPool_destroy(MemoryPool_t*);
void* Mem_Alloc(MemoryPool_t*);
void Mem_Free(MemoryPool_t*,void* p);

///////////////////////////////////////////////////////////
#endif

