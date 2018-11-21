
struct I_Vertex
{
	float3 position : POSITION;
	float2 tex : TEXCOORD;
};

struct O_Vertex
{
	float4 position: SV_POSITION;
	float2 tex : TEXCOORD;
};

O_Vertex vert(I_Vertex i)
{
	O_Vertex o;
	o.position = float4(i.position, 1.0);
	o.tex = i.tex;
	return o;
}

float4 pixel(O_Vertex i) : SV_TARGET
{
	return float4(i.tex, 1.0, 1.0);
}