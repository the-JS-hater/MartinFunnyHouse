#version 460

in vec3 fragNormal;
in vec4 surfacePosition;
in vec2 fragTexCoord;

out vec4 outColor;

uniform samplerCube mirrorCube;
uniform sampler2D bumpMap; 
uniform vec3 cameraPosition;
uniform vec3 cubemapPos;

// vec3 fun(vec3 ray, vec3 pos, vec3 boxMin, vec3 boxMax, vec3 cubemapPos)
vec3 standard(vec3);
vec3 parallaxCorrected(vec3, vec3, vec3, vec3, vec3);

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
	// vec3 distortedNormal = rotMat * fragNormal;

	// vec3 reflectionDirection = reflect(-cameraDirection, normalize(distortedNormal));
	// vec3 distortedNormal = vec3(sin(surfacePosition.x), sin(surfacePosition.y), fragNormal.z);
	// vec3 distortedNormal = vec3(fragNormal.x, sin(surfacePosition.y), fragNormal.z);
	// rotationMatrix = rotationMatrix * rotMat;

	vec3 cameraDirection = vec3(normalize(surfacePosition.xyz - cameraPosition));
	// vec3 ray = reflect(-cameraDirection, normalize(distortedNormal));
	
	// When doing normal cubemap reflections.
	// vec3 ray = reflect(-cameraDirection, fragNormal);
	// vec3 sampleVector = standard(ray);

	// When doing parallax correction.
	const vec3 boxMin = vec3(-102.0, -102.0, -102.0);
	const vec3 boxMax = vec3(102.0, 102.0, 102.0);

	vec3 ray = -cameraDirection;
	vec3 sampleVector = parallaxCorrected(ray, vec3(surfacePosition), boxMin, boxMax, cubemapPos);
	sampleVector = reflect(sampleVector, fragNormal);

	// Output color
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

	return vec3(sampleVector.x, -sampleVector.y, sampleVector.z);
}
