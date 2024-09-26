#include<Header.hlsli>
void main(
    in float4 i_pos : SV_POSITION,
    in float2 i_uv : TEXCOORD,
    out float4 o_col : SV_TARGET)
{
    o_col = float4(Texture.Sample(Sampler, i_uv)) * Diffuse;
}
