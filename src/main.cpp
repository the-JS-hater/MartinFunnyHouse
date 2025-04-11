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
void drawMirror(float, float, vec3, vec3);
void drawModelWrapper(mat4, Model*, GLuint);
void drawSkybox();
void loadCubemap();
void loadMirror(float, float);

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

// Cubemap testing files
const char *textureFileName[12] =
	{
		"../textures/cubemap_test/skyrender0004.tga",
		"../textures/cubemap_test/skyrender0001.tga",
		"../textures/cubemap_test/skyrender0003.tga",
		"../textures/cubemap_test/skyrender0006.tga",
		"../textures/cubemap_test/skyrender0002.tga",
		"../textures/cubemap_test/skyrender0005.tga",
};

TextureData t[6];

// Reference to shader program
GLuint program;
GLuint mirrorProgram;

// vertex array object
unsigned int modelsVertexArrayObjID;

// Martin data
Model *martin;
GLuint martinTex;

// Other models
Model *skybox;
Model *ground;
Model *mirror;

// Other textures
GLuint grassTex;
GLuint skyTex;
GLuint cubemap;

void init(void)
{
	glutPassiveMotionFunc(*updateFocus);
	updateCamera();
	
	// set to rerender in ~60FPS
	glutRepeatingTimer(16);

	glClearColor(0.9, 0.3, 0.4 ,0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	printError("GL inits");
	
	// Load and compile shader
	program = loadShaders("../shaders/shader.vert", "../shaders/shader.frag");
	
	printError("init shader");
	
	//NOTE: always do this after loadShaders
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	
	mirrorProgram = loadShaders("../shaders/mirror.vert", "../shaders/mirror.frag");
	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "projection"), 1, GL_TRUE, projectionMatrix);

	// Load Martin
	LoadTGATextureSimple("../textures/martin.tga", &martinTex);
	martin = LoadModel("../models/martin.obj");
	
	// Load Textures
	LoadTGATextureSimple("../textures/grass.tga", &grassTex);
	LoadTGATextureSimple("../textures/SkyBoxFull.tga", &skyTex);

	// Load skybox model
	skybox = LoadModel("../models/skyboxfull.obj");

	// Load mirror model
	loadCubemap();
	loadMirror(10.0, 10.0);

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
	#ifndef _WIN32
	if (glutKeyIsDown(GLUT_KEY_ESC)) glutExit();
	#endif

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

void drawModelWrapper(mat4 mdl, Model* model, GLuint tex)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, mdl.m);
	DrawModel(model, program, "inPosition", "inNormal", "inTexCoord");
}

void drawSkybox()
{
	mat4 skyMat = mat3tomat4(mat4tomat3(camera)) * S(10);
	
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, IdentityMatrix().m);
	
	glBindTexture(GL_TEXTURE_2D, skyTex);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, skyMat.m);
	DrawModel(skybox, program, "inPosition", "inNormal", "inTexCoord");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, camera.m);
}

void loadCubemap() {
	glGenTextures(1, &cubemap);	  // Generate OpenGL texture IDs
	glActiveTexture(GL_TEXTURE0); // Just make sure the texture unit match

	// Note all operations on GL_TEXTURE_CUBE_MAP, not GL_TEXTURE_2D

	// Load texture data and create ordinary texture objects (for skybox)
	for (int i = 0; i < 6; i++)
	{
		LoadTGATexture(textureFileName[i], &t[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	// Load to cube map
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, t[0].w, t[0].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[0].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, t[1].w, t[1].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[1].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, t[2].w, t[2].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[2].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, t[3].w, t[3].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[3].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, t[4].w, t[4].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[4].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, t[5].w, t[5].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[5].imageData);

	//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// MIPMAPPING
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void display(void)
{
	if (glutKeyIsDown(GLUT_KEY_F7)) glutToggleFullScreen();
	printError("pre display");

	glutHideCursor();
	input();
	updateCamera();
	
	// GROUND 
	mat4 groundMtW = T(0,0,0);
	
	// MARTIN
	GLfloat martinHeight = 2.2;
	mat4 scaleMartin = S(martinHeight);
	mat4 matTrans = T(cameraPos.x, cameraPos.y - martinHeight, cameraPos.z);
	
	vec3 cameraDirXZ = Normalize({lookingDir.x, 0.0f, lookingDir.z});
	float cameraAngle = atan2(cameraDirXZ.x, cameraDirXZ.z);
	// +0.6 random ass offset because the model is annoying
	mat4 matMtW = matTrans * Ry(cameraAngle + 0.6) * scaleMartin;

	// DRAW WORLD
	glUseProgram(program);

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(modelsVertexArrayObjID);    // Select VAO
	
	// DRAW SKYBOX
	drawSkybox();
	
	// DRAW GROUND
	drawModelWrapper(groundMtW, ground, grassTex);
	
	// DRAW MARTIN
	drawModelWrapper(matMtW, martin, martinTex);

	// DRAW MIRROR
	glUseProgram(mirrorProgram);

	drawMirror(10.0f, 10.0f, {0.0,10.0,0.0},{0,0,0});
	
	printError("display");
	glutSwapBuffers();
}

void loadMirror(float width, float height) {
	// Load model model
	vec3 vertices[] =
		{
			vec3(-width / 2.0f, -height / 2.0f, 0.0f),
			vec3(-width / 2.0f, height / 2.0f, 0.0f),
			vec3(width / 2.0f, -height / 2.0f, -0.0f),
			vec3(width / 2.0f, height / 2.0f, -0.0f)};

	vec3 vertexNormals[] =
		{
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f)};

	vec2 texCoords[] =
		{
			vec2(0.0f, 0.0f),
			vec2(0.0f, 20.0f),
			vec2(20.0f, 0.0f),
			vec2(20.0f, 20.0f)};
	GLuint indices[] =
		{
			0, 1, 2, 1, 3, 2};

	mirror = LoadDataToModel(
		vertices,
		vertexNormals,
		texCoords,
		nullptr,
		indices,
		sizeof(vertices),
		sizeof(indices));
}

void drawMirror(float width, float height, vec3 position, vec3 rotation)
{

	mat4 modelToWorld = T(position.x, position.y, position.z) * Rz(rotation.z) * Rx(rotation.x) * Ry(rotation.y);

	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0); // Just make sure the texture unit match
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	glUniform1i(glGetUniformLocation(mirrorProgram, "mirrorCube"), 0);

	glBindTexture(GL_TEXTURE_2D, grassTex);
	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "worldToView"), 1, GL_TRUE, camera.m);
	glUniform3f(glGetUniformLocation(mirrorProgram, "cameraPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
	DrawModel(mirror, mirrorProgram, "inPosition", "inNormal", "inTexCoord");

	glEnable(GL_CULL_FACE);
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
