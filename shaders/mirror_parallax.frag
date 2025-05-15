#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform sampler2D bumpMap; 
uniform vec3 cameraPosition;
uniform vec3 cubemapPos;

vec3 parallaxCorrected(vec3, vec3, vec3, vec3, vec3);
vec3 standard(vec3);

void main(void)
{
	const vec3 boxMin = vec3(-102.0, -102.0, -102.0) + cubemapPos;
	const vec3 boxMax = vec3(102.0, 102.0, 102.0) + cubemapPos;

	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	// vec3 ray = reflect(vec3(-cameraDirection.x, cameraDirection.y, -cameraDirection.z), fragNormal);
	vec3 ray = cameraDirection;
	vec3 sampleVector = parallaxCorrected(ray, vec3(surfacePosition), boxMin, boxMax, cubemapPos);
	// vec3 sampleVector = standard(ray);
	// sampleVector = reflect(sampleVector, fragNormal);

	outColor = vec4(texture(mirrorCube, sampleVector).rgb, 1.0);
}

vec3 standard(vec3 ray) {
	return vec3(ray.x, -ray.y, ray.z);
}

vec3 parallaxCorrected(vec3 ray, vec3 pos, vec3 boxMin, vec3 boxMax, vec3 cubemapPos) {
	// https://forum.derivative.ca/t/parallax-corrected-cubemap-shaders-glsl/299290
	vec3 intersection1 = (boxMax - pos) / ray;
	vec3 intersection2 = (boxMin - pos) / ray;

	vec3 furthestPlane = max(intersection1, intersection2);

	float planeDistance = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);

	vec3 intersectedPosition = pos + ray * planeDistance;

	vec3 sampleVector = intersectedPosition - cubemapPos;

	return vec3(sampleVector.x, sampleVector.y, sampleVector.z);
}
