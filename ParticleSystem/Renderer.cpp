#include "Renderer.h"
#include <stdio.h>

Renderer::Renderer()
{
}


Renderer::~Renderer()
{
	// if (inputLayout)inputLayout->Release();
	if (rtv)rtv->Release();
	if (dsv)dsv->Release();
	if (swapChain)swapChain->Release();
	if (context)context->Release();
	if (device)device->Release();
}

bool Renderer::Init()
{
	UINT createflags = 0;
#if DEBUG || _DEBUG
	createflags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createflags, 0, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
	if (FAILED(hr)) return false;

	hr = device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality);
	if (FAILED(hr))
		return false;

	puts("Device Created Successfully");
	return true;
}

bool Renderer::OnResize(int width, int height, HWND & window)
{
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
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
	HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (FAILED(hr))return false;

	IDXGIAdapter * adapter = nullptr;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter);
	if (FAILED(hr)) return false;

	IDXGIFactory * factory = nullptr;
	hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(hr)) return false;
	hr = factory->CreateSwapChain(device, &sd, &swapChain);
	if (FAILED(hr)) return false;
	hr = factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) return false;
	
	dxgiDevice->Release();
	adapter->Release();
	factory->Release();

	puts("Swap Chain Created Successfully");

	ID3D11Texture2D * backBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	if (FAILED(hr)) return false;

	hr = device->CreateRenderTargetView(backBuffer, 0, &rtv);
	if (FAILED(hr)) return false;
	
	backBuffer->Release();

	puts("Back Buffer Created Successfully");

	D3D11_TEXTURE2D_DESC dsd;
	dsd.Width = width;
	dsd.Height = height;
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
	if (FAILED(hr)) return false;

	hr = device->CreateDepthStencilView(depthStencilBuffer, 0, &dsv);
	if (FAILED(hr)) return false;

	depthStencilBuffer->Release();

	context->OMSetRenderTargets(1, &rtv, dsv);

	puts("Render Targets Created Successfully");

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	context->RSSetViewports(1, &vp);
	return true;
}

ID3D11Buffer * Renderer::CreateBuffer(D3D11_BUFFER_DESC * desc, D3D11_SUBRESOURCE_DATA * data)
{
	ID3D11Buffer * buffer = nullptr;
	HRESULT hr = device->CreateBuffer(desc, data, &buffer);
	if (FAILED(hr))
	{
		_com_error err(hr);
		printf("Error Creating Buffer: %s", err.ErrorMessage());
	}
	return buffer;
}

ID3D11InputLayout * Renderer::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC * descs, int count, ID3DBlob * shaderBytecode)
{
	ID3D11InputLayout * layout;
	device->CreateInputLayout(descs, count, shaderBytecode->GetBufferPointer(), shaderBytecode->GetBufferSize(), &layout);
	return layout;
}

void Renderer::ClearDepthStencil(float depth, UINT8 stencil)
{
	context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void Renderer::ClearRenderTarget(const float color[4])
{
	context->ClearRenderTargetView(rtv, color);
}

void Renderer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	context->IASetPrimitiveTopology(topology);
}

void Renderer::SetInputLayout(ID3D11InputLayout * inputLayout)
{
	context->IASetInputLayout(inputLayout);
}

void Renderer::SetVertexBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer * const * buffers, const UINT * strides, const UINT * offsets)
{
	context->IASetVertexBuffers(startSlot, numBuffers, buffers, strides, offsets);
}

void Renderer::SetIndexBuffer(ID3D11Buffer * buffer, DXGI_FORMAT format, UINT offset)
{
	context->IASetIndexBuffer(buffer, format, offset);
}

void Renderer::DrawIndexed(UINT indexCount, UINT startIndex, int startVertex)
{
	context->DrawIndexed(indexCount, startIndex, startVertex);
}

void Renderer::DrawIndexedInstanced(UINT indexCountPerObject, UINT instanceCount, UINT startIndex, int startVertex, UINT startInstance)
{
	context->DrawIndexedInstanced(indexCountPerObject, instanceCount, startIndex, startVertex, startInstance);
}

void Renderer::SwapBuffers()
{
	swapChain->Present(presentSyncInterval, 0);
}

ID3D11DeviceContext * Renderer::GetContext() const
{
	return context;
}

ID3D11Device * Renderer::GetDevice() const
{
	return device;
}
