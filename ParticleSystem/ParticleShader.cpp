#include "ParticleShader.h"
#include "Utils.h"

ParticleShader::ParticleShader()
{
	inputLayout = nullptr;
	pass.vShader = nullptr;
	pass.pShader = nullptr;
	pass.rasterizerState = nullptr;
	pass.depthStencilState = nullptr;
	pass.blendState = nullptr;
}


ParticleShader::~ParticleShader()
{
	SafeRelease(inputLayout);
	SafeRelease(pass.vShader);
	SafeRelease(pass.pShader);
	SafeRelease(pass.rasterizerState);
	SafeRelease(pass.depthStencilState);
	SafeRelease(pass.blendState);
	for (size_t i = 0; i < NumCBuffers; i++)
	{
		SafeRelease(CBuffers[i]);
	}
}

void ParticleShader::Init(Renderer & renderer)
{
	// CBuffers
	CBuffer::Application appBuffer;
	appBuffer.viewProjection = XMMatrixIdentity();

	D3D11_BUFFER_DESC appBufferDesc;
	appBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	appBufferDesc.ByteWidth = sizeof(CBuffer::Application);
	appBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	appBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	appBufferDesc.MiscFlags = 0;
	appBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA appBufferData;
	appBufferData.pSysMem = &appBuffer;

	CBuffers[CB_Application] = renderer.CreateBuffer(&appBufferDesc, &appBufferData);


	CBuffer::PerFrame perFrameBuffer;
	perFrameBuffer.time = XMFLOAT4();
	perFrameBuffer.color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	D3D11_BUFFER_DESC perFrameBufferDesc;
	perFrameBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	perFrameBufferDesc.ByteWidth = sizeof(CBuffer::PerFrame);
	perFrameBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	perFrameBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	perFrameBufferDesc.MiscFlags = NULL;
	perFrameBufferDesc.StructureByteStride = NULL;

	D3D11_SUBRESOURCE_DATA perFrameBufferData;
	perFrameBufferData.pSysMem = &appBuffer;

	CBuffers[CB_PerFrame] = renderer.CreateBuffer(&perFrameBufferDesc, &perFrameBufferData);

	// Shaders
	UINT compileFlags = 0;
#if DEBUG || _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif // DEBUG || _DEBUG
	ID3DBlob * vsBlob = nullptr;
	ID3DBlob * psBlob = nullptr;
	ID3DBlob * shaderError = nullptr;

	HRESULT hr = D3DCompileFromFile(L"particleShader.hlsl", NULL, NULL, "vert", "vs_5_0", compileFlags, NULL, &vsBlob, &shaderError);
	if (FAILED(hr))
		puts((char *)shaderError->GetBufferPointer());

	hr = D3DCompileFromFile(L"particleShader.hlsl", NULL, NULL, "pixel", "ps_5_0", compileFlags, NULL, &psBlob, &shaderError);
	if (FAILED(hr))
		puts((char *)shaderError->GetBufferPointer());

	if (shaderError)shaderError->Release();
	shaderError = nullptr;

	pass.vShader = renderer.CreateShader<ID3D11VertexShader>(vsBlob);
	pass.pShader = renderer.CreateShader<ID3D11PixelShader>(psBlob);

	// InputLayout
	D3D11_INPUT_ELEMENT_DESC ieDescs[] =
	{
		// Vertex Buffer
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

		// Instance Buffer
		{"POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		
		// World MAtrix
		//{"POSITION", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		//{"POSITION", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		//{ "POSITION", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		//{ "POSITION", 5, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	inputLayout = renderer.CreateInputLayout(ieDescs, 4, vsBlob);

	if (vsBlob)vsBlob->Release();
	vsBlob = nullptr;
	if (psBlob)psBlob->Release();
	psBlob = nullptr;

	// State Objects
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;

	renderer.GetDevice()->CreateRasterizerState(&rasterizerDesc, &pass.rasterizerState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = true;

	renderer.GetDevice()->CreateDepthStencilState(&depthStencilDesc, &pass.depthStencilState);

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget->BlendEnable = false;
	blendDesc.RenderTarget->SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget->DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget->BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget->SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget->DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget->BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	renderer.GetDevice()->CreateBlendState(&blendDesc, &pass.blendState);
}

void ParticleShader::UploadApplicationCBuffer(Renderer &renderer, CBuffer::Application & data)
{
	D3D11_MAPPED_SUBRESOURCE appBuffer;
	renderer.GetContext()->Map(CBuffers[CB_Application], 0, D3D11_MAP_WRITE_DISCARD, 0, &appBuffer);

	CBuffer::Application * buffer = (CBuffer::Application *)appBuffer.pData;
	buffer->viewProjection = data.viewProjection;

	renderer.GetContext()->Unmap(CBuffers[CB_Application], 0);

	renderer.GetContext()->VSSetConstantBuffers(0, 1, &CBuffers[CB_Application]);
	renderer.GetContext()->PSSetConstantBuffers(0, 1, &CBuffers[CB_Application]);
}

void ParticleShader::UploadPerFrameCBuffer(Renderer & renderer, CBuffer::PerFrame & data)
{
	D3D11_MAPPED_SUBRESOURCE perFrameMappedSubresource;
	HRESULT hr = renderer.GetContext()->Map(CBuffers[CB_PerFrame], 0, D3D11_MAP_WRITE_DISCARD, 0, &perFrameMappedSubresource);
	if (FAILED(hr))
	{
		_com_error err(hr);
		printf("Error Mapping Per Frame CBuffer: %s", err.ErrorMessage());
	}
	CBuffer::PerFrame * perFrameBuffer = (CBuffer::PerFrame *)perFrameMappedSubresource.pData;
	perFrameBuffer->time = data.time;
	perFrameBuffer->color = data.color;

	renderer.GetContext()->Unmap(CBuffers[CB_PerFrame], 0);

	renderer.GetContext()->VSSetConstantBuffers(1, 1, &CBuffers[CB_PerFrame]);
	renderer.GetContext()->PSSetConstantBuffers(1, 1, &CBuffers[CB_PerFrame]);
}

void ParticleShader::ApplyPass(Renderer & renderer)
{
	renderer.GetContext()->RSSetState(pass.rasterizerState);
	renderer.GetContext()->OMSetBlendState(pass.blendState, 0, 0xffffffff);
	renderer.GetContext()->OMSetDepthStencilState(pass.depthStencilState, 0);

	renderer.SetShader(pass.vShader);
	renderer.SetShader(pass.pShader);

	renderer.SetInputLayout(inputLayout);
}
