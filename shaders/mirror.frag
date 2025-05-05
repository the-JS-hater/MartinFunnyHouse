#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform sampler2D bumpMap; 
uniform vec3 cameraPosition;

void getRotation(in vec3, out mat3);

void main(void)
{
	float t = surfacePosition.y * 0.1;

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

	mat3 rotMat = mat3(1.0) * roll;

	vec3 distortedNormal = rotMat * fragNormal;
	// vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	// vec3 reflectionDirection = reflect(-cameraDirection, normalize(distortedNormal));
	// vec3 distortedNormal = vec3(sin(surfacePosition.x), sin(surfacePosition.y), fragNormal.z);
	// vec3 distortedNormal = vec3(fragNormal.x, sin(surfacePosition.y), fragNormal.z);
	mat3 rotationMatrix;
	getRotation(normalize(fragNormal), rotationMatrix);
	rotationMatrix = rotationMatrix * rotMat;
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	vec3 reflectionDirection = reflect(-cameraDirection, rotationMatrix * vec3(0.0, 1.0, 0.0));
	// vec3 reflectionDirection = reflect(-cameraDirection, normalize(distortedNormal));
	outColor = vec4(texture(mirrorCube, vec3(reflectionDirection.x, -reflectionDirection.y, reflectionDirection.z)).rgb, 1.0);
}	

void getRotation(in vec3 normal, out mat3 rotationMatrix)
{
	rotationMatrix[0] = vec3(0, 0, 0);
	rotationMatrix[1] = normal;
	rotationMatrix[2] = vec3(0, 0, 0);
}