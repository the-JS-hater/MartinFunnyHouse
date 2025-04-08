#version 460

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 surfacePosition;

out vec4 outColor;

uniform sampler2D texUnit;
uniform mat4 modelToWorld;
uniform mat4 worldToView;
uniform mat4 projection;

/* TODO: Change to uniform value passed in */
const float specularExp = 125.0;

const vec4 sunPos 		= vec4(-77.390175, 81.960693, -81.914574, 1.0);
const vec3 sunColor 	= vec3(1.0, 1.0, 0.6);

void main(void)
{
	/* AMBIENT LIGHT */

	vec3 ambLight = sunColor;

	/* DIFFUSE LIGHT */

	vec3 diccVec = normalize(vec3((worldToView * modelToWorld * sunPos) - surfacePosition)); 
	vec3 diffColor = (max(0.0, dot(normalize(fragNormal), diccVec)) * sunColor);
	
	/* SPECULAR LIGHT */

	vec3 specVec = reflect(-normalize(vec3(worldToView * modelToWorld * sunPos) - vec3(surfacePosition)), normalize(fragNormal));
	float cosPhi = dot(specVec, -vec3(normalize(surfacePosition)));
	vec3 specColor = sunColor * pow(max(0.0, cosPhi), specularExp);
	

	/* FINAL LIGHT */

	outColor = vec4(0.5 * ambLight + 1.0 * diffColor + 0.3 * specColor, 1.0) * texture(texUnit, fragTexCoord);
}
