#version 330 core
out vec4 fragment_color;

in vec3 world_position;

uniform sampler2D equirectangular_texture;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= inv_atan;
  uv += 0.5;
  return uv;
}

void main(){
  vec2 uv = SampleSphericalMap(normalize(world_position));
  vec3 color = texture(equirectangular_texture, uv).rgb;
  fragment_color = vec4(color, 1.0);
}