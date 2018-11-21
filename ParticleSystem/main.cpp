#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <chrono>
#include <stdio.h>
#include <DirectXColors.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

bool isRunning = true;

MSG msg;
ID3D11Device * device;
ID3D11DeviceContext * context;
IDXGISwapChain * swapChain;

ID3D11InputLayout * inputLayout;

ID3D11VertexShader * vShader;
ID3D11PixelShader * pShader;

ID3D11RenderTargetView * rtv;
ID3D11DepthStencilView * dsv;

using std::puts;
using std::printf;
using namespace std::chrono;

UINT msaaQuality;

const int WIDTH = 800, HEIGHT = 800;

void Cleanup(void)
{
	if(inputLayout)inputLayout->Release();
	if(rtv)rtv->Release();
	if(dsv)dsv->Release();
	if(swapChain)swapChain->Release();
	if(context)context->Release();
	if(device)device->Release();

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

	UINT createflags = 0;
	#if DEBUG || _DEBUG
	createflags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createflags, 0, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
	if (FAILED(hr))
	{
		puts("\nCreating Device Failed\n");
		return -1;
	}

	hr = device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality);
	if (FAILED(hr))
	{
		puts("\nChecking MSAA Failed\n");
		return -1;
	}

	puts("Device Created Successfully");

	// OnResize Stuff
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = WIDTH;
	sd.BufferDesc.Height = HEIGHT;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 4;
	sd.SampleDesc.Quality = msaaQuality - 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = window;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = NULL;

	IDXGIDevice * dxgiDevice = nullptr;
	hr  = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (FAILED(hr))
	{
		puts("\nSomething Failed\n");
		return -1;
	}

	IDXGIAdapter * adapter = nullptr;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter);
	if (FAILED(hr))
	{
		puts("\nSomething Failed\n");
		return -1;
	}

	IDXGIFactory * factory = nullptr;
	hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(hr))
	{
		puts("\nSomething Failed\n");
		return -1;
	}

	hr = factory->CreateSwapChain(device, &sd, &swapChain);
	if (FAILED(hr))
	{
		puts("\nSomething Failed\n");
		return -1;
	}
	hr = factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr))
	{
		puts("\nSomething Failed\n");
		return -1;
	}

	dxgiDevice->Release();
	adapter->Release();
	factory->Release();

	puts("Swap Chain Created Successfully");

	ID3D11Texture2D * backBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	if (FAILED(hr))
	{
		puts("Something Failed");
		return -1;
	}
	hr = device->CreateRenderTargetView(backBuffer, 0, &rtv);
	if (FAILED(hr))
	{
		puts("\nCreating RTV Failed\n");
		return -1;
	}
	backBuffer->Release();

	puts("Back Buffer Created Successfully");

	D3D11_TEXTURE2D_DESC dsd;
	dsd.Width = WIDTH;
	dsd.Height = HEIGHT;
	dsd.MipLevels = 1;
	dsd.ArraySize = 1;
	dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsd.SampleDesc.Count = 4;
	dsd.SampleDesc.Quality = msaaQuality - 1;
	dsd.Usage = D3D11_USAGE_DEFAULT;
	dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsd.CPUAccessFlags = 0;
	dsd.MiscFlags = 0;

	ID3D11Texture2D * depthStencilBuffer;
	hr = device->CreateTexture2D(&dsd, 0, &depthStencilBuffer);
	if (FAILED(hr))
	{
		puts("\nCreating Depth Stencil Buffer Failed");
		return -1;
	}
	hr = device->CreateDepthStencilView(depthStencilBuffer, 0, &dsv);
	if (FAILED(hr))
	{
		puts("\nCreating DSV Failed\n");
		return -1;
	}
	depthStencilBuffer->Release();

	context->OMSetRenderTargets(1, &rtv, dsv);

	puts("Render Targets Created Successfully");

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = static_cast<float>(WIDTH);
	vp.Height = static_cast<float>(HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	context->RSSetViewports(1, &vp);

	// Start
	auto startTime = high_resolution_clock::now();
	auto lastTime = high_resolution_clock::now();

	ID3D11Buffer * vBuffer, * iBuffer;

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

		hr = device->CreateBuffer(&vbd, &data, &vBuffer);
		if (FAILED(hr))
		{
			puts("\nCreating Vertex Buffer Failed\n");
			return -1;
		}
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

		hr = device->CreateBuffer(&ibd, &data, &iBuffer);
		if (FAILED(hr))
		{
			puts("\nCreating Index Buffer Failed\n");
			return -1;
		}
	}
	// Shaders

	UINT compileFlags = 0;
#if DEBUG || _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif // DEBUG || _DEBUG

	ID3DBlob * vsBlob;
	ID3DBlob * psBlob;
	hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "vert", "vs_5_0", compileFlags, NULL, &vsBlob, NULL);
	hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "pixel", "ps_5_0", compileFlags, NULL, &psBlob, NULL);

	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vShader);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pShader);

	D3D11_INPUT_ELEMENT_DESC ieDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	device->CreateInputLayout(ieDescs, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

	if(vsBlob)vsBlob->Release();
	if(psBlob)psBlob->Release();

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
			context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
			context->ClearRenderTargetView(rtv, DirectX::Colors::Black);

			auto thisTime = high_resolution_clock::now();
			duration<float> delta = thisTime - lastTime;
			lastTime = thisTime;
			float dt = delta.count();
			//	-	-	Begin Drawing
			UINT strides[] = { sizeof(float) * 5 };
			UINT offsets[] = { 0 };

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->IASetVertexBuffers(0, 1, &vBuffer, strides, offsets);
			context->IASetIndexBuffer(iBuffer, DXGI_FORMAT_R32_UINT, 0);

			context->IASetInputLayout(inputLayout);

			context->VSSetShader(vShader, NULL, 0);
			context->PSSetShader(pShader, NULL, 0);

			context->DrawIndexed(6, 0, 0);

			//	-	-	End Drawing
			hr = swapChain->Present(1, 0);
			if (FAILED(hr))
			{
				puts("\nPresenting Failed\n");
				return -1;
			}
			printf("\rDrawing. FrameTime = %f\t", 1.0f / dt);
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