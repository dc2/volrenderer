#version 140
in vec3 current_vertex;

out vec3 fragColor;

void main(void)
{
    fragColor = (current_vertex+vec3(1))/2.;
}
