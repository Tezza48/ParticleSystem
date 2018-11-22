#include "ParticleEmitter.h"
#include <ctime>
#include <random>

ParticleEmitter::ParticleEmitter()
{
	perVertexBuffer = nullptr;
	perInstanceBuffer = nullptr;
	indexBuffer = nullptr;
}


ParticleEmitter::~ParticleEmitter()
{
	if (perVertexBuffer)perVertexBuffer->Release();
	if (perInstanceBuffer)perInstanceBuffer->Release();
	if (indexBuffer)indexBuffer->Release();
}

// Create Buffers and Input Layout
bool ParticleEmitter::Init(Renderer & renderer)
{
	//TODO: Add Error Checking and comments
	// vertices for quad
	struct VertexPositionTexture verts[] = {
		{XMFLOAT3(-0.5, -0.5, 0.0), XMFLOAT2(0.0, 0.0)},
		{XMFLOAT3(-0.5,  0.5, 0.0), XMFLOAT2(0.0, 1.0)},
		{XMFLOAT3(0.5,  0.5, 0.0), XMFLOAT2(1.0, 1.0)},
		{XMFLOAT3(0.5, -0.5, 0.0), XMFLOAT2(1.0, 0.0)}
	};

	// Initizlize Particles on surface of a sphere
	InstancePositionColor *instances = new InstancePositionColor[numInstances];
	std::time_t seed = std::time(nullptr);
	std::mt19937 mt(seed);
	std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
	for (size_t i = 0; i < numInstances; i++)
	{
		XMVECTOR dir = XMVector3Normalize(XMVectorSet(dist(mt), dist(mt), dist(mt), 0.0f));
		dir = XMVectorScale(dir, dist(mt));
		XMStoreFloat3(&instances[i].position, dir);
		instances[i].color = XMFLOAT4(dist(mt), dist(mt), dist(mt), 1.0f);
	}

	// write once and leave
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = sizeof(VertexPositionTexture) * 4;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// write once and we can edit it later
	D3D11_BUFFER_DESC instanceBufferDesc;
	instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	instanceBufferDesc.ByteWidth = sizeof(InstancePositionColor) * numInstances;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexSubresourceData;
	vertexSubresourceData.pSysMem = verts;

	D3D11_SUBRESOURCE_DATA instanceSubresourceData;
	instanceSubresourceData.pSysMem = instances;

	// Create the buffers
	vertexBuffers[0] = renderer.CreateBuffer(&vertexBufferDesc, &vertexSubresourceData);
	vertexBuffers[1] = renderer.CreateBuffer(&instanceBufferDesc, &instanceSubresourceData);

	// indices for quad
	unsigned int indices[] = {
	0, 1, 2,
	0, 2, 3
	};

	// write once like per vertex buffer
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

	return true;
}

void ParticleEmitter::Draw(Renderer & renderer)
{
	UINT strides[] = { sizeof(VertexPositionTexture), sizeof(InstancePositionColor) };
	UINT offsets[] = { 0 , 0 };

	renderer.SetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	renderer.SetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	renderer.DrawIndexedInstanced(numIndices, numInstances, 0, 0, 0);
}
