#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec2 OutUVcoords;
layout(set = 0,binding = 1) uniform sampler2D TextureSampler;

void main() {
    const vec3 LightDirection = {0.4f,-0.6f,0.8f};
    float LightValue = max(0.0f,dot(fragColor,LightDirection));
    outColor = vec4((LightValue + 0.05f)* texture(TextureSampler,OutUVcoords * 4.0f).xyz,1.0f);


}
