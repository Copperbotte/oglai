#version 330
out vec4 gl_FragColor;
in vec4 color;
in vec3 normal;
in vec4 wpos;
uniform vec4 ambient;
uniform vec3 campos;

float s2l(float s)
{
    if (s <= 0.04045)
        return s / 12.92;
    return pow((s + 0.055) / 1.055, 2.4);
}

float l2s(float l)
{
    if (l <= 0.0031308)
        return l * 12.92;
    return pow(l, 1.0 / 2.4) * 1.055 - 0.055;
}

vec3 s2lvec(vec3 s)
{
    return vec3(s2l(s.x), s2l(s.y), s2l(s.z));
}

vec3 l2svec(vec3 l)
{
    return vec3(l2s(l.x), l2s(l.y), l2s(l.z));
}
void main()
{
   vec3 albedo = s2lvec(color.xyz);
   vec3 light = vec3(0,1,0);
   vec3 view = normalize(campos - wpos.xyz);
   float power = 300;
   float reflectivity = 0.2;
   
   vec3 refl = reflect(light, normal);
   float diffuse = clamp(dot(normal, light), 0.0, 1.0);
   float specular = clamp(dot(view, refl), 0.0, 1.0);
   specular = pow(specular, power) * (power + 2.0) / 2.0;
   
   vec3 output = s2lvec(ambient.xyz) * ((1.0 - reflectivity) * albedo + reflectivity);
   output += (1.0 - reflectivity) * diffuse * albedo + reflectivity * specular;
   gl_FragColor = vec4(l2svec(output), 1.0);
}
