//	-	-	  Constant Buffers	-	-
cbuffer CB_Application : register(b0)
{
	matrix gViewProjection;
	float4 gGravity;
};

cbuffer CB_PerFrame : register(b1)
{
	float4 gTime;
	float4 gColor;
}
//	-	-   In/Output Definitions	-	-
struct I_Vertex
{
	float3 position : POSITION0;
	float2 tex : TEXCOORD;
};

struct I_Instance
{
	float3 position : POSITION1;
	float4 color : COLOR;
};

struct O_Vertex
{
	float4 position: SV_POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

//	-	-  Pipeline Stage Functions	-	-
O_Vertex vert(I_Vertex iv, I_Instance ii, uint instanceID : SV_InstanceID)
{
	O_Vertex o;
	iv.position += ii.position;
	o.position = mul(float4(iv.position, 1.0), gViewProjection);
	//o.position.x += instanceID;
	o.tex = iv.tex;
	o.color = ii.color;
	return o;
}

float4 pixel(O_Vertex i) : SV_TARGET
{
	float2 uv = i.tex * 2.0 - 1.0;

	float size = 1.0;

	float circle = smoothstep(size, size - 0.1, length(uv));
	//if (circle < 0.001) discard;
	return float4(i.color.xyz, circle * 0.1);
}