#version 330 core
layout (location = 0) in vec3 object_position;
layout (location = 1) in vec2 object_texture_coord;

out vec2 texture_coord;

void main()
{
    texture_coord = object_texture_coord;
	gl_Position = vec4(object_position, 1.0);
}