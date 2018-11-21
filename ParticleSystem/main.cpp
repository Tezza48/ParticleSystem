#include <Windows.h>
#include <chrono>
#include <stdio.h>
#include <queue>
#include "Renderer.h"


LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

bool isRunning = true;
float syncInterval = 1;

MSG msg;

ID3D11InputLayout * inputLayout;
ID3D11VertexShader * vShader;
ID3D11PixelShader * pShader;

const int WIDTH = 800, HEIGHT = 800;

using std::puts;
using std::printf;
using std::queue;
using namespace std::chrono;

void Cleanup(void)
{
	if (inputLayout)inputLayout->Release();
	if (vShader)vShader->Release();
	if (pShader)pShader->Release();

	system("PAUSE");
}

int main(int argc, char ** argv)
{
	atexit(::Cleanup);

	HMODULE instance = GetModuleHandle(NULL);
	
	LPCSTR classname = "wndclass";

	WNDCLASS wndClass;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = ::WindowProc;// the global one
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
		return -1;
	}

	HWND window = CreateWindow(classname, "ParticleSystem", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, 0, 0, instance, 0);

	ShowWindow(window, SW_SHOW);
	UpdateWindow(window);

	puts("Window Created Successfully");

	Renderer renderer;
	if (!renderer.Init()) return -1;
	if (!renderer.OnResize(WIDTH, HEIGHT, window)) return -1;

	// Start
	auto startTime = high_resolution_clock::now();
	auto lastTime = high_resolution_clock::now();

	ID3D11Buffer * vBuffer, * iBuffer;
	HRESULT hr;
	// VBuffer
	{
		float verts[] = {
			-0.5, -0.5, 0.0,	 0.0, 0.0,
			-0.5,  0.5, 0.0,	 0.0, 1.0,
			 0.5,  0.5, 0.0,	 1.0, 1.0,
			 0.5, -0.5, 0.0,	 1.0, 0.0
		};

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(float) * 5 * 4;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = NULL;
		vbd.MiscFlags = NULL;
		vbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = verts;

		vBuffer = renderer.CreateBuffer(&vbd, &data);
	}
	// IBuffer
	{
		unsigned int indices[] = {
			0, 1, 2,
			0, 2, 3
		};

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(unsigned int) * 6;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = NULL;
		ibd.MiscFlags = NULL;
		ibd.StructureByteStride = NULL;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = indices;

		iBuffer = renderer.CreateBuffer(&ibd, &data);
	}
	// Shaders

	UINT compileFlags = 0;
#if DEBUG || _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif // DEBUG || _DEBUG

	ID3DBlob * vsBlob;
	ID3DBlob * psBlob;
	ID3DBlob * shaderError;

	hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "vert", "vs_5_0", compileFlags, NULL, &vsBlob, &shaderError);
	if (FAILED(hr))
		puts((char *)shaderError->GetBufferPointer());

	hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "pixel", "ps_5_0", compileFlags, NULL, &psBlob, &shaderError);
	if (FAILED(hr))
		puts((char *)shaderError->GetBufferPointer());

	vShader = renderer.CreateShader<ID3D11VertexShader>(vsBlob);
	pShader = renderer.CreateShader<ID3D11PixelShader>(psBlob);

	D3D11_INPUT_ELEMENT_DESC ieDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	inputLayout = renderer.CreateInputLayout(ieDescs, 2, vsBlob);

	if(vsBlob)vsBlob->Release();
	if(psBlob)psBlob->Release();

	// Misc
	queue<float> framerateBuffer;

	// Run

	while (isRunning)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			renderer.ClearDepthStencil(1.0f, 0);
			renderer.ClearRenderTarget(DirectX::Colors::Black);

			auto thisTime = high_resolution_clock::now();
			duration<float> delta = thisTime - lastTime;
			lastTime = thisTime;
			float dt = delta.count();

			framerateBuffer.push(dt);
			//	-	-	Begin Drawing
			UINT strides[] = { sizeof(float) * 5 };
			UINT offsets[] = { 0 };

			renderer.SetInputLayout(inputLayout);

			renderer.SetShader(vShader);
			renderer.SetShader(pShader);

			renderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			renderer.SetVertexBuffers(0, 1, &vBuffer, strides, offsets);
			renderer.SetIndexBuffer(iBuffer, DXGI_FORMAT_R32_UINT, 0);

			renderer.DrawIndexed(6, 0, 0);

			//	-	-	End Drawing
			renderer.SwapBuffers();
			//printf("\rDrawing. FrameTime = %.0f\t", 1.0f / AverageFramerate(framerateBuffer));
		}
	}
	puts("");
	return 0;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	msg = { window, message, wparam, lparam };
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		isRunning = false;
		return 0;
	}
	return DefWindowProc(window, message, wparam, lparam);
}