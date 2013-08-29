#version 140

in vec4 vertex;

uniform mat4 mvp;

out vec3 current_vertex;


void main(void)
{
    current_vertex = vec3(vertex);
    gl_Position = mvp*vertex;
}
