//	-	-	  Constant Buffers	-	-
cbuffer CB_Application : register(b0)
{
	matrix gViewProjection;
};

cbuffer CB_PerFrame : register(b1)
{
	float4 gTime;
	float4 gColor;
}
//	-	-   In/Output Definitions	-	-
struct I_Vertex
{
	float3 position : POSITION;
	float2 tex : TEXCOORD;
};

struct I_Instance
{
	matrix world;
};

struct O_Vertex
{
	float4 position: SV_POSITION;
	float2 tex : TEXCOORD;
};

//	-	-  Pipeline Stage Functions	-	-
O_Vertex vert(I_Vertex iv, uint instanceID : SV_InstanceID)
{
	O_Vertex o;
	o.position = mul(float4(iv.position, 1.0), gViewProjection);
	o.position.x += instanceID;
	o.tex = iv.tex;
	return o;
}

float4 pixel(O_Vertex i) : SV_TARGET
{
	float2 uv = i.tex * 2.0 - 1.0;

	float size = cos(gTime.x) * 0.45 + 0.55;

	float circle = smoothstep(size, size - 0.1, length(uv));
	return float4(circle, circle, circle, 1.0);
}