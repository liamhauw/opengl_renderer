#version 330 core
out vec4 fragment_color;

in vec3 world_position;

uniform samplerCube environment_texture;

const float kPi = 3.14159265359;

void main()
{		
    vec3 n = normalize(world_position);

    vec3 irradiance = vec3(0.0);   
    
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, n));
    up         = normalize(cross(n, right));
       
    float sample_delta = 0.025;
    float sample_count = 0.0;
    for(float phi = 0.0; phi < 2.0 * kPi; phi += sample_delta)
    {
        for(float theta = 0.0; theta < 0.5 * kPi; theta += sample_delta)
        {
            vec3 tangent_sample_vec = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 world_sample_vec = tangent_sample_vec.x * right + tangent_sample_vec.y * up + tangent_sample_vec.z * n; 

            irradiance += texture(environment_texture, world_sample_vec).rgb * cos(theta) * sin(theta);
            sample_count++;
        }
    }
    irradiance = kPi * irradiance * (1.0 / float(sample_count));
    
    fragment_color = vec4(irradiance, 1.0);
}
