#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform vec3 cameraPosition;

void main(void)
{
	vec3 distortedNormal = vec3(sin(surfacePosition.x), sin(surfacePosition.y), fragNormal.z);
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	//vec3 reflectionDirection = reflect(cameraDirection, normalize(fragNormal));
	vec3 reflectionDirection = reflect(cameraDirection, normalize(distortedNormal));
	outColor = vec4(texture(mirrorCube, vec3(reflectionDirection.x, -reflectionDirection.y, reflectionDirection.z)).rgb, 1.0);
	//outColor = vec4(distortedNormal, 1.0);
}	
