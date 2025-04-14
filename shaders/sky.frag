#version 460

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 surfacePosition;

out vec4 outColor;

uniform sampler2D texUnit;
uniform mat4 modelToWorld;
uniform mat4 worldToView;
uniform mat4 projection;

void main(void)
{
	outColor = texture(texUnit, fragTexCoord);
}
