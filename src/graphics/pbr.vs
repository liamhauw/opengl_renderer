#version 330 core

layout (location = 0) in vec3 object_position;
layout (location = 1) in vec3 object_normal;
layout (location = 2) in vec2 object_texture_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 world_position;
out vec3 world_normal;
out vec2 texture_coord;

void main() {
  world_position = vec3(model * vec4(object_position, 1.0));
  world_normal = transpose(inverse(mat3(model))) * object_normal;
  texture_coord = object_texture_coord;

  gl_Position = projection * view * vec4(world_position, 1.0);
}