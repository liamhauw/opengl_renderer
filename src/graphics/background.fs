#version 330 core
out vec4 fragment_color;

in vec3 world_position;

uniform samplerCube environment_texture;

void main() {
  vec3 env_color = textureLod(environment_texture, world_position, 0.0).rgb;
  env_color = env_color / (env_color + vec3(1.0));
  env_color = pow(env_color, vec3(1.0 / 2.2));
  fragment_color = vec4(env_color, 1.0);
}