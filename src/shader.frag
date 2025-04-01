#version 460

uniform sampler2D texUnit;

uniform mat4 worldToView;

in vec4 surface_Position;
in vec2 frag_TexCoord;
in vec3 frag_Normal;

out vec4 out_Color;

void main(void)
{
	out_Color =  texture(texUnit, frag_TexCoord);
}
