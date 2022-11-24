#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

uniform mat4 model;

void main()
{
    TexCoords = vertex.zw;
    gl_Position = model * vec4(vertex.xy, 1.0, 1.0);

}