#include <chrono>
#include <comdef.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <queue>
#include <random>
#include <stdio.h>
#include "ParticleEmitter.h"
#include "Renderer.h"
#include "WindowManager.h"

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

using std::puts;
using std::printf;
using std::queue;
using namespace DirectX;
using namespace std::chrono;

struct Particle
{
	float birthTime;
	XMFLOAT3 position;
	XMFLOAT3 velocity;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 texCoord;
};

struct Instance
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

enum CBuffers
{
	CB_Application,
	CB_PerFrame,
	NumCBuffers
};

struct CBuffer
{
	struct Application
	{
		XMMATRIX viewProjection;
	};
	struct PerFrame
	{
		XMFLOAT4 time;
		XMFLOAT4 color;
	};
};


#if DEBUG || _DEBUG
ID3D11Debug * debug;
#endif // DEBUG || _DEBUG

//WindowManager window;
//Renderer renderer;
const int WIDTH = 800, HEIGHT = 800;
const int MAX_FRAMERATE_TIMES = 100;

bool isRunning = true;

MSG msg;


UINT numParticles = 100;

float CalcAvgFramerate(queue<float> & buffer);

void Cleanup(void)
{
#if DEBUG || _DEBUG
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	debug->Release();
	debug = nullptr;
#endif // DEBUG || _DEBUG
}

int main(int argc, char ** argv)
{
	atexit(::Cleanup);

	ID3D11InputLayout * inputLayout;
	ID3D11VertexShader * vShader;
	ID3D11PixelShader * pShader;

	ID3D11Buffer * CBuffers[NumCBuffers];

	ID3D11RasterizerState * rasterizerState;
	ID3D11DepthStencilState * depthStencilState;
	ID3D11BlendState * blendState;

	// Create Win32 Window
	WindowManager window = WindowManager();
	if (!window.Init(WIDTH, HEIGHT, ::WindowProc))
		return false;
	puts("Window Created Successfully");

	// Init D3D
	Renderer renderer;
	if (!renderer.Init()) return -1;
	if (!renderer.OnResize(WIDTH, HEIGHT, window.GetHandle())) return -1;

	HRESULT hr;
#if DEBUG || _DEBUG
	// 
	hr = renderer.GetDevice()->QueryInterface(IID_PPV_ARGS(&debug));
	if (FAILED(hr)) return false;
#endif

	// Start
	auto startTime = high_resolution_clock::now();
	auto lastTime = high_resolution_clock::now();

	ParticleEmitter emitter;
	emitter.Init(renderer);
	
	// CBuffers
	// Application
	{
		CBuffer::Application buffer {
			XMMatrixIdentity()
		};

		D3D11_BUFFER_DESC desc;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(CBuffer::Application);
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = &buffer;

		CBuffers[CB_Application] = renderer.CreateBuffer(&desc, &data);
	}
	// Frame
	{
		CBuffer::PerFrame buffer;
		buffer.time = XMFLOAT4();
		buffer.color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

		D3D11_BUFFER_DESC desc;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(CBuffer::PerFrame);
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = NULL;
		desc.StructureByteStride = NULL;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = &buffer;

		CBuffers[CB_PerFrame] = renderer.CreateBuffer(&desc, &data);
	}

	// Shaders

	{
		UINT compileFlags = 0;
#if DEBUG || _DEBUG
		compileFlags |= D3DCOMPILE_DEBUG;
#endif // DEBUG || _DEBUG
		ID3DBlob * vsBlob = nullptr;
		ID3DBlob * psBlob = nullptr;
		ID3DBlob * shaderError = nullptr;

		hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "vert", "vs_5_0", compileFlags, NULL, &vsBlob, &shaderError);
		if (FAILED(hr))
			puts((char *)shaderError->GetBufferPointer());

		hr = D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "pixel", "ps_5_0", compileFlags, NULL, &psBlob, &shaderError);
		if (FAILED(hr))
			puts((char *)shaderError->GetBufferPointer());

		if (shaderError)shaderError->Release();
		shaderError = nullptr;

		vShader = renderer.CreateShader<ID3D11VertexShader>(vsBlob);
		pShader = renderer.CreateShader<ID3D11PixelShader>(psBlob);

		D3D11_INPUT_ELEMENT_DESC ieDescs[] = {
			// Vertex Buffer
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

			// Instance Buffer
			{"POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},

			//{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			//{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			//{"TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		inputLayout = renderer.CreateInputLayout(ieDescs, 4, vsBlob);

		if (vsBlob)vsBlob->Release();
		vsBlob = nullptr;
		if (psBlob)psBlob->Release();
		psBlob = nullptr;
	}
	// Misc
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;

	renderer.GetDevice()->CreateRasterizerState(&rasterizerDesc, &rasterizerState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = false;

	renderer.GetDevice()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.RenderTarget->BlendEnable = true;
	blendDesc.RenderTarget->SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget->DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget->BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget->SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget->DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget->BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	renderer.GetDevice()->CreateBlendState(&blendDesc, &blendState);

	queue<float> framerateBuffer;

	float cameraDistance = 5.0f;

	XMFLOAT3 eyePos = XMFLOAT3(0.0f, 1.6f, -3.0f);
	XMFLOAT3 target = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&target), XMLoadFloat3(&up));
	XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.01f, 100.0f);

	XMMATRIX vp = view * projection;

	D3D11_MAPPED_SUBRESOURCE appBuffer;
	renderer.GetContext()->Map(CBuffers[CB_Application], 0, D3D11_MAP_WRITE_DISCARD, 0, &appBuffer);

	CBuffer::Application * buffer = (CBuffer::Application *)appBuffer.pData;
	buffer->viewProjection = XMMatrixTranspose(vp);

	renderer.GetContext()->Unmap(CBuffers[CB_Application], 0);
	buffer = nullptr;

	renderer.GetContext()->VSSetConstantBuffers(0, 1, &CBuffers[CB_Application]);
	renderer.GetContext()->PSSetConstantBuffers(0, 1, &CBuffers[CB_Application]);

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

			renderer.GetContext()->RSSetState(rasterizerState);
			renderer.GetContext()->OMSetBlendState(blendState, 0, 0xffffffff);
			renderer.GetContext()->OMSetDepthStencilState(depthStencilState, 0);

			auto thisTime = high_resolution_clock::now();
			duration<float> delta = thisTime - lastTime;
			lastTime = thisTime;
			float dt = delta.count();

			framerateBuffer.push(dt);

			// Update PerFrame CBuffer
			duration<float> appTime = thisTime - startTime;
			
			D3D11_MAPPED_SUBRESOURCE perFrameMappedSubresource;
			hr = renderer.GetContext()->Map(CBuffers[CB_PerFrame], 0, D3D11_MAP_WRITE_DISCARD, 0, &perFrameMappedSubresource);
			if (FAILED(hr))
			{
				_com_error err(hr);
				printf("Error Mapping Per Frame CBuffer: %s", err.ErrorMessage());
			}
			CBuffer::PerFrame * perFrameBuffer = (CBuffer::PerFrame *)perFrameMappedSubresource.pData;
			perFrameBuffer->time = XMFLOAT4(appTime.count(), dt, 0.0f, 1.0f);
			XMStoreFloat4(&perFrameBuffer->color, DirectX::Colors::CornflowerBlue);

			renderer.GetContext()->Unmap(CBuffers[CB_PerFrame], 0);

			renderer.GetContext()->VSSetConstantBuffers(1, 1, &CBuffers[CB_PerFrame]);
			renderer.GetContext()->PSSetConstantBuffers(1, 1, &CBuffers[CB_PerFrame]);

			//	-	-	Begin Drawing

			renderer.SetShader(vShader);
			renderer.SetShader(pShader);

			renderer.SetInputLayout(inputLayout);

			renderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//renderer.SetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
			//renderer.SetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			emitter.Draw(renderer);

			//renderer.DrawIndexed(6, 0, 0);
			//renderer.DrawIndexedInstanced(6, numParticles, 0, 0, 0);

			renderer.SwapBuffers();
			//	-	-	End Drawing
			printf("\rDrawing. Framerate = %.0f,\t Last Frame: %.4f,\t Time: %.1f", 1.0f / CalcAvgFramerate(framerateBuffer), delta.count(), appTime.count());
		}
	}
	puts("");
	if (inputLayout)inputLayout->Release();
	inputLayout = nullptr;
	if (vShader)vShader->Release();		 
	vShader = nullptr;
	if (pShader)pShader->Release();
	pShader = nullptr;
	for (size_t i = 0; i < NumCBuffers; i++)
	{
		if (CBuffers[i])CBuffers[i]->Release();
		CBuffers[i] = nullptr;
	}
	if (rasterizerState)rasterizerState->Release();
	rasterizerState = nullptr;
	if (depthStencilState)depthStencilState->Release();
	depthStencilState = nullptr;
	if (blendState)blendState->Release();
	blendState = nullptr;
	return 0;
}

float CalcAvgFramerate(queue<float> & buffer)
{
	// pop until <= MAX_FRAMERATE_TIMES
	while (buffer.size() > MAX_FRAMERATE_TIMES)
		buffer.pop();
	float average = 0;
	for (size_t i = 0; i < buffer.size(); i++)
	{
		average += buffer._Get_container()[i];
	}
	return average / buffer.size();
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	msg = { window, message, wparam, lparam };
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			DestroyWindow(window);
			return 0;
		default:
			return 0;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		isRunning = false;
		return 0;
	}
	return DefWindowProc(window, message, wparam, lparam);
}