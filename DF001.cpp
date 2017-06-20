#include <windows.h>
#include "DFSerial.h"

extern char dst[2];

// прототипчик
void DFDimSwitch(bool dim);
static bool DFinitialized=false;
static bool DFdimmed=false;


// Для установки хука на мышь
static HHOOK handle;
static volatile bool skip_mouse=false; // Признак того, что нажатия на клавиатуру нужно пропускать.

extern volatile int shutdown_thread;

//====================================================================
// Хук для мыши.
//====================================================================
LRESULT  CALLBACK HookProc(int disabled,WPARAM wParam,LPARAM lParam) 
{
	if (!disabled)
	{
		// if(skip_mouse) return 1; // Перехватываем, не даём превратиться в реальное событие от мыши
	}
	return CallNextHookEx(NULL,disabled,wParam,lParam);
} 

//===============================================================
// Собственно оконная процедура
//===============================================================
LRESULT CALLBACK WndProc(HWND hwnd,
						UINT message,
						WPARAM wparam,
						LPARAM lparam)
{
	switch (message)
	{

	case WM_PAINT:
			PAINTSTRUCT ps;
			HDC hdc;
			hdc=BeginPaint(hwnd,&ps);

			TextOut(hdc,20,20,(wchar_t *)dst,1);
			EndPaint(hwnd,&ps);
			break;


	case WM_CREATE:
		SetTimer(hwnd,1,1000,NULL);
		break;

	case WM_DESTROY:	// Завершение программы
		KillTimer(hwnd,1);
		DFDimSwitch(false);
		PostQuitMessage(0);
		break;

	case WM_TIMER:
		// Если были ошибки ввода-вывода, аккуратно уходим
		if(shutdown_thread) DestroyWindow(hwnd);
		else
		{
			int steps_remaining;
			steps_remaining=DFSerial::ReduceSteps();
			if(4>steps_remaining) // Затемняем экран, блокируем клавиатуру
			{
				//Beep(400,400);
				DFDimSwitch(true);
				skip_mouse=true;
			}
			else
			{
				DFDimSwitch(false);
				skip_mouse=false;
			}
		}
		break;

	default: // Сообщения обрабатываются системой
		return DefWindowProc(hwnd,message,wparam,lparam);
	}

return 0;
}

// Глобальные переменные, которые могут потребоваться везде
TCHAR*		DFAppName=L"Затемнение экрана";
HINSTANCE	DFInst;
HWND		DFhwnd;

//Имя класса окна
static const TCHAR DFWindowCName[]=L"DF000WC"; // Имя класса окна
//===================================================================

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR cline,INT)
// Командную строку не обрабатываем
{
	ATOM aresult; // Для всяких кодов возврата
	BOOL boolresult;
	MSG msg; // Сообщение

	// Сразу делаем Instance доступной всем
	DFInst=hInst;

	if(DFSerial::Init()) // Начинаем читать ком-порт и создаём critical_section ещё до создания окна
	{
		return 1;
	}

	// Регистрация класса окна
	WNDCLASS wcl={CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst,
                          //LoadIcon( hInst, MAKEINTRESOURCE(IDI_ICON1)),
						  LoadIcon( NULL, IDI_APPLICATION),
                          LoadCursor(NULL, IDC_ARROW), 
                          (HBRUSH)GetStockObject(WHITE_BRUSH), 
						  NULL,
						  //MAKEINTRESOURCE(IDR_MENU1),
						  DFWindowCName };

	aresult=RegisterClass(&wcl);

	if (aresult==0)
	{
		return (1);
	}


	//Создание главного окна
	DFhwnd=CreateWindow(DFWindowCName,
		//LoadSave::GetTitle(),
		DFAppName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		//CW_USEDEFAULT, CW_USEDEFAULT,
		//CW_USEDEFAULT, CW_USEDEFAULT,
		400, 300,
		320+8, 240+40,
		0L, 0L,
		hInst, 0L );

	if (DFhwnd==NULL)
	{
		return (1);
	}

	// Показываем окно
    ShowWindow( DFhwnd, SW_SHOWNORMAL );
	//ShowWindow( ZRhwnd, SW_MAXIMIZE );

	boolresult=UpdateWindow( DFhwnd ); 
	if(boolresult==0)
	{
		return (1);
	}
	


	// Инициализируем работу хука
	handle = SetWindowsHookEx(WH_MOUSE_LL, 
									HookProc, 
                                 GetModuleHandle(NULL), 
                                 NULL);

	//Цикл обработки сообщений
	while(GetMessage(&msg,NULL,0,0)) 
    {
		TranslateMessage( &msg );
        DispatchMessage( &msg );
	}// while !WM_QUIT

	DFSerial::ReduceSteps(6); // выключаем звук, если он
	Sleep(100);

	// Чистим за собой
	UnhookWindowsHookEx(handle);

	DFSerial::Halt(); // пока только уничтожает critical section, но теоретически должна завершить thread с чтением из ком-порта

	return 0;
}

//===================================
// основной функционал
//===================================
WORD GammaArray[3][256];
WORD GammaArrayDimmed[3][256];
WORD wBrightness=0;

void DFDimSwitch(bool dim)
{
	HDC hdc=GetDC(NULL);

	if(!DFinitialized)
	{
		int j;

		GetDeviceGammaRamp(hdc,GammaArray);
		for(j=0;j<256;j++)
		{
			int iArrayValue = j * (128 + wBrightness);

			GammaArrayDimmed[0][j]=
			GammaArrayDimmed[1][j]=
			GammaArrayDimmed[2][j]=iArrayValue;
		}
				
				
		DFinitialized=true;
	}

	if(DFdimmed && !dim) 
	{
		//char_buf[0]=L'@'; InvalidateRect(DFhwnd,NULL,TRUE);
		SetDeviceGammaRamp(hdc,GammaArray);
		DFdimmed=false;
	}
	else if(!DFdimmed && dim) 
	{
		//char_buf[0]=L'#'; InvalidateRect(DFhwnd,NULL,TRUE);
		SetDeviceGammaRamp(hdc,GammaArrayDimmed);
		DFdimmed=true;
	}

	ReleaseDC(NULL, hdc);
}
