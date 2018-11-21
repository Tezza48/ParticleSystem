#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <comdef.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

class Renderer
{
private:

	ID3D11Device * device;
	ID3D11DeviceContext * context;
	IDXGISwapChain * swapChain;

	ID3D11RenderTargetView * rtv;
	ID3D11DepthStencilView * dsv;

	UINT msaaQuality;
	UINT presentSyncInterval = 2;

	const int MAX_FRAMERATE_TIMES = 100;

public:
	Renderer();
	~Renderer();

	bool Init();
	bool OnResize(int width, int height, HWND & window);

	ID3D11Buffer * CreateBuffer(D3D11_BUFFER_DESC * desc, D3D11_SUBRESOURCE_DATA * data);
	ID3D11InputLayout * CreateInputLayout(D3D11_INPUT_ELEMENT_DESC * descs, int count, ID3DBlob * shaderBytecode);

	template<typename T> T * CreateShader(ID3DBlob * compiledShader);

	void ClearDepthStencil(float depth, UINT8 stencil);
	void ClearRenderTarget(const float color[4]);

	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
	void SetInputLayout(ID3D11InputLayout * inputLayout);
	void SetVertexBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer * const * buffers, const UINT * strides, const UINT * offsets);
	void SetIndexBuffer(ID3D11Buffer * buffer, DXGI_FORMAT format, UINT offset);
	template<typename T> void SetShader(T * shader);
	void DrawIndexed(UINT numElements, UINT startIndex, int startVertex);
	void SwapBuffers();

	ID3D11DeviceContext * GetContext() const;
	ID3D11Device * GetDevice() const;
};

template<typename T>
inline T * Renderer::CreateShader(ID3DBlob * compiledShader)
{
	return NULL;
}

template<>
inline ID3D11VertexShader * Renderer::CreateShader<ID3D11VertexShader>(ID3DBlob * compiledShader)
{
	ID3D11VertexShader * shader = nullptr;
	HRESULT hr = device->CreateVertexShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), NULL, &shader);
	if (FAILED(hr))
	{
		_com_error err(hr);
		printf("Error Creating Vertex Shader: %s", err.ErrorMessage());
	}
	return shader;
}

template<>
inline ID3D11PixelShader * Renderer::CreateShader<ID3D11PixelShader>(ID3DBlob * compiledShader)
{
	ID3D11PixelShader * shader = nullptr;
	HRESULT hr = device->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), NULL, &shader);
	if (FAILED(hr))
	{
		_com_error err(hr);
		printf("Error Creating Pixel Shader: %s", err.ErrorMessage());
	}
	return shader;
}
template<typename T>
inline void Renderer::SetShader(T * shader)
{
}

template<>
inline void Renderer::SetShader<ID3D11VertexShader>(ID3D11VertexShader * shader)
{
	context->VSSetShader(shader, NULL, 0);
}

template<>
inline void Renderer::SetShader<ID3D11PixelShader>(ID3D11PixelShader * shader)
{
	context->PSSetShader(shader, NULL, 0);
}