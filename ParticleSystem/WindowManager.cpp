#include "WindowManager.h"
#include <stdio.h>


WindowManager::WindowManager()
{
}


WindowManager::~WindowManager()
{
}

bool WindowManager::Init(UINT width, UINT height, WNDPROC wndproc)
{
	HMODULE instance = GetModuleHandle(NULL);

	LPCSTR classname = "wndclass";

	WNDCLASS wndClass;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;// the global one
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = instance;
	wndClass.hIcon = LoadIcon(instance, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = 0;
	wndClass.lpszClassName = classname;

	if (!RegisterClass(&wndClass))
	{
		puts("Failed to register Window Class");
		return false;
	}

	windowHandle = CreateWindow(classname, "ParticleSystem", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);
	if (!windowHandle)
	{
		puts("Failed to createWindow");
		return false;
	}
	// These 2 return a bool, i should handle this
	ShowWindow(windowHandle, SW_SHOW);
	UpdateWindow(windowHandle);
	return true;
}

HWND & WindowManager::GetHandle()
{
	return windowHandle;
}
