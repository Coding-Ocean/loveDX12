#include<Header.hlsli>
void main(
    in float4 i_pos : POSITION,
    in float2 i_uv : TEXCOORD,
    out float4 o_pos : SV_POSITION,
    out float2 o_uv : TEXCOORD)
{
    o_pos = mul(WorldViewProj, i_pos);
    o_uv = i_uv;
}
