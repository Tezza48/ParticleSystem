//	-	-	  Constant Buffers	-	-
cbuffer CB_Application : register(b0)
{
	matrix gViewProjection;
};

cbuffer CB_PerFrame : register(b1)
{
	float gTime;
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
O_Vertex vert(I_Vertex i)
{
	O_Vertex o;
	o.position = float4(i.position, 1.0);
	o.tex = i.tex;
	return o;
}

float4 pixel(O_Vertex i) : SV_TARGET
{
	float2 uv = i.tex * 2.0 - 1.0;
	float circle = smoothstep(1.0, 0.9, length(uv));
	return float4(circle, circle, circle, 1.0);
}