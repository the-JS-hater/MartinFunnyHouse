#define MAIN

#include "GL_utilities.h"
#include "MicroGlut.h"
#include "LittleOBJLoader.h" 
#include "LoadTGA.h" 
#include "VectorUtils4.h" 
#include <math.h>
#include <stdio.h>


#define WINDOW_H 720
#define WINDOW_W 1280
#define playerSpeed 0.7
#define kGroundSize 100.0f

//WARN: DO NOT define before include section 
#define near 1.0
#define far 280.0
#define right 0.7
#define left -0.7
#define top 0.5
#define bottom -0.5

//function prototypes
void input();
void updateCamera();
void updateFocus(int, int);
void placeMirror(float, float, vec3, vec3);

vec3 cameraPos = {-4, 10, -40};
vec3 lookingDir = {0, 0, 1};
vec3 focusPoint = {-4, 10, -39};
vec3 upDir = {0, 1, 0};
vec2 lastMousePos = {WINDOW_W / 2, WINDOW_H /2};

mat4 camera; 

GLfloat projectionMatrix[] = {    
	2.0f*near/(right-left), 0.0f, (right+left)/(right-left), 0.0f,
  0.0f, 2.0f*near/(top-bottom), (top+bottom)/(top-bottom), 0.0f,
  0.0f, 0.0f, -(far + near)/(far - near), -2*far*near/(far - near),
  0.0f, 0.0f, -1.0f, 0.0f 
};

// Reference to shader program
GLuint program;
// vertex array object
unsigned int modelsVertexArrayObjID;

// Martin data
Model *martin;
GLuint martinTex;

// Other models
Model *skybox;
Model *ground;

// Other textures
GLuint grassTex;
GLuint skyTex;

// modified version of source code from:
// https://en.wikipedia.org/wiki/Cube_mapping#Advantages
vec3 convertUVtoXYZ(int index, float u, float v)
{
  // convert range 0 to 1 to -1 to 1
  float uc = 2.0f * u - 1.0f;
  float vc = 2.0f * v - 1.0f;

	GLfloat x, y, z;
  switch (index)
  {
    case 0: x =  1.0f; y =    vc; z =   -uc; break;	// POSITIVE X
    case 1: x = -1.0f; y =    vc; z =    uc; break;	// NEGATIVE X
    case 2: x =    uc; y =  1.0f; z =   -vc; break;	// POSITIVE Y
    case 3: x =    uc; y = -1.0f; z =    vc; break;	// NEGATIVE Y
    case 4: x =    uc; y =    vc; z =  1.0f; break;	// POSITIVE Z
    case 5: x =   -uc; y =    vc; z = -1.0f; break;	// NEGATIVE Z
  }

	return vec3(x,y,z);
}

// modified version of source code from:
// https://en.wikipedia.org/wiki/Cube_mapping#Advantages
vec2 convertXYZtoUV(float x, float y, float z, int *index)
{
  float absX = fabs(x);
  float absY = fabs(y);
  float absZ = fabs(z);
  
  int isXPositive = x > 0 ? 1 : 0;
  int isYPositive = y > 0 ? 1 : 0;
  int isZPositive = z > 0 ? 1 : 0;
  
  GLfloat maxAxis, uc, vc;
  
  // POSITIVE X
  if (isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from +z to -z
    // v (0 to 1) goes from -y to +y
    maxAxis = absX;
    uc = -z;
    vc = y;
    *index = 0;
  }
  // NEGATIVE X
  if (!isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from -z to +z
    // v (0 to 1) goes from -y to +y
    maxAxis = absX;
    uc = z;
    vc = y;
    *index = 1;
  }
  // POSITIVE Y
  if (isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from +z to -z
    maxAxis = absY;
    uc = x;
    vc = -z;
    *index = 2;
  }
  // NEGATIVE Y
  if (!isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -z to +z
    maxAxis = absY;
    uc = x;
    vc = z;
    *index = 3;
  }
  // POSITIVE Z
  if (isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -y to +y
    maxAxis = absZ;
    uc = x;
    vc = y;
    *index = 4;
  }
  // NEGATIVE Z
  if (!isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from +x to -x
    // v (0 to 1) goes from -y to +y
    maxAxis = absZ;
    uc = -x;
    vc = y;
    *index = 5;
  }
	
	return vec2(
  	0.5f * (uc / maxAxis + 1.0f),
  	0.5f * (vc / maxAxis + 1.0f)
	);
}

void init(void)
{
	glutPassiveMotionFunc(*updateFocus);
	updateCamera();
	
	// set to rerender in ~60FPS
	glutRepeatingTimer(16);

	glClearColor(0.9, 0.3, 0.4 ,0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Load and compile shader
	program = loadShaders("shader.vert", "shader.frag");

	//NOTE: always do this after loadShaders
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	
	// Load Martin
	LoadTGATextureSimple("../textures/martin.tga", &martinTex);
	martin = LoadModel("../models/martin.obj");
	
	// Load Textures
	LoadTGATextureSimple("../textures/grass.tga", &grassTex);
	LoadTGATextureSimple("../textures/SkyBoxFull.tga", &skyTex);

	// Load skybox model
	skybox = LoadModel("../models/skyboxfull.obj");

	// Load mirror model

	// Load ground model
	vec3 vertices[] =
	{
	 vec3(-kGroundSize,0.0f,-kGroundSize),
	 vec3(-kGroundSize,0.0f,kGroundSize),
	 vec3(kGroundSize,-0.0f,-kGroundSize),
	 vec3(kGroundSize,-0.0f,kGroundSize)
	};
	
	vec3 vertexNormals[] =
	{
	  vec3(0.0f,1.0f,0.0f),
	  vec3(0.0f,1.0f,0.0f),
	  vec3(0.0f,1.0f,0.0f),
	  vec3(0.0f,1.0f,0.0f)
	};
	
	vec2 texCoords[] =
	{
	  vec2(0.0f,0.0f),
	  vec2(0.0f,20.0f),
	  vec2(20.0f,0.0f), 
	  vec2(20.0f,20.0f)
	};
	GLuint indices[] =
	{
	  0, 1, 2, 1, 3, 2
	};

	ground = LoadDataToModel(
		vertices,
		vertexNormals,
		texCoords,
		nullptr,
		indices,
		sizeof(vertices),
		sizeof(indices)
	);
}

void input()
{
	vec3 dir = normalize(focusPoint - cameraPos);
	vec3 side_dir = normalize(cross(upDir, dir));
	
	if (glutKeyIsDown('w')) {
		cameraPos += dir * playerSpeed;
	}
	if (glutKeyIsDown('a')) {
		cameraPos += side_dir * playerSpeed;
	};
	if (glutKeyIsDown('s')) {
		cameraPos -= dir * playerSpeed;
	} 
	if (glutKeyIsDown('d')) {
		cameraPos -= side_dir * playerSpeed;
	};
	//LEFT_SHIFT
	if (glutKeyIsDown(GLUT_KEY_LEFT_SHIFT)) {
		cameraPos -= upDir * playerSpeed;
	};
	//SPACEBAR
	if (glutKeyIsDown(' ')) {
		cameraPos += upDir * playerSpeed;
	};
	focusPoint = cameraPos + lookingDir;
}


void updateFocus(int x, int y)
{
	vec2 mouseDelta = (vec2){
		lastMousePos.x - x, 
		lastMousePos.y - y
	};
	
	GLfloat theta_x = 0.0f;
	GLfloat theta_y = 0.0f;
	GLfloat sensitivity = 0.0005f;
	int threshhold = 1;

	if (abs(mouseDelta.x) > threshhold) {
		theta_x = mouseDelta.x * sensitivity;
	}
	
	if (abs(mouseDelta.y) > threshhold) {
		theta_y = mouseDelta.y * sensitivity;
	}
			

	vec3 side_dir = cross(lookingDir, upDir);
	lookingDir = normalize(ArbRotate(upDir, theta_x) * ArbRotate(side_dir, theta_y) * lookingDir);
	
	focusPoint = cameraPos + lookingDir;

	glutWarpPointer(WINDOW_W / 2, WINDOW_H / 2);
	lastMousePos = (vec2){WINDOW_W / 2, WINDOW_H / 2};
}

void updateCamera()
{
	camera = lookAtv(
		cameraPos,
		focusPoint,
		upDir
	);

	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, camera.m);
}

void display(void)
{
	if (glutKeyIsDown(GLUT_KEY_F7)) glutToggleFullScreen();
	printError("pre display");

	glutHideCursor();
	input();
	updateCamera();

	// GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME);
	// GLfloat theta = t/600;
	
	// SKYBOX
	mat4 skyMat = mat3tomat4(mat4tomat3(camera)) * S(10);
	
	// GROUND 
	mat4 groundMtW = T(0,0,0);
	
	// MARTIN
	// TODO: this will be altered later on to move martin around
	mat4 scaleMartin = S(2.2);
	mat4 matTrans = T(-3,4.7,-2.4);
	mat4 matMtW = matTrans * Ry(-M_PI / 2) * scaleMartin;
	
	
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(modelsVertexArrayObjID);    // Select VAO
	
	// DRAW SKYBOX
	// NOTE: important to draw this first
	
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, IdentityMatrix().m);
	
	glBindTexture(GL_TEXTURE_2D, skyTex);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, skyMat.m);
	DrawModel(skybox, program, "inPosition", "inNormal", "inTexCoord");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// NOTE: must be "turned off" for skybox
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, camera.m);
	
	// DRAW GROUND
	glBindTexture(GL_TEXTURE_2D, grassTex);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, groundMtW.m);
	DrawModel(ground, program, "inPosition", "inNormal", "inTexCoord");
	
	// DRAW MARTIN
	glBindTexture(GL_TEXTURE_2D, martinTex);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, matMtW.m);
	DrawModel(martin, program, "inPosition", "inNormal", "inTexCoord");

	// DRAW MIRROR
	placeMirror(10.0f, 10.0f, {0.0,10.0,0.0},{0,0,0});
	
	printError("display");
	glutSwapBuffers();
}

void placeMirror(float width, float height, vec3 position, vec3 rotation)
{
	Model *mirror;

	// Load model model
	vec3 vertices[] =
	{
	 vec3(-width / 2.0f,-height / 2.0f,0.0f),
	 vec3(-width / 2.0f,height / 2.0f,0.0f),
	 vec3(width / 2.0f,-height / 2.0f,-0.0f),
	 vec3(width / 2.0f,height / 2.0f,-0.0f)
	};
	
	vec3 vertex_normals[] =
	{
	  vec3(1.0f,0.0f,0.0f),
	  vec3(1.0f,0.0f,0.0f),
	  vec3(1.0f,0.0f,0.0f),
	  vec3(1.0f,0.0f,0.0f)
	};
	
	vec2 tex_coords[] =
	{
	  vec2(0.0f,0.0f),
	  vec2(0.0f,20.0f),
	  vec2(20.0f,0.0f), 
	  vec2(20.0f,20.0f)
	};
	GLuint indices[] =
	{
	  0, 1, 2, 1, 3, 2
	};

	mirror = LoadDataToModel(
		vertices,
		vertex_normals,
		tex_coords,
		nullptr,
		indices,
		sizeof(vertices),
		sizeof(indices)
	);

	mat4 modelToWorld = T(position.x, position.y, position.z) * Rz(rotation.z) * Rx(rotation.x) * Ry(rotation.y);

	glBindTexture(GL_TEXTURE_2D, grassTex);
	glUniformMatrix4fv(glGetUniformLocation(program, "ModelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(mirror, program, "inPosition", "inNormal", "inTexCoord");
}

	int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowPosition (10, 10);
	glutInitWindowSize(WINDOW_W, WINDOW_H);
	glutCreateWindow ("Martin doing the funny");
	glutDisplayFunc(display); 
	init ();
	glutMainLoop();
	return 0;
}
