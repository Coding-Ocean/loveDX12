cbuffer b0 : register(b0)
{
    matrix Mat;
}
cbuffer b1 : register(b1)
{
    float4 Diffuse;
}
Texture2D<float4> Texture : register(t0); //�e�N�X�`��0��
SamplerState Sampler : register(s0); //�T���v��0��

