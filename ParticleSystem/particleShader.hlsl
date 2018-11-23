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

float4x4 CalcLookAt(float3 eye, float3 target, float3 up)
{
	float3 forward = normalize(target - eye);
	float3 right = normalize(cross(up, forward));
	up = normalize(cross(forward, right));
	float3 trans = float3(
		-dot(right, eye), -dot(up, eye), -dot(forward, eye));
	return float4x4(
		float4(right, 0.0), 
		float4(up, 0.0), 
		float4(forward, 0.0), 
		float4(trans, 1.0));
}

//	-	-  Pipeline Stage Functions	-	-
O_Vertex vert(I_Vertex iv, I_Instance ii, uint instanceID : SV_InstanceID)
{
	O_Vertex o;
	
	float4 pos = float4(iv.position, 1.0);

	float4x4 look = CalcLookAt(pos.xyz + ii.position, float3(0.0, 0.0, 0.0), float3(0.0, 1.0, 0.0));
	look = transpose(look);

	//pos = mul(pos, look);
	//float4x4 wvp = mul(look, gViewProjection);
	pos = mul(pos, look);
	pos = mul(pos, gViewProjection);
	o.position = pos;
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
	return float4(i.color.xyz, circle * 0.5);
}