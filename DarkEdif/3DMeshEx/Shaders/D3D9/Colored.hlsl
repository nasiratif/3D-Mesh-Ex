float4 ambient;
float4 diffuse;
float4 fogColor;
float3 lightDir;
float fogDist;

struct VS_OUTPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    
    float4 vertPos : COLOR0;
};

float4 main(in VS_OUTPUT input) : SV_TARGET
{
    float4 nDotL = dot(normalize(input.normal), normalize(lightDir));
    float4 light = max(ambient, saturate((nDotL * diffuse)));
    
    float4 destColor = lerp(light, fogColor, smoothstep(0.0f, fogDist, input.vertPos.z));
    return float4(destColor.rgb, 1.0f);
}