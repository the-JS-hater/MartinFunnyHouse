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
	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	vec3 ray = reflect(-cameraDirection, fragNormal);
	vec3 sampleVector = standard(ray);

	outColor = vec4(texture(mirrorCube, sampleVector).rgb, 1.0);
}

vec3 standard(vec3 ray) {
	return vec3(ray.x, -ray.y, ray.z);
}
