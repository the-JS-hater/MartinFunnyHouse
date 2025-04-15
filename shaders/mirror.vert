#version 460

uniform mat4 modelToWorld;
uniform mat4 worldToView;
uniform mat4 projection;

out vec4 surfacePosition;
out vec3 fragNormal;
out vec2 fragTexCoord;

in vec3 inPosition;
in vec3 inNormal;
in vec2 inTexCoord;


void main(void)
{	
	gl_Position = projection * worldToView * modelToWorld * vec4(inPosition, 1.0);
	fragNormal = mat3(worldToView) * mat3(modelToWorld) * inNormal;
	surfacePosition = modelToWorld * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	// surfacePosition = vec4(0.0,0.0,0.0,0.0);
	// fragNormal = vec3(0.0,0.0,0.0);
}
