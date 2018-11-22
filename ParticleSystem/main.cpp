#include <Windows.h>
#include <chrono>
#include <stdio.h>
#include <queue>
#include "Renderer.h"
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <comdef.h>
#include <random>

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

const int WIDTH = 800, HEIGHT = 800;
const int MAX_FRAMERATE_TIMES = 100;

bool isRunning = true;

MSG msg;

ID3D11InputLayout * inputLayout;
ID3D11VertexShader * vShader;
ID3D11PixelShader * pShader;

ID3D11Buffer * CBuffers[NumCBuffers];

ID3D11RasterizerState * rasterizerState;
ID3D11DepthStencilState * depthStencilState;
ID3D11BlendState * blendState;

UINT numParticles = 100;

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

void Cleanup(void)
{
	if (inputLayout)inputLayout->Release();
	if (vShader)vShader->Release();
	if (pShader)pShader->Release();
	for (size_t i = 0; i < NumCBuffers; i++)
		if (CBuffers[i])CBuffers[i]->Release();
#if DEBUG || _DEBUG
	//system("PAUSE");
#endif // DEBUG || _DEBUG
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

	ID3D11Buffer * vertexBuffers[2], * indexBuffer;
	HRESULT hr;
	// VBuffer
	{
		Vertex verts[] = {
			{XMFLOAT3(-0.5, -0.5, 0.0), XMFLOAT2(0.0, 0.0)},
			{XMFLOAT3(-0.5,  0.5, 0.0), XMFLOAT2(0.0, 1.0)},
			{XMFLOAT3 (0.5,  0.5, 0.0), XMFLOAT2(1.0, 1.0)},
			{XMFLOAT3( 0.5, -0.5, 0.0), XMFLOAT2(1.0, 0.0)}
		};

		Instance *instances = new Instance[numParticles];
		std::mt19937 mt(static_cast<unsigned int>(startTime.time_since_epoch().count()));
		std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
		for (size_t i = 0; i < numParticles; i++)
		{
			XMVECTOR dir = XMVector3Normalize(XMVectorSet(dist(mt), dist(mt), dist(mt), 0.0f));
			dir = XMVectorScale(dir, dist(mt));
			XMStoreFloat3(&instances[i].position, dir);
			instances[i].color = XMFLOAT4(dist(mt), dist(mt), dist(mt), 1.0f);
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * 4;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_DYNAMIC;
		ibd.ByteWidth = sizeof(Instance) * numParticles;
		ibd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vertexSubresourceData;
		vertexSubresourceData.pSysMem = verts;

		D3D11_SUBRESOURCE_DATA instanceSubresourceData;
		instanceSubresourceData.pSysMem = instances;

		vertexBuffers[0] = renderer.CreateBuffer(&vbd, &vertexSubresourceData);
		vertexBuffers[1] = renderer.CreateBuffer(&ibd, &instanceSubresourceData);
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
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = indices;

		indexBuffer = renderer.CreateBuffer(&ibd, &data);
	}
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

	if(vsBlob)vsBlob->Release();
	if(psBlob)psBlob->Release();

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

	XMFLOAT3 eyePos = XMFLOAT3(0.0f, 0.0f, -5.0f);
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
			UINT strides[] = { sizeof(Vertex), sizeof(Instance) };
			UINT offsets[] = { 0 , 0 };

			renderer.SetInputLayout(inputLayout);

			renderer.SetShader(vShader);
			renderer.SetShader(pShader);

			renderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			renderer.SetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
			renderer.SetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

			//renderer.DrawIndexed(6, 0, 0);
			renderer.DrawIndexedInstanced(6, numParticles, 0, 0, 0);

			//	-	-	End Drawing
			renderer.SwapBuffers();
			printf("\rDrawing. Framerate = %.0f,\t Last Frame: %.4f,\t Time: %.1f", 1.0f / CalcAvgFramerate(framerateBuffer), delta.count(), appTime.count());
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