#version 460

uniform mat4 modelToWorld;
uniform mat4 worldToView;
uniform mat4 projection;

in vec3 inPosition;
in vec3 inNormal;

out vec4 surfacePosition;
out vec3 fragNormal;

void main(void)
{	
	gl_Position = projection * worldToView * modelToWorld * vec4(inPosition, 1.0);
	fragNormal = mat3(worldToView) * mat3(modelToWorld) * inNormal;
	surfacePosition = worldToView * modelToWorld * vec4(inPosition, 1.0);
	fragNormal = vec3(0.0, 0.0, 0.0);
	surfacePosition = vec3(0.0, 0.0, 0.0);
	frag
}
