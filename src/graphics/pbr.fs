#version 330 core

out vec4 fragment_color;

in vec3 world_position;
in vec3 world_normal;
in vec2 texture_coord;

uniform sampler2D normal_texture;
uniform sampler2D albedo_texture;
uniform sampler2D metallic_texture;
uniform sampler2D roughness_texture;
uniform sampler2D ao_texture;

uniform samplerCube irradiance_texture;
uniform samplerCube prefilter_texture;
uniform sampler2D brdf_texture;

uniform vec3 light_position[4];
uniform vec3 light_color[4];

uniform vec3 camera_position;

uniform bool punctual_light;
uniform bool image_based_light;

const float kPi = 3.14159265359;

vec3 GetNormalFromMap() {
  vec3 tangent_normal = texture(normal_texture, texture_coord).rgb * 2.0 - 1.0;

  vec3 p1 = dFdx(world_position);
  vec3 p2 = dFdy(world_position);
  vec2 c1 = dFdx(texture_coord);
  vec2 c2 = dFdy(texture_coord);

  vec3 normal = normalize(world_normal);
  vec3 tangent = normalize(p1 * c2.t - p2 * c1.t);
  vec3 bitangent = -normalize(cross(normal, tangent));
  mat3 tbn = mat3(tangent, bitangent, normal);

  return normalize(tbn * tangent_normal);
}

float DistributionGGX(vec3 n, vec3 h, float roughness) {
  float a = roughness * roughness;
  float ndoth = max(dot(n, h), 0.0);

  float num = a * a;
  float denum = (ndoth * ndoth * (a * a - 1.0) + 1.0);
  denum = kPi * denum * denum;

  return num / denum;
}

float GeometrySchlickGGX(float ndotv, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num = ndotv;
  float denum = ndotv * (1.0 - k) + k;

  return num / denum;
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness) {
  float ndotv = max(dot(n, v), 0.0);
  float ndotl = max(dot(n, l), 0.0);
  float ggx2 = GeometrySchlickGGX(ndotv, roughness);
  float ggx1 = GeometrySchlickGGX(ndotl, roughness);

  return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cos_theta, vec3 f0) {
  return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cos_theta, vec3 f0, float roughness) {
  return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

void main() {
  vec3 n = GetNormalFromMap();
  vec3 albedo = pow(texture(albedo_texture, texture_coord).rgb, vec3(2.2));
  float metallic = texture(metallic_texture, texture_coord).r;
  float roughness = texture(roughness_texture, texture_coord).r;
  float ao = texture(ao_texture, texture_coord).r;

  vec3 v = normalize(camera_position - world_position);
  vec3 r = reflect(-v, n);

  vec3 f0 = mix(vec3(0.04), albedo, metallic);

  vec3 lo = vec3(0.0);

  if (punctual_light) {
    for(int i = 0; i < 4; ++i) {
      vec3 l = normalize(light_position[i] - world_position);
      vec3 h = normalize(v + l);
      float distance = length(light_position[i] - world_position);
      float attenuation = 1.0 / (distance * distance);
      vec3 radiance = light_color[i] * attenuation;

      float d = DistributionGGX(n, h, roughness);
      float g = GeometrySmith(n, v, l, roughness);
      vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

      vec3 numerator = d * g * f;
      float denumerator = 4 * max(dot(n, v), 0.0 * max(dot(n, l), 0.0)) + 0.0001;
      vec3 specular = numerator / denumerator;

      vec3 ks = f;
      vec3 kd = vec3(1.0) - ks;
      kd *= 1.0 - metallic;

      float ndotl = max(dot(n, l), 0.0);
      lo += (kd * albedo / kPi + specular) * radiance * ndotl;
    }
  }

  vec3 ambient = vec3(0.0);
  if (image_based_light) {
    vec3 f = FresnelSchlickRoughness(max(dot(n, v), 0.0), f0, roughness);
    vec3 ks = f;
    vec3 kd = vec3(1.0) - ks;
    kd *= 1.0 - metallic;

    vec3 irradiance = texture(irradiance_texture, n).rgb;
    vec3 diffuse = irradiance * albedo;

    const float kMaxReflectionLod = 4.0;
    vec3 prefilter = textureLod(prefilter_texture, r, roughness * kMaxReflectionLod).rgb;
    vec2 brdf = texture(brdf_texture, vec2(max(dot(n, v), 0.0), roughness)).rg;
    vec3 specular = prefilter * (f * brdf.x + brdf.y);

    ambient = (kd * diffuse + specular) * ao;
  }
  
  vec3 color = ambient + lo;
  
  color == color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  fragment_color = vec4(color, 1.0);
}
