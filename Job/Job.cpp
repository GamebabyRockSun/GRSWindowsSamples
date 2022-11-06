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

    //����һ��Ĭ�ϵİ�ȫ���Խṹ(���̳�)
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES)};
    sa.bInheritHandle = FALSE;

    //����Job����(����)
    g_hJob = CreateJobObject(&sa,NULL);
    //������ɶ˿ڶ���(1�߳�)
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,1);

    //��Job��������ɶ˿ڶ����
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT jobiocp = {};
    jobiocp.CompletionPort = g_hIOCP;
    jobiocp.CompletionKey  = g_hJob;        //��JOB����ľ����ΪCompletionKey
    SetInformationJobObject(g_hJob,JobObjectAssociateCompletionPortInformation
        ,&jobiocp,sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

    //��������Job�¼���IOCP�߳�(1�߳�)
    HANDLE hIOCPThread = GRS_BEGINTHREAD(IOCPThread,g_hIOCP);

    //Ϊ��ҵ��������һЩ������Ϣ
    JOBOBJECT_BASIC_LIMIT_INFORMATION jbli = {};
    //���ƽ��̵��û�ʱ���������ֵ,��λ��100����
    //����������Ϊ20����
    jbli.PerProcessUserTimeLimit.QuadPart = 20 * 1000 * 10i64;   
    //�����������Ϊ256k
    jbli.MaximumWorkingSetSize            = 256 * 1024;
    jbli.MinimumWorkingSetSize            = 4 * 1024;

    jbli.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_WORKINGSET;
    
    SetInformationJobObject(g_hJob,JobObjectBasicLimitInformation
        ,&jbli,sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION));
    
    //ָ������ʾ�쳣�رնԻ���
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags =  JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION; 
    SetInformationJobObject(g_hJob,JobObjectExtendedLimitInformation
        ,&jeli,sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));


    TCHAR pAppPath[MAX_PATH] = {};
    GetAppPath(pAppPath);
    StringCchCat(pAppPath,MAX_PATH,_T("JobProcess.exe"));
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi = {};
    //����ͣ���̷߳�ʽ����һ������(���Ը�Ϊѭ���������),ע�����CREATE_BREAKAWAY_FROM_JOB��־
    //��Ϊ���������,VSĬ�ϻ�Ϊ����ӽ��̰�һ��JOB,ʹ��CREATE_BREAKAWAY_FROM_JOB��־�ʹ���һ
    //�������Ľ���
    CreateProcess(pAppPath,NULL,&sa,&sa,FALSE,
        CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB ,NULL,NULL,&si,&pi);
    //��������Job�����
    AssignProcessToJobObject(g_hJob,pi.hProcess);
    //�ý��̿�ʼ����(�ָ����̵����߳�ִ��)
    ResumeThread(pi.hThread);


    //��ѯһЩ������ҵ�����ͳ����Ϣ,�����в�ѯ����ͳ����Ϣ��IOͳ����Ϣ
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION jbioinfo = {};
    DWORD dwNeedLen = 0;
    QueryInformationJobObject(g_hJob,JobObjectBasicAndIoAccountingInformation
        ,&jbioinfo,sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),&dwNeedLen);

	
    //�Ƚ����˳�
    WaitForSingleObject(pi.hProcess,INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    //��IOCPThread����һ���˳�����Ϣ,ָ��dwNumberOfBytesTransferredΪ0
    PostQueuedCompletionStatus(g_hIOCP,0,(ULONG_PTR)g_hJob,NULL);
    //���߳��˳�
    WaitForSingleObject(hIOCPThread,INFINITE);
    CloseHandle(hIOCPThread);

    //�ر���ҵ�����
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
            GRS_PRINTF(_T("IOCPThread: GetQueuedCompletionStatus ����ʧ��,������: 0x%08x \n"),GetLastError());
            continue;
        }

        switch(dwReasonID)
        {
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            //Indicates that a process associated with the job exited with an exit code 
            //that indicates an abnormal exit (see the list following this table). 
            //The value of lpOverlapped is the identifier of the exiting process.
            {//�����쳣�˳�
                dwProcessID = (DWORD)lpOverlapped;
                DWORD dwExitCode = 0;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID);
                if( NULL != hProcess )
                {
                    GetExitCodeProcess(hProcess,&dwExitCode);
                    CloseHandle(hProcess);
                }

                GRS_PRINTF(_T("����[ID:%u]�쳣�˳�,�˳���:%u\n"),dwProcessID,dwExitCode);    

            }
            break;
        case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
                //Indicates that the active process limit has been exceeded. 
                //The value of lpOverlapped is NULL.
            {//ͬʱ��������޵���

            }
            break;
        case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
            //Indicates that the active process count has been decremented to 0. 
            //For example, if the job currently has two active processes, 
            //the system sends this message after they both terminate. 
            //The value of lpOverlapped is NULL.
            {//û�л�Ľ�����

            }
            break;
        case JOB_OBJECT_MSG_END_OF_JOB_TIME:
            //Indicates that the JOB_OBJECT_POST_AT_END_OF_JOB option is in effect 
            //and the end-of-job time limit has been reached. Upon posting this message, 
            //the time limit is canceled and the job's processes can continue to run. 
            //The value of lpOverlapped is NULL.
            {//��ҵ����CPUʱ�����ںľ�

            }
            break;
        case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
            //Indicates that a process has exceeded a per-process time limit. The system sends 
            //this message after the process termination has been requested. 
            //The value of lpOverlapped is the identifier of the process that exceeded its limit.
            {//���̺ľ���CPU����
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("����[ID:%u]�ľ�ʱ��Ƭ\n"),dwProcessID); 
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
                GRS_PRINTF(_T("����[ID:%u]�����˳�,�˳���:%u\n"),dwProcessID,dwExitCode);    
            }
            break;

        case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
            //Indicates that a process associated with the job caused the job to exceed the job-wide 
            //memory limit (if one is in effect).The value of lpOverlapped specifies the identifier 
            //of the process that has attempted to exceed the limit. The system does not send this 
            //message if the process has not yet reported its process identifier.
            {//��ҵ���������ڴ浽����ֵ
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("����[ID:%u]�����ڴ������ﵽ����\n"),dwProcessID); 
            }
            break;
        case JOB_OBJECT_MSG_NEW_PROCESS:
            //Indicates that a process has been added to the job. Processes added to a job at the 
            //time a completion port is associated are also reported. 
            //The value of lpOverlapped is the identifier of the process added to the job.
            {//�½��̼�����ҵ����
                dwProcessID = (DWORD)lpOverlapped;
                GRS_PRINTF(_T("����[ID:%u]������ҵ����[h:0x%08X]\n"),dwProcessID,hJob); 
            }
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            //Indicates that a process associated with the job has exceeded its memory limit (if one is in effect). 
            //The value of lpOverlapped is the identifier of the process that has exceeded its limit. 
            //The system does not send this message if the process has not yet reported its process identifier.
            {//��ҵ���������ڴ浽����ֵ

            }
            break;
        default:
            {
                bLoop = FALSE;
            }
            break;
        }

    }//while

    GRS_PRINTF(_T("IOCP�߳�(ID:0x%x)�˳�\n"),GetCurrentThreadId());

    return 0;
}
