#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET{
	//return float4(input.uv, 1, 1);
	//return float4(tex.Sample(smp, input.uv));
	return float4((input.uv.x * 6 % 2),input.uv.y,input.uv.x,1);
}