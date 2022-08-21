#version 330 core
out vec4 fragment_color;
in vec3 world_position;

uniform samplerCube environment_texture;
uniform float roughness;

const float kPi = 3.14159265359;

float DistributionGGX(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float ndoth = max(dot(n, h), 0.0);
    float ndoth2 = ndoth * ndoth;

    float num   = a2;
    float denum = (ndoth2 * (a2 - 1.0) + 1.0);
    denum = kPi * denum * denum;
    return num / denum;
}

float VanDerCorput(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint n)
{
	return vec2(float(i)/float(n), VanDerCorput(i));
}

vec3 ImportanceSampleGGX(vec2 xi, vec3 n, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * kPi * xi.x;
	float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	
	vec3 h;
	h.x = cos(phi) * sin_theta;
	h.y = sin(phi) * sin_theta;
	h.z = cos_theta;
	
	vec3 up          = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, n));
	vec3 bitangent = cross(n, tangent);
	
	vec3 sample_vec = tangent * h.x + bitangent * h.y + n * h.z;
	return normalize(sample_vec);
}

void main()
{		
    vec3 n = normalize(world_position);
    vec3 r = n;
    vec3 v = r;

    uint sample_count = 1024u;
    vec3 prefilter_color = vec3(0.0);
    float total_weight = 0.0;
    
    for(uint i = 0u; i < sample_count; ++i)
    {
        vec2 xi = Hammersley(i, sample_count);
        vec3 h = ImportanceSampleGGX(xi, n, roughness);
        vec3 l  = normalize(2.0 * dot(v, h) * h - v);

        float ndotl = max(dot(n, l), 0.0);
        if(ndotl > 0.0)
        {
            float d   = DistributionGGX(n, h, roughness);
            float ndoth = max(dot(n, h), 0.0);
            float hdotv = max(dot(h, v), 0.0);
            float pdf = d * ndoth / (4.0 * hdotv) + 0.0001; 

            float resolution = 512.0;
            float sa_texel  = 4.0 * kPi / (6.0 * resolution * resolution);
            float sa_sample = 1.0 / (float(sample_count) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel); 
            
            prefilter_color += textureLod(environment_texture, l, mipLevel).rgb * ndotl;
            total_weight      += ndotl;
        }
    }

    prefilter_color = prefilter_color / total_weight;

    fragment_color = vec4(prefilter_color, 1.0);
}
