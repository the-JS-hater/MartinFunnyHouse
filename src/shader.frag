#version 460

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 surfacePosition;

out vec4 outColor;

uniform sampler2D texUnit;
uniform mat4 projection;
uniform mat4 modelToWorld;

const vec4 sunPos 		= vec4(-77.390175, 81.960693, -81.914574, 1.0);
const vec3 sunColor 	= vec3(1.0, 1.0, 0.6);

void main(void)
{	
	vec3 diccVec = normalize(vec3((modelToWorld * sunPos) - surfacePosition)); 
	vec3 diffColor = (max(0.0, dot(normalize(fragNormal), diccVec)) * sunColor);
	outColor = vec4(0.1 * sunColor + 1.0 * diffColor, 1.0) * texture(texUnit, fragTexCoord);
}
