#pragma once
#include <Windows.h>

class WindowManager
{
private:
	HWND windowHandle;
	UINT width = 800, height = 800;
public:
	WindowManager();
	~WindowManager();
	
	bool Init(UINT width, UINT height, WNDPROC wndProc);

	HWND & GetHandle();
};

