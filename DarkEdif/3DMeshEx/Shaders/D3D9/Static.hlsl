float4x4 wvp;
float4x4 wNormal;
float4 screenRes;

struct VS_INPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    
    float4 boneIDs : BINORMAL0;
    float4 weights : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    
    // Since the POSITION semantic isn't readable in a pixel shader, gotta put it in another semantic :(
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
    // Fixes the "half-pixel offset" quirk that D3D9 has.. unfortunately
    output.position.xy += float2(-0.5f / screenRes.x, 0.5f / screenRes.y) * input.position.ww;
    return output;
}
