#version 330 core
out vec3 world_position;

layout(location = 0) in vec3 object_position;

uniform mat4 projection;
uniform mat4 view;

void main() {
  world_position = object_position;
  gl_Position = projection * view * vec4(world_position, 1.0);
}