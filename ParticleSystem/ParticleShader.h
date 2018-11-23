#pragma once
#include <d3d11.h>
#include "Renderer.h"
#include <DirectXMath.h>

using namespace DirectX;

struct Pass
{
	ID3D11RasterizerState * rasterizerState;
	ID3D11DepthStencilState * depthStencilState;
	ID3D11BlendState * blendState;

	ID3D11VertexShader * vShader;
	ID3D11PixelShader * pShader;
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

class ParticleShader
{
private:
	enum CBuffers
	{
		CB_Application,
		CB_PerFrame,
		NumCBuffers
	};
private:
	ID3D11InputLayout * inputLayout;
	ID3D11Buffer * CBuffers[NumCBuffers];
	Pass pass;

public:
	ParticleShader();
	~ParticleShader();

	void Init(Renderer & renderer);
	void UploadApplicationCBuffer(Renderer &renderer, CBuffer::Application &data);
	void UploadPerFrameCBuffer(Renderer &renderer, CBuffer::PerFrame &data);
	void ApplyPass(Renderer & renderer);
};

