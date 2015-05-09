#include	"gas.inc"

const USHORT MEMPOOL_ALIGNMENT=4;
///////////////////////////////////////////////////////////
MemoryBlock_t* MemoryBlock_create(USHORT nTypes, USHORT nUnitSize)
{
	MemoryBlock_t* pmb = (MemoryBlock_t*)calloc(1, nTypes * nUnitSize + sizeof(MemoryBlock_t));
	if(pmb == NULL)
		return NULL;
		
	pmb->nSize = nTypes * nUnitSize;
	pmb->nFree = nTypes;
	pmb->nFirst = 0;
	pmb->pNext = NULL;
	
	char *pData = pmb->aData;
	USHORT i;
	for (i = 1; i < nTypes; i++)
	{
		*(USHORT*)(pData) = i;
		pData += nUnitSize;
	}		
	return pmb;
}
void MemoryBlock_destroy(void *p)
{
	if(p)
		free(p);
}

int MemoryPool_create(MemoryPool_t* pmp,USHORT _nUnitSize, USHORT _nInitSize, USHORT _nGrowSize)
{
	if(!pmp)
		return FALSE;
	
	pmp->nUnitSize = _nUnitSize;
	pmp->nInitSize = _nInitSize;
	pmp->nGrowSize = _nGrowSize;
	pmp->pBlock = NULL;
	
	if (_nUnitSize > 4)
	{
		pmp->nUnitSize = (_nUnitSize + (MEMPOOL_ALIGNMENT-1)) & ~(MEMPOOL_ALIGNMENT-1);
	}
	else if (_nUnitSize <= 2)
	{
		pmp->nUnitSize = 2;
	}
	else
	{
		pmp->nUnitSize = 4;
	}
	pmp->pBlock = MemoryBlock_create(pmp->nInitSize, pmp->nUnitSize);
	
	if(pthread_mutex_init(&pmp->lock,NULL) != 0)	
		return FALSE;

	return TRUE;
}

void MemoryPool_destroy(MemoryPool_t* pmp)
{
	if(!pmp)
		return;
		
	MemoryBlock_t* NextBlock = pmp->pBlock;
	while (pmp->pBlock)
	{
		NextBlock = pmp->pBlock->pNext;
		MemoryBlock_destroy(pmp->pBlock);
		pmp->pBlock = NextBlock;
	}
	pthread_mutex_destroy(&pmp->lock);
}
void* Mem_Alloc(MemoryPool_t* pmp)
{
	if(!pmp)	return NULL;
	//第一次使用内存池
	if (!pmp->pBlock)
	{ 
		MemoryBlock_t* pMyBlock;
		pMyBlock = MemoryBlock_create(pmp->nInitSize, pmp->nUnitSize);
		if(!pMyBlock)		return NULL;
////////////////////////
//		pmp->pBlock = pMyBlock;
//----------------------
		do
		{
			pMyBlock->pNext = pmp->pBlock;
		}while(!__sync_bool_compare_and_swap(&pmp->pBlock, pMyBlock->pNext, pMyBlock));	
/////////////////////////
	}

	pthread_mutex_lock(&pmp->lock);
	MemoryBlock_t* pMyBlock = pmp->pBlock;
	//寻找一个有空闲内存单元的块
	while (pMyBlock && !pMyBlock->nFree)
		pMyBlock = pMyBlock->pNext;
	
	//内存池中的内存单元用光了，再向系统申请一大块内存
	if(pMyBlock == NULL)
	{
		pthread_mutex_unlock(&pmp->lock);
		
		if (!pmp->nGrowSize)	return NULL;

		pMyBlock = MemoryBlock_create(pmp->nGrowSize, pmp->nUnitSize);	
		if (!pMyBlock) 	return NULL;

		char* pFree = pMyBlock->aData + (pMyBlock->nFirst * pmp->nUnitSize);
		pMyBlock->nFirst = *((USHORT*)pFree);
		(pMyBlock->nFree)--;
		
////////////////////////////////
//		pMyBlock->pNext = pmp->pBlock;
//		pmp->pBlock = pMyBlock;
//------------------------------		
		do
		{
			pMyBlock->pNext = pmp->pBlock;
		}while(!__sync_bool_compare_and_swap(&pmp->pBlock, pMyBlock->pNext, pMyBlock));
////////////////////////////////		
		return (void*)pFree;
	}
/////////////////////////////////////////	
	//如果找到了有空闲内存单元的块，分配内存单元
	char* pFree = pMyBlock->aData + (pMyBlock->nFirst * pmp->nUnitSize);
	if(pMyBlock->nFirst != *((USHORT*)pFree))
	{
		pMyBlock->nFirst = *((USHORT*)pFree);
		(pMyBlock->nFree)--;
	}
	else
	{
		pFree = NULL;
	}
	pthread_mutex_unlock(&pmp->lock);
	return (void*)pFree;
//---------------------------------------	
//	char* pFree = NULL;
//	USHORT nFirst = 0;
//	do
//	{
//		nFirst = pMyBlock->nFirst;
//		pFree = pMyBlock->aData + (nFirst * pmp->nUnitSize);
//	}while(!__sync_bool_compare_and_swap(&pMyBlock->nFirst, nFirst, *((USHORT*)pFree));
//	__sync_fetch_and_sub(&pMyBlock->nFree,1);
//	return (void*)pFree;
//////////////////////////////////////////
}

void Mem_Free(MemoryPool_t* pmp,void* pFree)
{
	if(!pmp)
		return;
		
	MemoryBlock_t* pMyBlock = pmp->pBlock;
	MemoryBlock_t* preBlock = pmp->pBlock;
	//确定pFree在哪个块中
	while ( ((ULONG)pMyBlock->aData > (ULONG)pFree) || ((ULONG)pFree >= ((ULONG)pMyBlock->aData + pMyBlock->nSize)) )
	{
		preBlock = pMyBlock;
		pMyBlock = pMyBlock->pNext;
		if (pMyBlock == NULL)
		{
			return;
		}
	}
///////////////////////////////////////////
//	pthread_mutex_lock(&pmp->lock);
//	(pMyBlock->nFree)++;
	//把刚刚回收的内存单元放在最前面
/////	*((USHORT*)pFree) = pMyBlock->nFirst;
	//确定回收的内存单元的编号
//	pMyBlock->nFirst = (USHORT)(((ULONG)pFree - (ULONG)(pMyBlock->aData)) / pmp->nUnitSize);
//-----------------------------------------	
	USHORT nFirst = (USHORT)(((ULONG)pFree - (ULONG)(pMyBlock->aData)) / pmp->nUnitSize);
	do
	{
		if(nFirst == pMyBlock->nFirst)
			return;
		*((USHORT*)pFree) = pMyBlock->nFirst;
	}while(!__sync_bool_compare_and_swap(&pMyBlock->nFirst,pMyBlock->nFirst,nFirst));
	__sync_fetch_and_add(&pMyBlock->nFree,1);
//////////////////////////////////////////
	
	//判断是否需要把整个内存块返还给系统
	pthread_mutex_lock(&pmp->lock);
	if (pMyBlock->nFree*pmp->nUnitSize == pMyBlock->nSize)
	{
		//是否是第一个内存块
		if (preBlock == pMyBlock)
		{
			pthread_mutex_unlock(&pmp->lock);
		}
		else
		{
			preBlock->pNext = pMyBlock->pNext;
			pthread_mutex_unlock(&pmp->lock);
			MemoryBlock_destroy(pMyBlock);
		}
	}
	else
		pthread_mutex_unlock(&pmp->lock);
}

