#version 330 core
out vec2 fragment_color;

in vec2 texture_coord;

const float kPi = 3.14159265359;

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
	float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a*a - 1.0) * xi.y));
	float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
	
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

float GeometrySchlickGGX(float ndotv, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float num   = ndotv;
    float denum = ndotv * (1.0 - k) + k;

    return num / denum;
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float ndotv = max(dot(n, v), 0.0);
    float ndotl = max(dot(n, l), 0.0);
    float ggx2 = GeometrySchlickGGX(ndotv, roughness);
    float ggx1 = GeometrySchlickGGX(ndotl, roughness);

    return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float ndotv, float roughness)
{
    vec3 v;
    v.x = sqrt(1.0 - ndotv*ndotv);
    v.y = 0.0;
    v.z = ndotv;

    float a = 0.0;
    float b = 0.0; 

    vec3 n = vec3(0.0, 0.0, 1.0);
    
    uint sample_count = 1024u;
    for(uint i = 0u; i < sample_count; ++i)
    {
        vec2 xi = Hammersley(i, sample_count);
        vec3 h = ImportanceSampleGGX(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float ndotl = max(l.z, 0.0);
        float ndoth = max(h.z, 0.0);
        float vdoth = max(dot(v, h), 0.0);

        if(ndotl > 0.0)
        {
            float g = GeometrySmith(n, v, l, roughness);
            float g_vis = (g * vdoth) / (ndoth * ndotv);
            float fc = pow(1.0 - vdoth, 5.0);

            a += (1.0 - fc) * g_vis;
            b += fc * g_vis;
        }
    }
    a /= float(sample_count);
    b /= float(sample_count);
    return vec2(a, b);
}

void main() 
{
    vec2 integrated_brdf = IntegrateBRDF(texture_coord.x, texture_coord.y);
    fragment_color = integrated_brdf;
}