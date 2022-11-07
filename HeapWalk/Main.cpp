#include <tchar.h>
#include <windows.h>
#include <time.h>
#include <iostream>
#include <vector>
void DisplayHeapsInfo(std::ostream& out = std::cout);

int _tmain()
{
	DisplayHeapsInfo();
	_tsystem(_T("PAUSE"));
	return 0;

	srand((unsigned int)time(NULL));
	const int iCnt = 100;
	HANDLE hHeap = GetProcessHeap();
	void* pMem[iCnt];
	ZeroMemory(pMem,iCnt * sizeof(void * ));

	for(int i = 0; i < iCnt; i++)
	{
		pMem[i] = HeapAlloc(hHeap,0,rand()%iCnt);
	}
	
	PROCESS_HEAP_ENTRY phe = {};
	HeapLock(hHeap);
	int iBlock = 0;
	//遍历进程默认堆
	while (HeapWalk(hHeap, &phe))
	{
		++iBlock;

	}
	HeapUnlock(hHeap);

	for(int i = 0; i < iCnt; i++)
	{
		HeapValidate(hHeap,0,pMem[i]);
		HeapSize(hHeap,0,pMem[i]);
		HeapFree(hHeap,0,pMem[i]);
		pMem[i] = NULL;
	}

	//下面的代码演示如何遍历一个进程中所有的堆
	DWORD dwHeapCnt = 0;
	PHANDLE pHArray = NULL;
	dwHeapCnt = GetProcessHeaps(0,NULL);
	if( dwHeapCnt > 0 )
	{
		pHArray = (PHANDLE)HeapAlloc(GetProcessHeap(),0,dwHeapCnt * sizeof(HANDLE));
		GetProcessHeaps(dwHeapCnt,pHArray);
		for(DWORD i = 0; i < dwHeapCnt; i ++)
		{
			HeapLock(pHArray[i]);
			iBlock = 0;
			ZeroMemory(&phe,sizeof(PROCESS_HEAP_ENTRY));
			while (HeapWalk(pHArray[i], &phe))
			{
				++iBlock;
				//......
			}
			HeapUnlock(pHArray[i]);
		}
		HeapFree(GetProcessHeap(),0,pHArray);
	}
	return 0;
}

void DisplayHeapsInfo(std::ostream& out) 
{     
	std::vector<HANDLE> heaps(GetProcessHeaps(0, NULL));
	GetProcessHeaps((DWORD)heaps.size(), &heaps[0]); 
	DWORD totalBytes = 0;
	for(DWORD i = 0; i < heaps.size(); ++i) 
	{
		out << "Heap handle: 0x" << heaps[i] << '\n';   
		PROCESS_HEAP_ENTRY phi = {0}; 
		while(HeapWalk(heaps[i], &phi)) 
		{         
			out << "Block Start Address: 0x" << phi.lpData << '\n'; 
			out << "\tSize: " << phi.cbData << " - Overhead: "  
				<< static_cast<DWORD>(phi.cbOverhead) << '\n';     
			out << "\tBlock is a";         
			if(phi.wFlags & PROCESS_HEAP_REGION)  
			{       
				out << " VMem region:\n";   
				out << "\tCommitted size: " << phi.Region.dwCommittedSize << '\n';     
				out << "\tUncomitted size: " << phi.Region.dwUnCommittedSize << '\n';    
				out << "\tFirst block: 0x" << phi.Region.lpFirstBlock << '\n';      
				out << "\tLast block: 0x" << phi.Region.lpLastBlock << '\n';    
			}           
			else     
			{      
				if(phi.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
				{             
					out << "n uncommitted range\n"; 
				}          
				else if(phi.wFlags & PROCESS_HEAP_ENTRY_BUSY)   
				{     
					totalBytes += phi.cbData;    
					out << "n Allocated range: Region index - "   
						<< static_cast<unsigned>(phi.iRegionIndex) << '\n';          
					if(phi.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE)  
					{                
						out << "\tMovable: Handle is 0x" << phi.Block.hMem << '\n';  
					}               
					else if(phi.wFlags & PROCESS_HEAP_ENTRY_DDESHARE)       
					{                  
						out << "\tDDE Sharable\n";    
					}             
				}       
				else out << " block, no other flags specified\n";        
			}          
			out << std::endl;       
		}
	}   
	out << "End of report - total of " << std::dec << totalBytes << " allocated" << std::endl; 
} 
