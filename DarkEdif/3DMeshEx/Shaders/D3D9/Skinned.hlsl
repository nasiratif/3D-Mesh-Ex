#define BONE_TEX_SIZE 64

sampler2D deformMatrices : register(s0);

float4x4 wvp;
float4x4 wNormal;
float2 screenRes;

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
    
    float4 vertPos : COLOR0;
};

float4x4 SampleDeformMatrix(int id)
{
    id *= 4;
    int2 pos = int2(
        id % BONE_TEX_SIZE,
        id / BONE_TEX_SIZE
    );
    float2 uv = float2(
        (float)pos.x / BONE_TEX_SIZE,
        (float)pos.y / BONE_TEX_SIZE
    );
    float uvStep = 1.0f / BONE_TEX_SIZE;
    
    float4 rowX = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowY = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowZ = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowW = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    
    float4x4 ret =
    {
        rowX.x, rowX.y, rowX.z, rowX.w,
        rowY.x, rowY.y, rowY.z, rowY.w,
        rowZ.x, rowZ.y, rowZ.z, rowZ.w,
        rowW.x, rowW.y, rowW.z, rowW.w,
    };
    return ret;
}

float4x4 SampleDeformNormalMatrix(int id)
{
    id *= 4;
    int2 pos = int2(
        id % BONE_TEX_SIZE,
        (id / BONE_TEX_SIZE) + (BONE_TEX_SIZE / 2)
    );
    float2 uv = float2(
        (float) pos.x / BONE_TEX_SIZE,
        (float) pos.y / BONE_TEX_SIZE
    );
    float uvStep = 1.0f / BONE_TEX_SIZE;
    
    float4 rowX = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowY = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowZ = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    uv.x += uvStep;
    float4 rowW = tex2Dlod(deformMatrices, float4(uv, 0.0f, 0));
    
    float4x4 ret =
    {
        rowX.x, rowX.y, rowX.z, rowX.w,
        rowY.x, rowY.y, rowY.z, rowY.w,
        rowZ.x, rowZ.y, rowZ.z, rowZ.w,
        rowW.x, rowW.y, rowW.z, rowW.w,
    };
    return ret;
}

VS_OUTPUT main(in VS_INPUT input)
{
    VS_OUTPUT output;
    float4 position = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        int boneID = (int)input.boneIDs[i];
        if (boneID < 0)
            break;
        
        float weight = input.weights[i];
        position += mul(SampleDeformMatrix(boneID), input.position) * weight;
        normal += mul(SampleDeformNormalMatrix(boneID), float4(input.normal, 1.0f)) * weight;
    }
    
    output.position = mul(wvp, position);
    output.position.y = -output.position.y;
    output.normal = mul(wNormal, normal).xyz;
    output.texcoord = input.texcoord;
    output.texcoord.y = 1.0 - output.texcoord.y;
    
    output.vertPos = output.position;
    output.position.xy += float2(-0.5f / screenRes.x, 0.5f / screenRes.y) * input.position.ww;
    return output;
}
