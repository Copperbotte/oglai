#version 430
in vec4 gl_Vertex;
in vec4 gl_Normal;
in vec4 gl_Color;

uniform mat4 world;
uniform mat4 viewproj;
uniform vec4 ambient;
uniform vec3 campos;

varying vec4 color;
varying vec3 normal;
varying vec4 wpos;

void main()
{
   wpos = world * gl_Vertex;
   gl_Position = viewproj * wpos;
   normal = normalize((world * vec4(gl_Normal.xyz, 0.0)).xyz);
   color = gl_Color;
}
