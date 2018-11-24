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
	//float4 worldr0 : POSITION2;
	//float4 worldr1 : POSITION3;
	//float4 worldr2 : POSITION4;
	//float4 worldr3 : POSITION5;
};

struct O_Vertex
{
	float4 position: SV_POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

float4x4 CalcPointAt(float3 eye, float3 target, float3 up)
{
	float3 forward = normalize(eye);
	float3 right = normalize(cross(up, forward));
	float3 newUp = normalize(cross(forward, right));

	return float4x4(
		float4(right, 0.0), 
		float4(newUp, 0.0), 
		float4(forward, 0.0), 
		float4(-eye, 1.0));
}

//	-	-  Pipeline Stage Functions	-	-
O_Vertex vert(I_Vertex iv, I_Instance ii, uint instanceID : SV_InstanceID)
{
	O_Vertex o;
	
	float4 pos = float4(iv.position, 1.0);
	//pos.xyz += ii.position;

	float4x4 look = CalcPointAt(ii.position, float3(0.0, 0.0, 0.0), float3(0.0, 1.0, 0.0));
	//look = transpose(look);

	//////pos = mul(pos, look);
	//float4x4 wvp = mul(look, gViewProjection);
	pos = mul(pos, look);
	pos = mul(pos, gViewProjection);
	o.position = pos;
	o.tex = iv.tex;
	o.color = ii.color;
	return o;
}

float sdBox(float2 p, float2 b)
{
	float2 d = abs(p) - b;
	return length(max(d, float2(0.0, 0.0))) + min(max(d.x, d.y), 0.0);
}

float4 pixel(O_Vertex i) : SV_TARGET
{
	float2 uv = i.tex * 2.0 - 1.0;

	float size = 1.0;
	float2 leafuv = uv;
	leafuv.x -= abs(uv.y) * 0.5;
	leafuv /= 0.5;
	float circle = length(leafuv) - size;

	circle = min(circle, sdBox(uv - float2(0.5, 0.0), float2(0.5, 0.05)));

	if (circle > 0) discard;
	return float4(i.color.xyz, 1);
}