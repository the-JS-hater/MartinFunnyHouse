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
#define PLAYER_SPEED 0.7
#define GROUND_SIZE 100.0f
#define SCALE_HEIGHT 8.0f
#define SCALE_SIZE 1.0f

// TODO: Define these for mirror 90 deg FOV
// Frustum

#define near 1.0
#define far 280.0
#define right 0.7
#define left -0.7
#define top 0.5
#define bottom -0.5

struct Camera3D 
{
	vec3 pos;
	vec3 lookingDir;
	vec3 upDir;

	Camera3D(vec3 pos, vec3 lookingDir, vec3 upDir) {
		this->pos = pos;
		this->lookingDir = lookingDir;
		this->upDir = upDir;
	}
};

//function prototypes
void input();
void updateCamera(Camera3D camera3D);
void updateFocus(int, int);
void drawMirror(vec3, vec3, Camera3D);
void drawModelWrapper(mat4, Model*, GLuint);
void drawSkybox();
void loadCubemap();
void loadMirror(float, float);
void updateMirror(FBOstruct*, vec3);
void updateFBO(FBOstruct*, Camera3D);
Model* GenerateBumpmap(TextureData *tex);

Camera3D playerCamera = Camera3D(
	{-4, 10, -40},
	{0, 0, 1},
	{0, 1, 0}
);

vec2 lastMousePos = {WINDOW_W / 2, WINDOW_H /2};

mat4 cameraMatrix;

GLfloat projectionMatrix[] = {    
	2.0f*near/(right-left), 0.0f, (right+left)/(right-left), 0.0f,
  0.0f, 2.0f*near/(top-bottom), (top+bottom)/(top-bottom), 0.0f,
  0.0f, 0.0f, -(far + near)/(far - near), -2*far*near/(far - near),
  0.0f, 0.0f, -1.0f, 0.0f 
};

// GLfloat fov90Matrix[] = {
// 	2.0f*near/(right-left), 0.0f, (right+left)/(right-left), 0.0f,
//   0.0f, 2.0f*near/(top-bottom), (top+bottom)/(top-bottom), 0.0f,
//   0.0f, 0.0f, -(far + near)/(far - near), -2*far*near/(far - near),
//   0.0f, 0.0f, -1.0f, 0.0f 
// };

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
GLuint skyProgram;

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

// Bumpmap
GLuint bumpTex;
TextureData testBumpmap; 
Model* bumpModel;

// Mirror FBOs
FBOstruct *mirrorFBO[6];

void init(void)
{
	glutPassiveMotionFunc(*updateFocus);
	updateCamera(playerCamera);
	
	// set to rerender in ~60FPS
	glutRepeatingTimer(16);

	glClearColor(0.9, 0.3, 0.4 ,0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	printError("GL inits");
	
	// Load and compile shader
	program = loadShaders("../shaders/shader.vert", "../shaders/shader.frag");
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	
	mirrorProgram = loadShaders("../shaders/mirror.vert", "../shaders/mirror.frag");
	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "projection"), 1, GL_TRUE, projectionMatrix);

	skyProgram = loadShaders("../shaders/shader.vert", "../shaders/sky.frag");
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "projection"), 1, GL_TRUE, projectionMatrix);

	printError("init shader");
	
	// Load Martin
	LoadTGATextureSimple("../textures/martin.tga", &martinTex);
	martin = LoadModel("../models/martin.obj");
	
	// Load Textures
	LoadTGATextureSimple("../textures/grass.tga", &grassTex);
	LoadTGATextureSimple("../textures/SkyBoxFull.tga", &skyTex);
	LoadTGATextureData("../textures/44-terrain.tga", &testBumpmap);

	// Load skybox model
	skybox = LoadModel("../models/skyboxfull.obj");
	bumpModel = GenerateBumpmap(&testBumpmap);

	// Initialize mirror framebuffer objects.
	for (size_t i = 0; i < 6; i++) {
		mirrorFBO[i] = initCubemapFBO(512, 512, 0);
	}

	// Load mirror model
	loadCubemap();

	loadMirror(10.0, 10.0);
	//mirror->normalArray = bumpModel->normalArray;

	// Load ground model
	vec3 vertices[] =
	{
	 vec3(-GROUND_SIZE,0.0f,-GROUND_SIZE),
	 vec3(-GROUND_SIZE,0.0f,GROUND_SIZE),
	 vec3(GROUND_SIZE,-0.0f,-GROUND_SIZE),
	 vec3(GROUND_SIZE,-0.0f,GROUND_SIZE)
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

	vec3 dir = normalize(playerCamera.lookingDir);
	vec3 side_dir = normalize(cross(playerCamera.upDir, dir));
	
	if (glutKeyIsDown('w')) {
		playerCamera.pos += dir * PLAYER_SPEED;
	}
	if (glutKeyIsDown('a')) {
		playerCamera.pos += side_dir * PLAYER_SPEED;
	};
	if (glutKeyIsDown('s')) {
		playerCamera.pos -= dir * PLAYER_SPEED;
	} 
	if (glutKeyIsDown('d')) {
		playerCamera.pos -= side_dir * PLAYER_SPEED;
	};
	//LEFT_SHIFT
	if (glutKeyIsDown(GLUT_KEY_LEFT_SHIFT)) {
		playerCamera.pos -= playerCamera.upDir * PLAYER_SPEED;
	};
	//SPACEBAR"
	if (glutKeyIsDown(' ')) {
		playerCamera.pos += playerCamera.upDir * PLAYER_SPEED;
	};
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
			

	vec3 side_dir = cross(playerCamera.lookingDir, playerCamera.upDir);
	playerCamera.lookingDir = normalize(ArbRotate(playerCamera.upDir, theta_x) * ArbRotate(side_dir, theta_y) * playerCamera.lookingDir);

	glutWarpPointer(WINDOW_W / 2, WINDOW_H / 2);
	lastMousePos = (vec2){WINDOW_W / 2, WINDOW_H / 2};
}

void updateCamera(Camera3D camera)
{
	cameraMatrix = lookAtv(
		camera.pos,
		camera.pos + camera.lookingDir,
		camera.upDir
	);

	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, cameraMatrix.m);
}

void drawModelWrapper(mat4 mdl, Model* model, GLuint tex)
{
	glUseProgram(program);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, mdl.m);
	DrawModel(model, program, "inPosition", "inNormal", "inTexCoord");
}

void drawSkybox()
{
	glUseProgram(skyProgram);
	mat4 skyMat = mat3tomat4(mat4tomat3(cameraMatrix)) * S(10);
	
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "worldToView"), 1, GL_TRUE, IdentityMatrix().m);
	
	glBindTexture(GL_TEXTURE_2D, skyTex);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "modelToWorld"), 1, GL_TRUE, skyMat.m);
	DrawModel(skybox, skyProgram, "inPosition", NULL, "inTexCoord");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void loadCubemap() 
{
	glUseProgram(mirrorProgram);

	glUniform1i(glGetUniformLocation(mirrorProgram, "mirrorCube"), 0);

	glGenTextures(1, &cubemap);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	for (int i = 0; i < 6; i++)
	{
		LoadTGATexture(textureFileName[i], &t[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, t[0].width, t[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[0].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, t[1].width, t[1].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[1].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, t[2].width, t[2].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[2].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, t[3].width, t[3].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[3].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, t[4].width, t[4].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[4].imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, t[5].width, t[5].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t[5].imageData);
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void display(void)
{
	if (glutKeyIsDown(GLUT_KEY_F7)) glutToggleFullScreen();
	printError("pre display");

	glutHideCursor();
	input();

	// Render mirror perspective
	updateMirror(mirrorFBO[0], {0.0, 5.0, 0.0});
	// updateFBO(mirrorFBO[0], );

	// Render Martin's perspective
	updateFBO(0, playerCamera);
	
	printError("display");
	glutSwapBuffers();
}

void loadMirror(float width, float height) {
	// Load model model
	

	//TODO: heap allocate. Arrs too big for stack

	//Mirrors are henceforth assumed to be 512x512
	// vec3 vertices[512 * 512];
	// vec3 vertexNormals[512 * 512];
	// GLuint indices[512 * 512 * 2 * 3];
	// 
	// float const stepX = width / 512.0f;  
	// float const stepY = height / 512.0f;  

	// for (size_t i {0}; i < 512 * 512; ++i)
	// {
	// 	vertices[i] = (vec3){
	// 		(float)(i / 512) * stepX,
	// 		(float)(i % 512) * stepY,
	// 		0.0f,
	// 	};
	// 	vertexNormals[i] = (vec3){1.0f, 0.0f, 0.0f};
	// }
	// for (size_t i {0}; i < 511 * 511; ++i)
	// {
	// 	indices[i + 0] = i; 
	// 	indices[i + 1] = i + 1;
	// 	indices[i + 2] = i + 512;
	// 	indices[i + 3] = i + 512;
	// 	indices[i + 4] = i + 1;
	// 	indices[i + 5] = i + 512 + 1;
	// }

	vec3 vertices[] =
		{
			vec3(-width / 2.0f, -height / 2.0f, 0.0f),
			vec3(-width / 2.0f, height / 2.0f, 0.0f),
			vec3(width / 2.0f, -height / 2.0f, -0.0f),
			vec3(width / 2.0f, height / 2.0f, -0.0f)
		};
	vec3 vertexNormals[] =
		{
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f),
			vec3(1.0f, 0.0f, 0.0f)
		};
	GLuint indices[] = {0, 1, 2, 1, 3, 2};

	mirror = LoadDataToModel(
		vertices,
		vertexNormals,
		nullptr,
		nullptr,
		indices,
		sizeof(vertices),
		sizeof(indices)
	);
}

void updateMirror(FBOstruct *mirrorFBO, vec3 position) {
	GLenum cubeSides[6] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	};
	vec3 upVectors[6] = {
		{0, -1, 0},
		{0, -1, 0},
		{0, 0, 1},
		{0, 0, -1},
		{0, -1, 0},
		{0, -1, 0},
	};
	vec3 directions[6] = {
		{1, 0, 0},
		{-1, 0, 0},
		{0, 1, 0},
		{0, -1, 0},
		{0, 0, 1},
		{0, 0, -1},
	};

	for (size_t i = 0; i < 6; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeSides[i], mirrorFBO->texid, 0);
		CHECK_FRAMEBUFFER_STATUS();

		// Camera3D()

		Camera3D mirrorCamera = Camera3D(
			position,
			directions[i],
			upVectors[i]
		);

		updateFBO(mirrorFBO, mirrorCamera);
	}
}

void updateFBO(FBOstruct *fbo, Camera3D camera) {
	useFBO(fbo, 0, 0);

	updateCamera(camera);

	// GROUND
	mat4 groundMtW = T(0, 0, 0);

	// MARTIN
	GLfloat martinHeight = 2.2;
	mat4 scaleMartin = S(martinHeight);
	mat4 matTrans = T(playerCamera.pos.x, playerCamera.pos.y - martinHeight, playerCamera.pos.z);

	vec3 cameraDirXZ = Normalize({playerCamera.lookingDir.x, 0.0f, playerCamera.lookingDir.z});
	float cameraAngle = atan2(cameraDirXZ.x, cameraDirXZ.z);
	// +0.6 random ass offset because the model is annoying
	mat4 matMtW = matTrans * Ry(cameraAngle + 0.6) * scaleMartin;

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(modelsVertexArrayObjID); 

	// DRAW SKYBOX
	drawSkybox();

	// DRAW GROUND
	drawModelWrapper(groundMtW, ground, grassTex);

	// DRAW MARTIN
	drawModelWrapper(matMtW, martin, martinTex);

	// DRAW MIRROR
	drawMirror({0.0, 5.0, 0.0}, {0, 0, 0}, camera);
}

void drawMirror(vec3 position, vec3 rotation, Camera3D camera)
{

	mat4 modelToWorld = T(position.x, position.y, position.z) * Rz(rotation.z) * Rx(rotation.x) * Ry(rotation.y);

	glDisable(GL_CULL_FACE);
	glUseProgram(mirrorProgram);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mirrorFBO[0]->texid);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glUniform1i(glGetUniformLocation(mirrorProgram, "mirrorCube"), 0);

	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	glUniformMatrix4fv(glGetUniformLocation(mirrorProgram, "worldToView"), 1, GL_TRUE, cameraMatrix.m);
	glUniform3f(glGetUniformLocation(mirrorProgram, "cameraPosition"), camera.pos.x, camera.pos.y, camera.pos.z);
	DrawModel(mirror, mirrorProgram, "inPosition", "inNormal", NULL);

	glEnable(GL_CULL_FACE);
}

Model* GenerateBumpmap(TextureData *tex)
{
	int vertexCount = tex->width * tex->height;
	int triangleCount = (tex->width-1) * (tex->height-1) * 2;
	unsigned int x, z;
	
	vec3 *vertexArray = (vec3 *)malloc(sizeof(GLfloat) * 3 * vertexCount);
	vec3 *normalArray = (vec3 *)malloc(sizeof(GLfloat) * 3 * vertexCount);
	vec2 *texCoordArray = (vec2 *)malloc(sizeof(GLfloat) * 2 * vertexCount);
	GLuint *indexArray = (GLuint *) malloc(sizeof(GLuint) * triangleCount*3);
	
	for (x = 0; x < tex->width; x++)
	{
		for (z = 0; z < tex->height; z++)
		{
			// Vertex array
			vertexArray[(x + z * tex->width)].x = x / SCALE_SIZE;
			vertexArray[(x + z * tex->width)].y = tex->imageData[(x + z * tex->width) * (tex->bpp/8)] / SCALE_HEIGHT;
			vertexArray[(x + z * tex->width)].z = z / SCALE_SIZE;
			// Normal vectors
			if (x == 0 or z == 0 or x == tex->width or z == tex->height)
			{
				normalArray[(x + z * tex->width)].x = 0.0;
				normalArray[(x + z * tex->width)].y = 1.0;
				normalArray[(x + z * tex->width)].z = 0.0;
			} 
			else 
			{
				vec3 a = vertexArray[(x + z * tex->width)];
				vec3 b = vertexArray[((x-1) + z * tex->width)];
				vec3 c = vertexArray[(x + (z-1) * tex->width)];
				vec3 norm = CalcNormalVector(b,a,c);
				normalArray[(x + z * tex->width)] = norm;
			}
			// Texture coordinates
			texCoordArray[(x + z * tex->width)].x = x; // (float)x / tex->width;
			texCoordArray[(x + z * tex->width)].y = z; // (float)z / tex->height;
		}
	}
	for (x = 0; x < tex->width-1; x++)
	{
		for (z = 0; z < tex->height-1; z++)
		{
			// Triangle 1
			indexArray[(x + z * (tex->width-1))*6 + 0] = x + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 1] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 2] = x+1 + z * tex->width;
			// Triangle 2
			indexArray[(x + z * (tex->width-1))*6 + 3] = x+1 + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 4] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 5] = x+1 + (z+1) * tex->width;
		}
	}
	Model* model = LoadDataToModel(
		vertexArray,
		normalArray,
		texCoordArray,
		NULL,
		indexArray,
		vertexCount,
		triangleCount*3
	);
	return model;
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 2);
	glutInitWindowPosition (10, 10);
	glutInitWindowSize(WINDOW_W, WINDOW_H);
	glutCreateWindow ("Martin doing the funny");
	glutDisplayFunc(display); 
	init();
	glutMainLoop();
	return 0;
}
