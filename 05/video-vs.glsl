#version 450 core

uniform vec2 scale;
out vec2 tex_coords;

void main()
{
    float x     = float((gl_VertexID & 1) * 2) - 1.0;
    float y     = float((gl_VertexID & 2)) - 1.0;
    tex_coords  = vec2(x, -y) * 0.5 + 0.5;
    gl_Position = vec4(x * scale.x, y * scale.y, 0.0, 1.0);
}
