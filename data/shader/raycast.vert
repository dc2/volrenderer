#version 140

in vec4 vertex;
in vec3 vertexTexCoord;

uniform int width, depth, height;

out vec3 texCoord;

void main(void)
{
    texCoord = vertexTexCoord;

    gl_Position = vertex;
}
