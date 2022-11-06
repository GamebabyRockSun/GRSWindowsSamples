#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

__inline VOID GetAppPath(LPTSTR pszBuffer)
{
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuffer,MAX_PATH)))
	{
		return;			
	}
	DWORD i = dwLen;
	for(; i > 0; i -- )
	{
		if( '\\' == pszBuffer[i] )
		{
			pszBuffer[i + 1] = '\0';
			break;
		}
	}
}


HANDLE g_hJob  = NULL;
HANDLE g_hIOCP = NULL;

DWORD WINAPI IOCPThread( LPVOID lpParam );

int _tmain()
{
    GRS_USEPRINTF();

    //创建一个默认的安全属性结构(不继承)
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES)};
    sa.bInheritHandle = FALSE;

    //创建Job对象(匿名)
    g_hJob = CreateJobObject(&sa,NULL);
    //创建完成端口对象(1线程)
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,1);

    //将Job对象与完成端口对象绑定
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT jobiocp = {};
    jobiocp.CompletionPort = g_hIOCP;
    jobiocp.CompletionKey  = g_hJob;        //把JOB对象的句柄作为CompletionKey
    SetInformationJobObject(g_hJob,JobObjectAssociateCompletionPortInformation
        ,&jobiocp,sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

    //启动监视Job事件的IOCP线程(1线程)
    HANDLE hIOCPThread = GRS_BEGINTHREAD(IOCPThread,g_hIOCP);

    //为作业对象设置一些限制信息
    JOBOBJECT_BASIC_LIMIT_INFORMATION jbli = {};
    //限制进程的用户时间周期最大值,单位是100纳秒
    //此例中设置为20毫秒
    jbli.PerProcessUserTimeLimit.QuadPart = 20 * 1000 * 10i64;   
    //限制最大工作集为256k
    jbli.MaximumWorkingSetSize            = 256 * 1024;
    jbli.MinimumWorkingSetSize            = 4 * 1024;

    jbli.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_WORKINGSET;
    
    SetInformationJobObject(g_hJob,JobObjectBasicLimitInformation
        ,&jbli,sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION));
    
    //指定不显示异常关闭对话框
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags =  JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION; 
    SetInformationJobObject(g_hJob,JobObjectExtendedLimitInformation
        ,&jeli,sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));


    TCHAR pAppPath[MAX_PATH] = {};
    GetAppPath(pAppPath);
    StringCchCat(pAppPath,MAX_PATH,_T("JobProcess.exe"));
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi = {};
    //以暂停主线程方式创建一个进程(可以改为循环创建多个),注意加上CREATE_BREAKAWAY_FROM_JOB标志
    //因为调试情况下,VS默认会为这个子进程绑定一个JOB,使用CREATE_BREAKAWAY_FROM_JOB标志就创建一
    //个独立的进程
    CreateProcess(pAppPath,NULL,&sa,&sa,FALSE,
        CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB ,NULL,NULL,&si,&pi);
    //将进程与Job对象绑定
    AssignProcessToJobObject(g_hJob,pi.hProcess);
    //让进程开始运行(恢复进程的主线程执行)
    ResumeThread(pi.hThread);


    //查询一些关于作业对象的统计信息,本例中查询基本统计信息和IO统计信息
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION jbioinfo = {};
    DWORD dwNeedLen = 0;
    QueryInformationJobObject(g_hJob,JobObjectBasicAndIoAccountingInformation
        ,&jbioinfo,sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),&dwNeedLen);

	
    //等进程退出
    WaitForSingleObject(pi.hProcess,INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    //向IOCPThread发送一个退出的消息,指定dwNumberOfBytesTransferred为0
    PostQueuedCompletionStatus(g_hIOCP,0,(ULONG_PTR)g_hJob,NULL);
    //等线程退出
    WaitForSingleObject(hIOCPThread,INFINITE);
    CloseHandle(hIOCPThread);

    //关闭作业对象等
    CloseHandle(g_hJob);
    CloseHandle(g_hIOCP);
    
    _tsystem(_T("PAUSE"));

	return 0;
}


DWORD WINAPI IOCPThread( LPVOID lpParam )
{
    GRS_USEPRINTF();
    ULONG_PTR   hJob = NULL;
    OVERLAPPED* lpOverlapped = NULL;
    DWORD       dwProcessID = 0;
    DWORD       dwReasonID = 0;
    HANDLE      hIocp = (HANDLE)lpParam;
    BOOL        bLoop = TRUE;

    while (bLoop)
    {
        if(!GetQueuedCompletionStatus(hIocp,&dwReasonID,(PULONG_PTR)&hJob,&lpOverlapped,INFINITE))
        {
            GRS_PRINTF(_T("IOCPThread: GetQueuedCompletionStatus 调用失败,错误码: 0x%08x \n"),GetLastError());
            continue;
        }

        switch(dwReasonID)
        {
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            //Indicates that a process associated with the job exited with an exit code 
            //that indicates an abnormal exit (see the list following this table). 
            //The value of lpOverlapped is the identifier of the exiting process.
            {//进程异常退出
                dwProcessID = (DWORD)lpOverlapped;
                DWORD dwExitCode = 0;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID);
                if( NULL != hProcess )
                {
                    GetExitCodeProcess(hProcess,&dwExitCode);
                    CloseHandle(hProcess);
                }

                GRS_PRINTF(_T("进程[ID:%u]异常退出,退出码:%u\n"),dwProcessID,dwExitCode);    

            }
            break;
        case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
                //Indicates that the active process limit has been exceeded. 
                //The value of lpOverlapped is NULL.
            {//同时活动进程上限到达

            }
            break;
        case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
            //Indicates that the active process count has been decremented to 0. 
            //For example, if the job currently has two active processes, 
            //the system sends this message after they both terminate. 
            //The value of lpOverlapped is NULL.
            {//没有活动的进程了

            }
            break;
        case JOB_OBJECT_MSG_END_OF_JOB_TIME:
            //Indicates that the JOB_OBJECT_POST_AT_END_OF_JOB option is in effect 
            //and the end-of-job time limit has been reached. Upon posting this message, 
            //the time limit is canceled and the job's processes can continue to run. 
            //The value of lpOverlapped is NULL.
            {//作业对象CPU时间周期耗尽

            }
            break;
        case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
            //Indicates that a process has exceeded a per-process time limit. The system sends 
            //this message after the process termination has been requested. 
            //The value of lpOverlapped is the identifier of the process that exceeded its limit.
            {//进程耗尽其CPU周期
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("进程[ID:%u]耗尽时间片\n"),dwProcessID); 
            }
            break;
        case JOB_OBJECT_MSG_EXIT_PROCESS:
            //Indicates that a process associated with the job has exited. 
            //The value of lpOverlapped is the identifier of the exiting process.
            {
                dwProcessID = (DWORD)lpOverlapped;
                DWORD dwExitCode = 0;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID);
                if( NULL != hProcess )
                {
                    GetExitCodeProcess(hProcess,&dwExitCode);
                    CloseHandle(hProcess);
                }
                GRS_PRINTF(_T("进程[ID:%u]正常退出,退出码:%u\n"),dwProcessID,dwExitCode);    
            }
            break;

        case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
            //Indicates that a process associated with the job caused the job to exceed the job-wide 
            //memory limit (if one is in effect).The value of lpOverlapped specifies the identifier 
            //of the process that has attempted to exceed the limit. The system does not send this 
            //message if the process has not yet reported its process identifier.
            {//作业对象消耗内存到限制值
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("进程[ID:%u]消耗内存数量达到上限\n"),dwProcessID); 
            }
            break;
        case JOB_OBJECT_MSG_NEW_PROCESS:
            //Indicates that a process has been added to the job. Processes added to a job at the 
            //time a completion port is associated are also reported. 
            //The value of lpOverlapped is the identifier of the process added to the job.
            {//新进程加入作业对象
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("进程[ID:%u]加入作业对象[h:0x%08X]\n"),dwProcessID,hJob); 
            }
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            //Indicates that a process associated with the job has exceeded its memory limit (if one is in effect). 
            //The value of lpOverlapped is the identifier of the process that has exceeded its limit. 
            //The system does not send this message if the process has not yet reported its process identifier.
            {//作业进程消耗内存到限制值

            }
            break;
        default:
            {
                bLoop = FALSE;
            }
            break;
        }

    }//while

    GRS_PRINTF(_T("IOCP线程(ID:0x%x)退出\n"),GetCurrentThreadId());

    return 0;
}
