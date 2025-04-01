#version 460

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 surfacePosition;

out vec4 outColor;

uniform sampler2D texUnit;
uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

const vec4 sunPos 		= vec4(0.0, 110.58, 0.0, 1.0);
const vec3 sunColor 	= vec3(1.0, 1.0, 0.6);

void main(void)
{	
	vec3 diccVec = normalize(vec3((mdlMatrix * sunPos) - surfacePosition)); 
	vec3 diffColor = (max(0.0, dot(normalize(fragNormal), diccVec)) * sunColor);
	outColor = vec4(0.1 * sunColor + 0.8 * diffColor, 1.0) * texture(texUnit, fragTexCoord);
}
