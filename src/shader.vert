#version 460

uniform mat4 ModelToWorld;
uniform mat4 worldToView;
uniform mat4 projection;

in vec3 in_Position;
in vec2 inTexCoord;
in vec3 in_Normal;

out vec4 surface_Position;
out vec3 frag_Normal;
out vec4 frag_Colors;
out vec2 frag_TexCoord;

void main(void)
{	
	gl_Position = projection * worldToView * ModelToWorld * vec4(in_Position, 1.0);
	frag_Normal = mat3(worldToView) * mat3(ModelToWorld) * in_Normal;
	frag_TexCoord = inTexCoord;
	surface_Position = worldToView * ModelToWorld * vec4(in_Position, 1.0);
}
