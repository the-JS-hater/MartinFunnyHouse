#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform vec3 cameraPosition;

void main(void)
{	
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	vec3 reflectionDirection = reflect(cameraDirection, vec3(normalize(fragNormal)));
	outColor = vec4(texture(mirrorCube, reflectionDirection).rgb, 1.0);
	// outColor = vec4(texture(mirrorCube, normalize(vec3(1.0, 1.0, 0.0))).rgb, 0.0);
	// outColor = vec4(reflectionDirection.xyz, 1.0);
	// outColor = vec4(cameraDirection.xyz, 1.0);
	// outColor = vec4(0.0, 1.0, 0.0, 0.0);
}	
