cbuffer props : register(b0)
{
    float4x4 wvp;
    float4x4 wNormal;
};

struct VS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    
    float4 boneIDs : BINORMAL0;
    float4 weights : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;

    float4 vertPos : COLOR0;
};

VS_OUTPUT main(in VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(wvp, input.position);
    output.position.y = -output.position.y;
    output.normal = mul(wNormal, float4(input.normal, 1.0f)).xyz;
    output.texcoord = input.texcoord;
    output.texcoord.y = 1.0 - output.texcoord.y;
    
    output.vertPos = output.position;
    return output;
}
