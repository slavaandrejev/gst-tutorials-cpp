#version 450 core

in vec2 tex_coords;
out vec4 frag_color;

uniform layout(binding = 0) sampler2D video_frame;

void main()
{
    frag_color = texture(video_frame, tex_coords);
}
