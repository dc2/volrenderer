#version 140

uniform sampler3D volData;
uniform sampler1D lut;

uniform sampler2D front;
uniform sampler2D back;

uniform int width, height, depth;

uniform vec3 volumePosition;

uniform float stepsize;
uniform int steps;

uniform bool rayDithering;
uniform bool front2back;

uniform vec4 backgroundColor = vec4(1);

in vec3 texCoord;
in vec3 lightDir, eye;
out vec4 fragColor;

struct LightSource {
    bool enabled;
    vec3 pos;     // Light position in eye coords
    vec4 ambient; // Ambient light intensity
    vec4 diffuse; // Diffuse light intensity
    vec4 specular;// Specular light intensity
};

uniform LightSource light;

/**
 * Calculate the gradient at pos with delta d 
 */
vec3 grad(vec3 pos, vec3 d)
{
    float dx = texture(volData, vec3(pos.x+d.x, pos.y, pos.z)).r - texture(volData, vec3(pos.x-d.x, pos.y, pos.z)).r;
    float dy = texture(volData, vec3(pos.x, pos.y+d.y, pos.z)).r - texture(volData, vec3(pos.x, pos.y-d.y, pos.z)).r;
    float dz = texture(volData, vec3(pos.x, pos.y, pos.z+d.z)).r - texture(volData, vec3(pos.x, pos.y, pos.z-d.z)).r;
    
    return vec3(dx, dy, dz)*.5;
}

const float specularFactor = .3, specularExponent = 40;

/**
 * Calculate the lighting (Phong-Model) 
 */
vec3 lighting(vec3 pos, vec3 N)
{
    N = normalize(N);
    
    vec3 lightVector = light.pos - pos;//volumePosition;
    float lightDistance = length(lightVector);
    vec3 L = lightVector * (1.0 / lightDistance);
    
    float lightNormDot = dot(N, L);
    
    
    vec3 R = reflect(-L, N);
    vec3 V = normalize(pos);
    
    float specular = specularFactor * float(lightNormDot > 0.0) * pow(max(dot(R, V), 0.0), specularExponent);
    
    float diffuse = clamp(lightNormDot, 0.0, 1.0);
    
    return light.ambient.rgb + light.diffuse.rgb * diffuse + light.specular.rgb * specular;
}

/**
 * Perform raycasting through the volume
 */
vec3 raycast(const bool front2back)
{
    vec3 start, end; // ray start / end positions relative to volume
    vec4 dst;        // resulting color
    
    if(front2back) {
        end = texture(front, texCoord.st).rgb;
        start = texture(back, texCoord.st).rgb;
        
        dst = vec4(0);
    } else {
        start = texture(front, texCoord.st).rgb;
        end = texture(back, texCoord.st).rgb;
        
        dst = vec4(backgroundColor.rgb, 0);
    }
    
    vec3 dir = end - start; // ray direction
    
    if(dir == vec3(0)) {
        return backgroundColor.rgb;
    }
    
    float len = length(dir);
    vec3  step = normalize(dir) * stepsize;
    
    if(rayDithering) {
        float rnd = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453); // GLSL-"Random" 
        
         //return vec3(1)*rnd;
        start += step * rnd;
    }
    
    float len_acc = 0;
    
    vec4 voxel;
    vec4 color_sample;
    
    vec3 normal_delta = vec3(1.0/width, 1.0/height, 1.0/depth);
    //vec3 normal_delta = vec3(0.005);
    
    vec3 pos = start;
    
    int steps = int(len/stepsize);
    
    vec3 normal;
    for(int i = 0; i < steps; ++i)
    {
        voxel = texture(volData, pos);
        color_sample = texture(lut, voxel.r); // voxel.r = density
        
        pos += step;
        len_acc += stepsize;
        
        // terminate if opacity ~1 or the ray is outside the volume
        if(len_acc >= len || dst.a >= .95) break;
        
        // Skip transparent samples
        if(color_sample.a == 0) continue;
        
        if(light.enabled) {
            normal = grad(pos, normal_delta);
            
            color_sample.rgb *= lighting(pos, normal);
        }
        
        if(front2back) {
            dst.rgb += ((1 - dst.a) * color_sample.rgb * color_sample.a);
            dst.a   += ((1 - dst.a) * color_sample.a);
        } else {
            //dst.rgb = color_sample.rgb + (1-color_sample.a) * dst.rgb; // works only with premultiplied alpha
            dst.rgb = (color_sample.rgb * color_sample.a) + (dst.rgb * (1 - color_sample.a));
        }
    }
    
    if(front2back) {
        dst.rgb += backgroundColor.rgb * (1-dst.a);
    }
    
    return dst.rgb;
}

void main(void)
{
    fragColor.rgb = raycast(front2back);
}
