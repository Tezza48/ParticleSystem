#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Renderer.h"

using namespace DirectX;

struct Particle
{
	float birthTime;
	XMFLOAT3 position;
	XMFLOAT3 velocity;
};

class ParticleEmitter
{
	struct VertexPositionTexture
	{
		XMFLOAT3 position;
		XMFLOAT2 texCoord;
	};
	struct InstancePositionWorldColor
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
		//XMMATRIX world;
	};
private:
	// union means the 2 vertex buffers are either accessable as an array
	// or by separate names, easier to pick individually but they're
	// already together for drawing
	union
	{
		ID3D11Buffer * vertexBuffers[2];
		struct
		{
			ID3D11Buffer * perVertexBuffer;
			ID3D11Buffer * perInstanceBuffer;
		};
	};
	ID3D11Buffer * indexBuffer;

	UINT numVertices = 4;
	UINT numInstances = 100;
	UINT numIndices = 6;
public:
	ParticleEmitter();
	~ParticleEmitter();

	bool Init(Renderer & renderer);
	void Draw(Renderer & renderer);
};

