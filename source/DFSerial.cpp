#include <Windows.h>
#include <process.h>
#include "DFSerial.h"

extern HWND		DFhwnd;

int DFSerial::light=5, DFSerial::last_light=5; // не горит

static uintptr_t handler_Reader=0; // thread handler

volatile int shutdown_thread=0;
char dst[2]={'#',0};
static unsigned long size=1;

static HANDLE Port=0;
static OVERLAPPED oReader = {0}, oWriter = {0};

static CRITICAL_SECTION cs;
static int num_steps=8;

static char outsyms[5]={'0','1','2','3','4'};

//====================================================================
// И читаем, и пишем
//====================================================================
static unsigned __stdcall ReaderThread(void *p)
{
    DWORD dwIncommingReadSize;

	do
	{

		do
		{
			dwIncommingReadSize=0;
			if(!ReadFile(Port, dst, 1, NULL, &oReader))
			//if(!ReadFile(Port, dst, 1, &dwIncommingReadSize, NULL))
			{
				// Ждём-с реального прочтения
				if(ERROR_IO_PENDING==GetLastError())
					WaitForSingleObject(oReader.hEvent, INFINITE ); // Ждать бесконечно
				else
					shutdown_thread=1;
			}

			EnterCriticalSection(&cs);
			num_steps++;
			LeaveCriticalSection(&cs);
			
			InvalidateRect(DFhwnd,NULL,TRUE);
        } while (dwIncommingReadSize > 0);
		
		if(num_steps<9)
		{
			if(!WriteFile(Port,&outsyms[num_steps/2],1,&size,&oWriter))
			{
				if(ERROR_IO_PENDING==GetLastError())
					WaitForSingleObject(oWriter.hEvent, INFINITE ); // Ждать бесконечно
				else
					shutdown_thread=1;
			}
		}
	} while(!shutdown_thread);

/*
	while(ReadFile(Port,dst,1,&size,0))
	{
		// для отладки было
		//WriteFile(Port,dst,1,&size,0);
		EnterCriticalSection(&cs);
		num_steps++;
		LeaveCriticalSection(&cs);
		if(num_steps<5)
		{
			WriteFile(Port,&outsyms[num_steps],1,&size,0);
		}
		InvalidateRect(DFhwnd,NULL,TRUE);
	}
*/
	return 0;
}

//==========================================================================
// По таймеру снижает счётчик шагов (thread-safe)
//==========================================================================
int DFSerial::ReduceSteps(int increase_value)
{
		EnterCriticalSection(&cs);
		num_steps--;
		if(num_steps<0) num_steps=0;
		num_steps+=increase_value;
		LeaveCriticalSection(&cs);
		if(num_steps<9)
		{
			if(!WriteFile(Port,&outsyms[num_steps/2],1,&size,&oWriter))
			{
				if(ERROR_IO_PENDING==GetLastError())
					WaitForSingleObject(oWriter.hEvent, INFINITE ); // Ждать бесконечно
				else
					shutdown_thread=1; // используется и для выхода из основной программы
			}
		}
		return num_steps;
}

//==========================================================================
// порождаем процесс-считыватель порта
//==========================================================================
DCB dcb;
int DFSerial::Init()
{
	// 1. Создаём поток
	if(!handler_Reader)
	{
		// Сначала порт
		Port = CreateFile(L"\\\\.\\COM9", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		//Port = CreateFile(L"\\\\.\\COM9", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (Port == INVALID_HANDLE_VALUE) 
		{
			MessageBox(NULL, L"Невозможно открыть последовательный порт", L"Error", MB_OK);
			Port=0;
			return 1;
		}

		// Скорость настраиваем
		FillMemory(&dcb, sizeof(dcb), 0);
		dcb.DCBlength = sizeof(dcb);
		if (!BuildCommDCB(L"9600,n,8,1", &dcb))
		{
			// Couldn't build the DCB. Usually a problem
			// with the communications specification string.
			return 1;
		}

		if (!SetCommState(Port, &dcb))
		{
			return 1;
		}

		oReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		oWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		InitializeCriticalSection(&cs);

		// А потом уж поток
		handler_Reader=_beginthreadex(NULL,0,ReaderThread,0,0,NULL);
		if(1>handler_Reader)
		{
			handler_Reader=0;
			return 1;
		}

		return 0; // Всё закончлось успешно
   
	}
	else return 1;
			
}

//==========================================================================
// уничтожаем процесс-считыватель порта
//==========================================================================
void DFSerial::Halt()
{
	if(handler_Reader)
	{
		DeleteCriticalSection(&cs);

		shutdown_thread=1;
		
		// ЗАБЬЁМ ПОКА, ПРОСТО ВЫХОДИМ И ВСЁ


		// Прибить все операции ввода-вывода
		//CancelIoEx(hFile,NULL);

		// Ждать, когда очнётся 
		// Теоретически здесь возможно thread race... Но пока забьём. Если зависнет при выходе - прибьём.
		// WaitForSingleObject((HANDLE)handler_Reader,0);
	}
}