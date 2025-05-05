#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform vec3 cameraPosition;

void main(void)
{
	float t = surfacePosition.y * 0.01;

	mat3 yaw = mat3(
		cos(t), sin(t), 0.0, 	//first COLUMN
		-sin(t), cos(t), 0.0, 
		0.0, 0.0, 1.0
	);              

	mat3 pitch = mat3(
		cos(t), 0.0, -sin(t),
		0.0, 1.0, 0.0,
		sin(t), 0.0, cos(t)
	);

	mat3 roll = mat3(
		1.0, 0.0, 0.0,
		0.0, cos(t), sin(t),
		0.0, -sin(t), cos(t)
	);

	mat3 rotMat = mat3(1.0);

	vec3 distortedNormal = rotMat * fragNormal;
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	vec3 reflectionDirection = reflect(-cameraDirection, normalize(distortedNormal));
	outColor = vec4(texture(mirrorCube, vec3(reflectionDirection.x, -reflectionDirection.y, reflectionDirection.z)).rgb, 1.0);
}	
