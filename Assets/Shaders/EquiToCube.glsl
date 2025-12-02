#type vertex
#version 300 es
layout (location = 0) in vec3 a_Position;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    localPos = a_Position;  
    gl_Position = projection * view * vec4(localPos, 1.0);
}

#type fragment
#version 300 es
precision mediump float;
out vec4 FragColor;

in vec3 localPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{       
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}