#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform sampler2D bumpMap; 
uniform vec3 cameraPosition;
uniform vec3 cubemapPos;

vec3 standard(vec3);

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

	mat3 rotMat = mat3(1.0) * yaw * pitch;
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));

	vec3 distortedNormal = vec3(fragNormal.x, sin(surfacePosition.y), fragNormal.z);

	vec3 ray = reflect(-cameraDirection, normalize(distortedNormal));
	vec3 sampleVector = standard(ray);

	// Output color
	outColor = vec4(texture(mirrorCube, sampleVector).rgb, 1.0);
}

vec3 standard(vec3 ray) {
	return vec3(ray.x, -ray.y, ray.z);
}
