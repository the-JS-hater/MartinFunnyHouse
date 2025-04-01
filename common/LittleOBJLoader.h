#ifndef loadobj_h
#define loadobj_h

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#else
	#if defined(_WIN32)
		#include "glew.h"
	#endif
	#include <GL/gl.h>
#endif

#include "VectorUtils4.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

// How many error messages do you want before it stops?
#define NUM_DRAWMODEL_ERROR 8

typedef struct Mtl
{
	char newmtl[255];
	
	vec3 Ka, Kd, Ks, Ke;
	// I have also seen vec3's named Tf and a float called Ni in one file. What is that?
	// Tf = transmission filter
	// Ni = optical_density
	GLfloat Ns, Tr, d; // Tr = 1-d
	int illum;	// illumination model 0..10
	char map_Ka[255], map_Kd[255], map_Ks[255], map_Ke[255], map_Ns[255], map_d[255], map_bump[255];
	
	// NEW for texture support
	int texidKa, texidKd, texidKs, texidKe, texidNs, texid_d, texid_bump; // References to loaded textures!
	// refl -type not supported -o for options? A whole bunch.
	// Extensions for physically based rendering not supported
} Mtl, *MtlPtr, **MtlHandle;

typedef struct
{
  vec3* vertexArray;
  vec3* normalArray;
  vec2* texCoordArray;
  vec3* colorArray; // Rarely used
  GLuint* indexArray;
  int numVertices;
  int numIndices;
  char externalData;
  
  // Space for saving VBO and VAO IDs
  GLuint vao; // VAO
  GLuint vb, ib, nb, tb; // VBOs
  
  Mtl *material;
} Model;

// Basic model loading
#define LoadModelPlus LoadModel
Model* LoadModel(const char* name); // Load OBJ as single Model
Model** LoadModelSet(const char* name);  // Multi-part OBJ!

// Drawing models
void DrawModel(Model *m, GLuint program, const char* vertexVariableName, const char* normalVariableName, const char* texCoordVariableName);
void DrawWireframeModel(Model *m, GLuint program, const char* vertexVariableName, const char* normalVariableName, const char* texCoordVariableName);

// Utility functions that you may need if you want to modify the model.

Model* LoadDataToModel(
			vec3 *vertices,
			vec3 *normals,
			vec2 *texCoords,
			vec3 *colors,
			GLuint *indices,
			int numVert,
			int numInd);
void ReloadModelData(Model *m);

void CenterModel(Model *m);
void ScaleModel(Model *m, float sx, float sy, float sz);
void DisposeModel(Model *m);

//#ifdef __cplusplus
//}
///#endif

#endif

// --------------- Implementation part ----------------

#ifdef MAIN

#ifndef LOL_IMPLEMENTATION
#define LOL_IMPLEMENTATION

// Little OBJ loader
// Ingemar's little OBJ loader
// Formerly named LoadOBJ or loadobj
// by Ingemar Ragnemalm 2005, 2008, 2019, 2021

// Original version 2005 was extremely simple and intended for use with old OpenGL immediate mode.
// Additions and reorganization by Mikael Kalms 2008
// 120913: Revised LoadModelPlus/DrawModel by Jens.
// Partially corrected formatting. (It is a mess!)
// 130227: Error reporting in DrawModel
// 130422: Added ScaleModel
// 150909: Frees up temporary "Mesh" memory i LoadModel. Thanks to Simon Keisala for finding this!
// Added DisposeModel. Limited the number of error printouts, thanks to Rasmus Hytter for this suggestion!
// 160302: Uses fopen_s on Windows, as suggested by Jesper Post. Should reduce warnings a bit.
// 160510: Uses calloc instead of malloc (for safety) in many places where it could possibly cause problems.
// 170406: Added "const" to string arguments to make C++ happier.
// 190416: free mesh->coordStarts properly.
// 191110: Finally! Multi-part OBJ files work! Renamed LoadModelPlus to LoadModel.
// and added LoadModelSet! The old LoadModel was removed. Converted to use vec2/vec3
// instead of arrays of float. It is now dependent on VectorUtils3. If you want to
// make it independent of VectorUtils3, only a few structs and functions have to be replaced.
// MTL support working! Parser totally rewritten. Officially renamed to Ingemars Little OBJ Loader,
// LOL for short! And boy did I find a nice acronym!
// 210125: Corrected types in LoadDataToModel. Corrected material disposal in the still badly tested DisposeModel.
// 210218: Allows TAB between parts of string.
// 210304: Corrected bad allocation size on row 538, found by Jens Lindgren, who also found glDeleteVertexArrays in DisposeModel.
// 210422: Change by Felicia Castenbrandt, around row 415, copy path or source. This change is likely to be needed in some other places.
// 210502: Dispose the material name list, omission found by Jens Lindgren.
// 220222: Added a printed error message if ParseOBJ can't open a file. Also makes DrawModel always activare the shader for you.
// 2022 somewhere: Made experimental header only version, necessary to work with VectorUtils in C++.
// 2023-02-05: Added LOL_IMPLEMENTATION to avoid double compilation.

// Usage:
// Load simple models with LoadModel. Multi-part models are loaded with LoadModelSet.
// DrawModel will draw the model, or, for a multi-part model, one part.
// If you need to modify a model on the CPU, you can do that and re-upload
// with LoadDataToModel.
// There is no longer any call that only loads the data, but you can
// make your own by taking out the GPU upload from LoadModel.

// This is free software. I appreciate credits in derivate products or
// if it is used in public products.

//#include "LittleOBJLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "VectorUtils4.h"

#if defined(_WIN32)
	#define sscanf sscanf_s
#endif


static char atLineEnd = 0;
static char atFileEnd = 0;

static char ParseString(char *line, int *pos, char *s)
{
	int i = 0;
	char c;
	c = line[(*pos)++];
	while (c == 32 || c == 9) // Important change 210218
		c = line[(*pos)++]; // Skip leading spaces
	while (c != 0 && c != 32 && c != 9  && c != EOF && c != '/' && i < 254)
	{
		s[i++] = c;
		c = line[(*pos)++];
	}
	atLineEnd = (c == 0);
	s[i] = 0;
	
	return c;
}

static float ParseFloat(char *line, int *pos)
{
	float floatValue = 0.0;
	char s[255];
	ParseString(line, pos, s);
	sscanf(s, "%f", &floatValue);
	if (isnan(floatValue) || isinf(floatValue))
		floatValue = 0.0;
	return floatValue;
}

static int ParseInt(char *line, int *pos, char *endc)
{
	int intValue = 0;
	char s[255];
	char c = ParseString(line, pos, s);
	if (endc != NULL) *endc = c;
	if (strlen(s) == 0)
		intValue = -1; // Nothing found!
	else
		sscanf(s, "%d", &intValue);
	return intValue;
}

static vec3 ParseVec3(char *line, int *pos)
{
	vec3 v;
	v.x = ParseFloat(line, pos);
	v.y = ParseFloat(line, pos);
	v.z = ParseFloat(line, pos);
	return v;
}

void ParseLine(FILE *fp, char *line)
{
	int i, j;
	char c = '_';
	for (i = 0, j= 0; i < 2048; i++)
	{
		c = getc(fp);
		if (c != 10 && c != 13)
			line[j++] = c;
		if (c == EOF || ((c == 10 || c == 13) && j > 0))
		{
			if (c == EOF)
				atFileEnd = 1;
			line[j] = 0;
			return;
		}
	}
	line[j] = 0;
	return;
}


// Dispose the strings in the materials list
static void DisposeMtlList(Mtl **materials)
{
	if (materials != NULL)
	for (int i = 0; materials[i] != NULL; i++)
		free(materials[i]);
	free(materials);
}

static Mtl **ParseMTL(char *filename)
{
	Mtl *m;
	char s[255];
	char line[2048];
	int pos;
	Mtl **materials = NULL;
	FILE *fp;

	// It seems Windows/VS doesn't like fopen any more, but fopen_s is not on the others.
	#if defined(_WIN32)
	fopen_s(&fp, filename, "r");
	#else
	fp = fopen(filename, "rb"); // rw works everywhere except Windows?
	#endif
	if (fp == NULL)
		return NULL;
	atLineEnd = 0;
	atFileEnd = 0;
	int matcount = 0;

	while (!atFileEnd)
	{
		ParseLine(fp, line);
		pos = 0;
		ParseString(line, &pos, s);
		if (strcmp(s, "newmtl") == 0)
			matcount++;
	}
	rewind(fp);

	materials = (Mtl **)calloc(sizeof(MtlPtr)*(matcount+1), 1);

	matcount = 0;
	atLineEnd = 0;
	atFileEnd = 0;
	
	while (!atFileEnd)
	{
		ParseLine(fp, line);
		pos = 0;
		ParseString(line, &pos, s);

		// NEW MATERIAL!
		if (strcmp(s, "newmtl") == 0)
		{
			matcount++;
			materials[matcount-1] = (Mtl *)calloc(sizeof(Mtl),1);
			m = materials[matcount-1];
			materials[matcount] = NULL;
			
			ParseString(line, &pos, m->newmtl);
		}

		if (m != NULL)
		{
			if (strcmp(s, "Ka") == 0)
				m->Ka = ParseVec3(line, &pos);
			if (strcmp(s, "Kd") == 0)
				m->Kd = ParseVec3(line, &pos);
			if (strcmp(s, "Ks") == 0)
				m->Ks = ParseVec3(line, &pos);
			if (strcmp(s, "Ke") == 0)
				m->Ke = ParseVec3(line, &pos);
			if (strcmp(s, "Tr") == 0)
			{
				m->Tr = ParseFloat(line, &pos);
				m->d = 1 - m->Tr;
			}
			if (strcmp(s, "d") == 0)
			{
				m->d = ParseFloat(line, &pos);
				m->Tr = 1 - m->d;
			}

			if (strcmp(s, "illum") == 0)
				m->illum = ParseInt(line, &pos, NULL);

			if (strcmp(s, "map_Ka") == 0)
				ParseString(line, &pos, m->map_Ka);
			if (strcmp(s, "map_Kd") == 0)
				ParseString(line, &pos, m->map_Kd);
			if (strcmp(s, "map_Ks") == 0)
				ParseString(line, &pos, m->map_Ks);
			if (strcmp(s, "map_Ke") == 0)
				ParseString(line, &pos, m->map_Ke);
			if (strcmp(s, "map_d") == 0)
				ParseString(line, &pos, m->map_d);
			if (strcmp(s, "map_bump") == 0)
				ParseString(line, &pos, m->map_bump);
			if (strcmp(s, "bump") == 0)
				ParseString(line, &pos, m->map_bump);
		}
	}
	fclose(fp);

	return materials;
}

typedef struct Mesh
{
	vec3	*vertices;
	int		vertexCount;
	vec3	*vertexNormals;
	int		normalsCount;
	vec2	*textureCoords;
	int		texCount;
	
	int		*coordIndex;
	int		*normalsIndex;
	int		*textureIndex;
	int		coordCount; // Number of indices in each index struct
	
	// Borders between groups
	int		*coordStarts;
	int		groupCount;
	
	char	*materialName;
} Mesh, *MeshPtr;

// Globals set by LoadObj
static int vertCount, texCount, normalsCount, coordCount;
static int zeroFix;
static char hasPositionIndices;
static char hasNormalIndices;
static char hasTexCoordIndices;

// List of materials
Mtl **gMaterials;
char **gMtlNameList;
char *gMtlLibName = NULL;

static char ParseOBJ(const char *filename, MeshPtr theMesh)
{
	int lastCoordCount = -1;
	FILE *fp;
	
	// It seems Windows/VS doesn't like fopen any more, but fopen_s is not on the others.
	#if defined(_WIN32)
	fopen_s(&fp, filename, "r");
	#else
	fp = fopen(filename, "rb"); // rw works everywhere except Windows?
	#endif
	if (fp == NULL)
	{
		fprintf(stderr, "File \"%s\" could not be opened\n", filename);
		return -1;
	}

// These must be taken care of by the caller!
	vertCount=0;
	texCount=0;
	normalsCount=0;
	coordCount=0;

	atLineEnd = 0;
	atFileEnd = 0;
		
	while (!atFileEnd)
	{
		char line[2048];
		ParseLine(fp, line);
		int pos = 0;
		
		char s[256];
		ParseString(line, &pos, s);
		
		if (strcmp(s, "v") == 0)
		{
			vec3 v = ParseVec3(line, &pos);
			if (theMesh->vertices != NULL)
				theMesh->vertices[vertCount++] = v;
			else
				vertCount += 1;
		}
		if (strcmp(s, "vn") == 0)
		{
			vec3 v = ParseVec3(line, &pos);			
			if (theMesh->vertexNormals != NULL)
				theMesh->vertexNormals[normalsCount++] = v;
			else
				normalsCount += 1;
		}
		if (strcmp(s, "vt") == 0)
		{
			vec3 v = ParseVec3(line, &pos);
			if (theMesh->textureCoords != NULL)
			{
				theMesh->textureCoords[texCount].x = v.x;
				theMesh->textureCoords[texCount++].y = v.y;
			}
			else
				texCount += 1;
		}
		if (strcmp(s, "f") == 0)
		{
		// Unknown number of singeltons or triplets!
			
			int p = -1;
			int i = -1;
			int p1;
			char *ns;
			char done = 0;
			for (i = pos ; !done; pos++)
			{
				if (line[pos] == 0)
					done = 1;
				if (p >= 0) // in triplet)
				{
					char c = line[pos];
					if (c == '/' || c == ' ' || c == 0)
					{
						ns = &line[p1]; // Start of substring
						line[pos] = 0; // End of substring
						if (p1 != pos)
						{
							i = atoi(ns);
							if (i == 0)
								zeroFix = 1;
							if (i < 0)
							{
								switch (p)
								{
									case 0: i = vertCount + i + 1; break;
									case 1: i = texCount + i + 1; break;
									case 2: i = normalsCount + i + 1; break;
								}
							}
							i = i + zeroFix;
						}
						else
							i = 0; // -1
						
						if (i > 0)
						switch (p)
						{
							case 0:
								if (theMesh->coordIndex != NULL)
									theMesh->coordIndex[coordCount-1] = i-1;
								break;
							case 1:
								if (theMesh->textureIndex != NULL)
									if (i >= 0)
										theMesh->textureIndex[coordCount-1] = i-1;
								hasTexCoordIndices = 1;
								break;
							case 2:
								if (theMesh->normalsIndex != NULL)
									if (i >= 0)
										theMesh->normalsIndex[coordCount-1] = i-1;
								hasNormalIndices = 1;
								break;
						}
						p1 = pos + 1;
						p++;
					}
					if (c == ' ')
					{
						p = -1; // Next triplet
					}
				}
				else // Wait for non-space
				if (line[pos] != ' ')
				{
					if (!done)
						coordCount++;
					p = 0; // Triplet starts!
					p1 = pos;
				}
			}
			
			if (theMesh->coordIndex != NULL)
				theMesh->coordIndex[coordCount] = -1;
			if (theMesh->textureIndex != NULL)
				theMesh->textureIndex[coordCount] = -1;
			if (theMesh->normalsIndex != NULL)
				theMesh->normalsIndex[coordCount] = -1;
			coordCount++;
		}
		if (strcmp(s, "mtllib") == 0)
		{
			// Save file name for later
			char libname[256];
			
			// Added by Felicia Castenbrandt: Get path of main file, if any
        	char temp[255];
			int index = 0;

			//find index of last /
			for(unsigned int i = 0; i < strlen(filename); i++)
				if(filename[i] == '/')
					index = i;

			//copy everything before the last / to temp
			if (index != 0)
			{
				for(int i = 0; i < index+1; i++)
				{
					char ch = filename[i];
					strncat(temp, &ch, 1);
				}
			}
			// End of addition
			
			
			/*char c =*/ ParseString(line, &pos, libname);
			gMtlLibName = (char *)malloc(strlen(libname)+1);
#if defined(_WIN32)
			strcpy_s(gMtlLibName, strlen(libname) + 1, libname);
#else
			strcpy(gMtlLibName, libname);
#endif

		}
		if (strcmp(s, "usemtl") == 0)
		{
			// Groups are only identified by usemtl!
			
			if (coordCount > 0) // If no data has been seen, this must be the first group!
			{
				if (lastCoordCount != coordCount) // must not be the same; empty group (happens if a model has both "g" and "o")
				{
					theMesh->groupCount += 1;
					if (theMesh->coordStarts != NULL) // NULL if we are just counting
						theMesh->coordStarts[theMesh->groupCount] = coordCount;

					lastCoordCount = coordCount;
				}
				else
					printf("Ignored part!\n");
			}
			
			if (gMtlNameList != NULL)
			{
				// Save matname for later
				char matname[255];
				ParseString(line, &pos, matname);
				gMtlNameList[theMesh->groupCount] = (char *)malloc(strlen(matname)+1);
#if defined(_WIN32)
				strcpy_s(gMtlNameList[theMesh->groupCount], strlen(matname) + 1, matname);
#else
				strcpy(gMtlNameList[theMesh->groupCount], matname);
#endif


			}
		}
	} // While
	
	// Add last groups *if* there was more data since last time!
	if (coordCount > lastCoordCount)
	{
		theMesh->groupCount += 1; // End of last group
		if (theMesh->coordStarts != NULL)
			theMesh->coordStarts[theMesh->groupCount] = coordCount;
	}
		
	fclose(fp);
	
	if (gMtlLibName != NULL)
		gMaterials = ParseMTL(gMtlLibName);
	// else try based on filename: filename.obj -> filename.mtl or filename_obj.mtl
	if (gMaterials == NULL)
	if (strlen(filename) > 4)
	{
		char tryname[255];
		strcpy(tryname, filename);
		tryname[strlen(tryname) - 4] = '_';
		strcat(tryname, ".mtl");
		gMaterials = ParseMTL(tryname);
	}
	if (gMaterials == NULL)
	if (strlen(filename) > 4)
	{
		char tmpname[255];
		strcpy(tmpname, filename);
		tmpname[strlen(tmpname) - 4] = 0;
		strcat(tmpname, ".mtl");
		gMaterials = ParseMTL(tmpname);
	}
	return 0;
}

// Load raw, unprocessed OBJ data!
static struct Mesh * LoadOBJ(const char *filename)
{
	// Reads once to find sizes, again to fill buffers
	Mesh *theMesh;
	
	// Allocate Mesh but not the buffers
	theMesh = (Mesh *)calloc(sizeof(Mesh), 1);

	hasPositionIndices = 1;
	hasTexCoordIndices = 0;
	hasNormalIndices = 0;

	theMesh->coordStarts = NULL;
	theMesh->groupCount = 0;
	gMtlNameList = NULL;

// Dispose if they exist!
	gMtlLibName = NULL;
	gMaterials = NULL;

	vertCount=0;
	texCount=0;
	normalsCount=0;
	coordCount=0;
	zeroFix = 0;
		
	// Parse for size
	ParseOBJ(filename, theMesh);

	// Allocate arrays!
	if (vertCount > 0)
		theMesh->vertices = (vec3 *)malloc(sizeof(vec3) * vertCount);
	if (texCount > 0)
		theMesh->textureCoords = (vec2 *)malloc(sizeof(vec2) * texCount);
	if (normalsCount > 0)
		theMesh->vertexNormals = (vec3 *)malloc(sizeof(vec3) * normalsCount);
	if (hasPositionIndices)
		theMesh->coordIndex = (int *)calloc(coordCount, sizeof(int));
	if (hasNormalIndices)
		theMesh->normalsIndex = (int *)calloc(coordCount, sizeof(int));
	if (hasTexCoordIndices)
		theMesh->textureIndex = (int *)calloc(coordCount, sizeof(int));

	gMtlNameList = (char **)calloc(sizeof(char *) * (theMesh->groupCount+1), 1);
	theMesh->coordStarts = (int *)calloc(sizeof(int) * (theMesh->groupCount+2), 1); // Length found on last pass
	theMesh->groupCount = 0;

	// Zero again
	vertCount=0;
	texCount=0;
	normalsCount=0;
	coordCount=0;

	// Parse again for filling buffers
	ParseOBJ(filename, theMesh);
	
	theMesh->vertexCount = vertCount;
	theMesh->coordCount = coordCount;
	theMesh->texCount = texCount;
	theMesh->normalsCount = normalsCount;

	return theMesh;
}

void DecomposeToTriangles(struct Mesh *theMesh)
{
	int i, vertexCount, triangleCount;
	int *newCoords, *newNormalsIndex, *newTextureIndex;
	int newIndex = 0; // Index in newCoords
	int first = 0;

	// 1.1 Calculate how big the list will become
	
	vertexCount = 0; // Number of vertices in current polygon
	triangleCount = 0; // Resulting number of triangles
	for (i = 0; i < theMesh->coordCount; i++)
	{
		if (theMesh->coordIndex[i] == -1)
		{
		if (vertexCount > 2) triangleCount += vertexCount - 2;
			vertexCount = 0;
		}
		else
		{
			vertexCount = vertexCount + 1;
		}
	}
		
	newCoords = (int *)calloc(triangleCount * 3, sizeof(int));
	if (theMesh->normalsIndex != NULL)
		newNormalsIndex = (int *)calloc(triangleCount * 3, sizeof(int));
	if (theMesh->textureIndex != NULL)
		newTextureIndex = (int *)calloc(triangleCount * 3, sizeof(int));
	
	// 1.2 Loop through old list and write the new one
	// Almost same loop but now it has space to write the result
	vertexCount = 0;
	for (i = 0; i < theMesh->coordCount; i++)
	{
		if (theMesh->coordIndex[i] == -1)
		{
			first = i + 1;
			vertexCount = 0;
		}
		else
		{
			vertexCount = vertexCount + 1;
			if (vertexCount > 2)
			{
				newCoords[newIndex++] = theMesh->coordIndex[first];
				newCoords[newIndex++] = theMesh->coordIndex[i-1];
				newCoords[newIndex++] = theMesh->coordIndex[i];
				if (theMesh->normalsIndex != NULL)
				{
					newNormalsIndex[newIndex-3] = theMesh->normalsIndex[first];
					newNormalsIndex[newIndex-2] = theMesh->normalsIndex[i-1];
					newNormalsIndex[newIndex-1] = theMesh->normalsIndex[i];
				}
				if (theMesh->textureIndex != NULL)
				{
					newTextureIndex[newIndex-3] = theMesh->textureIndex[first];
					newTextureIndex[newIndex-2] = theMesh->textureIndex[i-1];
					newTextureIndex[newIndex-1] = theMesh->textureIndex[i];
				}
			}
		}
	}
	
	free(theMesh->coordIndex);
	theMesh->coordIndex = newCoords;
	theMesh->coordCount = triangleCount * 3;
	if (theMesh->normalsIndex != NULL)
	{
		free(theMesh->normalsIndex);
		theMesh->normalsIndex = newNormalsIndex;
	}
	if (theMesh->textureIndex != NULL)
	{
		free(theMesh->textureIndex);
		theMesh->textureIndex = newTextureIndex;
	}
} // DecomposeToTriangles

static void GenerateNormals(Mesh* mesh)
{
	// If model has vertices but no vertexnormals, generate normals
	if (mesh->vertices && !mesh->vertexNormals)
	{
		int face;
		int normalIndex;

		mesh->vertexNormals = (vec3 *)calloc(sizeof(vec3) * mesh->vertexCount, 1);
		mesh->normalsCount = mesh->vertexCount;

		mesh->normalsIndex = (int *)calloc(mesh->coordCount, sizeof(GLuint));
		memcpy(mesh->normalsIndex, mesh->coordIndex, sizeof(GLuint) * mesh->coordCount);

		for (face = 0; face * 3 < mesh->coordCount; face++)
		{
			int i0 = mesh->coordIndex[face * 3 + 0];
			int i1 = mesh->coordIndex[face * 3 + 1];
			int i2 = mesh->coordIndex[face * 3 + 2];
			
			vec3 v0 = VectorSub(mesh->vertices[i1], mesh->vertices[i0]);
			vec3 v1 = VectorSub(mesh->vertices[i2], mesh->vertices[i0]);
			vec3 v2 = VectorSub(mesh->vertices[i2], mesh->vertices[i1]);

			float sqrLen0 = dot(v0, v0);
			float sqrLen1 = dot(v1, v1);
			float sqrLen2 = dot(v2, v2);

			float len0 = (sqrLen0 >= 1e-6f) ? sqrt(sqrLen0) : 1e-3f;
			float len1 = (sqrLen1 >= 1e-6f) ? sqrt(sqrLen1) : 1e-3f;
			float len2 = (sqrLen2 >= 1e-6f) ? sqrt(sqrLen2) : 1e-3f;

			float influence0 = dot(v0, v1) / (len0 * len1);
			float influence1 = -dot(v0, v2) / (len0 * len2);
			float influence2 = dot(v1, v2) / (len1 * len2);

			float angle0 = (influence0 >= 1.f) ? 0 : 
				(influence0 <= -1.f) ? M_PI : acos(influence0);
			float angle1 = (influence1 >= 1.f) ? 0 : 
				(influence1 <= -1.f) ? M_PI : acos(influence1);
			float angle2 = (influence2 >= 1.f) ? 0 : 
				(influence2 <= -1.f) ? M_PI : acos(influence2);

			vec3 normal = cross(v0, v1);
			mesh->vertexNormals[i0] = VectorAdd(ScalarMult(normal, angle0), mesh->vertexNormals[i0]);
			mesh->vertexNormals[i1] = VectorAdd(ScalarMult(normal, angle1), mesh->vertexNormals[i1]);
			mesh->vertexNormals[i2] = VectorAdd(ScalarMult(normal, angle2), mesh->vertexNormals[i2]);
		}

		for (normalIndex = 0; normalIndex < mesh->normalsCount; normalIndex++)
		{
			float length = Norm(mesh->vertexNormals[normalIndex]);
			if (length > 0.01f)
				mesh->vertexNormals[normalIndex] = ScalarMult(mesh->vertexNormals[normalIndex], 1/length);
		}
	}
}

static Model* GenerateModel(Mesh* mesh)
{
	// Convert from Mesh format (multiple index lists) to Model format
	// (one index list) by generating a new set of vertices/indices
	// and where new vertices have been created whenever necessary

	typedef struct
	{
		int positionIndex;
		int normalIndex;
		int texCoordIndex;
		int newIndex;
	} IndexTriplet;

	int hashGap = 6;

	int indexHashMapSize = (mesh->vertexCount * hashGap + mesh->coordCount);

	IndexTriplet* indexHashMap = (IndexTriplet *)malloc(sizeof(IndexTriplet)
							* indexHashMapSize);

	int numNewVertices = 0;
	int index;

	int maxValue = 0;
		
	Model* model = (Model *)malloc(sizeof(Model));
	memset(model, 0, sizeof(Model));

	model->indexArray = (GLuint *)malloc(sizeof(GLuint) * mesh->coordCount);
	model->numIndices = mesh->coordCount;

	memset(indexHashMap, 0xff, sizeof(IndexTriplet) * indexHashMapSize);

	for (index = 0; index < mesh->coordCount; index++)
	{
		IndexTriplet currentVertex = { -1, -1, -1, -1 };
		int insertPos = 0;
		if (mesh->coordIndex)
			currentVertex.positionIndex = mesh->coordIndex[index];
		if (mesh->normalsIndex)
			currentVertex.normalIndex = mesh->normalsIndex[index];
		if (mesh->textureIndex)
			currentVertex.texCoordIndex = mesh->textureIndex[index];

		if (maxValue < currentVertex.texCoordIndex)
			maxValue = currentVertex.texCoordIndex;
 
		if (currentVertex.positionIndex >= 0)
			insertPos = currentVertex.positionIndex * hashGap;

		while (1)
		{
			if (indexHashMap[insertPos].newIndex == -1)
				{
					currentVertex.newIndex = numNewVertices++;
					indexHashMap[insertPos] = currentVertex;
					break;
				}
			else if (indexHashMap[insertPos].positionIndex
				 == currentVertex.positionIndex
				 && indexHashMap[insertPos].normalIndex
				 == currentVertex.normalIndex
				 && indexHashMap[insertPos].texCoordIndex
				 == currentVertex.texCoordIndex)
				{
					currentVertex.newIndex = indexHashMap[insertPos].newIndex;
					break;
				}
			else
				insertPos++;
		} 

		model->indexArray[index] = currentVertex.newIndex;
	}

	if (mesh->vertices)
		model->vertexArray = (vec3 *)malloc(sizeof(vec3) * numNewVertices);
	if (mesh->vertexNormals)
		model->normalArray = (vec3 *)malloc(sizeof(vec3) * numNewVertices);
	if (mesh->textureCoords)
		model->texCoordArray = (vec2 *)malloc(sizeof(vec2) * numNewVertices);
	
	model->numVertices = numNewVertices;

	for (index = 0; index < indexHashMapSize; index++)
	{
		if (indexHashMap[index].newIndex != -1)
		{
			if (mesh->vertices)
				model->vertexArray[indexHashMap[index].newIndex] = mesh->vertices[indexHashMap[index].positionIndex];
			if (mesh->vertexNormals)
			{
				model->normalArray[indexHashMap[index].newIndex] = mesh->vertexNormals[indexHashMap[index].normalIndex];
			}
			if (mesh->textureCoords)
				model->texCoordArray[indexHashMap[index].newIndex] = mesh->textureCoords[indexHashMap[index].texCoordIndex];
		}
	}

	free(indexHashMap);

	// If there is a material set, match materials to parts
	if (gMaterials != NULL)
	if (mesh->materialName != NULL)
		for (int ii = 0; gMaterials[ii] != NULL; ii++)
		{
			Mtl *mtl = gMaterials[ii];
			if (strcmp(mesh->materialName, mtl->newmtl) == 0)
			{
				// Copy mtl to model!
				model->material = (Mtl *)malloc(sizeof(Mtl));
				memcpy(model->material, mtl, sizeof(Mtl));

				strcpy(model->material->map_Ka, mtl->map_Ka);
				strcpy(model->material->map_Kd, mtl->map_Kd);
				strcpy(model->material->map_Ks, mtl->map_Ks);
				strcpy(model->material->map_Ke, mtl->map_Ke);
				strcpy(model->material->map_Ns, mtl->map_Ns);
				strcpy(model->material->map_d, mtl->map_d);
				strcpy(model->material->map_bump, mtl->map_bump);
			}
		}

	return model;
}

Mesh **SplitToMeshes(Mesh *m)
{
	int * mapc = (int *)malloc(m->vertexCount * sizeof(int));
	int * mapt = (int *)malloc(m->texCount * sizeof(int));
	int * mapn = (int *)malloc(m->normalsCount * sizeof(int));
	
	if (m == NULL || m ->vertices == NULL || m->vertexCount == 0)
	{
		printf("Illegal mesh!\n");
		return NULL;
	}
	
	Mesh **mm = (Mesh **)calloc(sizeof(Mesh *), m->groupCount+2);

	for (int mi = 0; mi < m->groupCount; mi++) // For all sections
	{
		int from = m->coordStarts[mi];
		int to = m->coordStarts[mi+1];
		
		mm[mi] = (Mesh *)calloc(sizeof(Mesh), 1); // allocate struct
		
		// Fill mapc, mapt, mapn with -1 (illegal index)
		for (int ii = 0; ii < m->vertexCount; ii++)
			mapc[ii] = -1;
		for (int ii = 0; ii < m->texCount; ii++)
			mapt[ii] = -1;
		for (int ii = 0; ii < m->normalsCount; ii++)
			mapn[ii] = -1;
		
		// Count number of entries needed
		int intVertexCount = 0;
		int intTexCount = 0;
		int intNormalsCount = 0;
		
		for (int j = from; j < to; j++) // For all index triplets
		{
			int ix = m->coordIndex[j];
			if (ix > -1)
				if (mapc[ix] == -1)
				{
					mapc[ix] = ix;
					intVertexCount++;
				}
			if (m->textureIndex != NULL)
			{
				ix = m->textureIndex[j];
				if (ix > -1)
					if (mapt[ix] == -1)
					{
						mapt[ix] = ix;
						intTexCount++;
					}
			}
			if (m->normalsIndex != NULL)
			{
				ix = m->normalsIndex[j];
				if (ix > -1)
					if (mapn[ix] == -1)
					{
						mapn[ix] = ix;
						intNormalsCount++;
					}
			}
		}
				
		// Allocate buffers
		mm[mi]->coordIndex = (int *)malloc((to - from) * sizeof(int));
		mm[mi]->textureIndex = (int *)malloc((to - from) * sizeof(int));
		mm[mi]->normalsIndex = (int *)malloc((to - from) * sizeof(int));
		if (intVertexCount > 0)
			mm[mi]->vertices = (vec3 *)malloc(intVertexCount * sizeof(vec3));
		if (intTexCount > 0)
			mm[mi]->textureCoords = (vec2 *)malloc(intTexCount * sizeof(vec2));
		if (intNormalsCount > 0)
			mm[mi]->vertexNormals = (vec3 *)malloc(intNormalsCount * sizeof(vec3));
		mm[mi]->vertexCount = intVertexCount;
		mm[mi]->texCount = intTexCount;
		mm[mi]->normalsCount = intNormalsCount;
		
		// Fill mapc, mapt, mapn with -1 (illegal index)
		for (int ii = 0; ii < m->vertexCount; ii++)
			mapc[ii] = -1;
		for (int ii = 0; ii < m->texCount; ii++)
			mapt[ii] = -1;
		for (int ii = 0; ii < m->normalsCount; ii++)
			mapn[ii] = -1;
		
		int mapcCount = 0;
		int maptCount = 0;
		int mapnCount = 0;
		for (int j = from; j < to; j++) // For all index triplets
		{
			int ix = m->coordIndex[j];
			if (ix > -1)
			{
				if (mapc[ix] == -1) // Unmapped
				{
					mapc[ix] = mapcCount++;
					mm[mi]->vertices[mapc[ix]] = m->vertices[ix]; // Copy vertex to mm[i]
				}
				mm[mi]->coordIndex[j-from] = mapc[ix];
			}
			else // Separator
			{
				mm[mi]->coordIndex[j-from] = -1;
			}

			if (m->textureIndex != NULL)
			if (mm[mi]->textureIndex != NULL)
			{
				ix = m->textureIndex[j];
				if (ix > -1)
				{
					if (mapt[ix] == -1) // Unmapped
					{
						mapt[ix] = maptCount++;
						mm[mi]->textureCoords[mapt[ix]] = m->textureCoords[ix]; // Copy vertex to mm[i]
					}
					mm[mi]->textureIndex[j-from] = mapt[ix];
				}
				else // Separator
				{
					mm[mi]->textureIndex[j-from] = -1;
				}
			}

			if (m->normalsIndex != NULL)
			if (mm[mi]->normalsIndex != NULL)
			{
				ix = m->normalsIndex[j];
				if (ix > -1)
				{
					if (mapn[ix] == -1) // Unmapped
					{
						mapn[ix] = mapnCount++;
						mm[mi]->vertexNormals[mapn[ix]] = m->vertexNormals[ix]; // Copy vertex to mm[i]
					}
					mm[mi]->normalsIndex[j-from] = mapn[ix];
				}
				else // Separator
				{
					mm[mi]->normalsIndex[j-from] = -1;
				}
			}
		} // for all index triplets
		mm[mi]->coordCount = to - from;
		
		if (gMtlNameList != NULL)
		{
			mm[mi]->materialName = gMtlNameList[mi];
			gMtlNameList[mi] = NULL; // No longer "owned" by the gMtlNameList
		}
	} // for all parts
	
	return mm;
}


static void DisposeMesh(Mesh *mesh)
{
// Free the mesh!
	if (mesh != NULL)
	{
		if (mesh->vertices != NULL)
			free(mesh->vertices);
		if (mesh->vertexNormals != NULL)
			free(mesh->vertexNormals);
		if (mesh->textureCoords != NULL)
			free(mesh->textureCoords);
		if (mesh->coordIndex != NULL)
			free(mesh->coordIndex);
		if (mesh->normalsIndex != NULL)
			free(mesh->normalsIndex);
		if (mesh->textureIndex != NULL)
			free(mesh->textureIndex);
#if !defined(_WIN32)
// This is very disturbing, causes heap corruption on Windows. Reason not found.
		if (mesh->coordStarts != NULL)
			free(mesh->coordStarts);
#endif
		if (mesh->materialName != NULL)
			free(mesh->materialName);

// Dispose the material name list too
		if (gMtlNameList != NULL)
		{
			for (int i = 0; i < mesh->groupCount; i++)
				if (gMtlNameList[i] != NULL)
					free(gMtlNameList[i]);
			free(gMtlNameList);
			gMtlNameList = NULL;
		}

		free(mesh);
	}
}

void CenterModel(Model *m)
{
	int i;
	float maxx = -1e10, maxy = -1e10, maxz = -1e10, minx = 1e10, miny = 1e10, minz = 1e10;
	
	for (i = 0; i < m->numVertices; i++)
	{
		if (m->vertexArray[i].x < minx) minx = m->vertexArray[i].x;
		if (m->vertexArray[i].x > maxx) maxx = m->vertexArray[i].x;
		if (m->vertexArray[i].y < miny) miny = m->vertexArray[i].y;
		if (m->vertexArray[i].y > maxy) maxy = m->vertexArray[i].y;
		if (m->vertexArray[i].z < minz) minz = m->vertexArray[i].z;
		if (m->vertexArray[i].z > maxz) maxz = m->vertexArray[i].z;
	}
	
	for (i = 0; i < m->numVertices; i++)
	{
		m->vertexArray[i].x -= (maxx + minx)/2.0f;
		m->vertexArray[i].y -= (maxy + miny)/2.0f;
		m->vertexArray[i].z -= (maxz + minz)/2.0f;
	}
}

void ScaleModel(Model *m, float sx, float sy, float sz)
{
	long i;
	for (i = 0; i < m->numVertices; i++)
	{
		m->vertexArray[i].x *= sx;
		m->vertexArray[i].y *= sy;
		m->vertexArray[i].z *= sz;
	}
}

static void LOLError(const char *caller, const char *name)
{
	static unsigned int draw_error_counter = 0; 
   if(draw_error_counter < NUM_DRAWMODEL_ERROR)
   {
		    fprintf(stderr, "%s warning: '%s' not found in shader!\n", caller, name);
		    draw_error_counter++;
   }
   else if(draw_error_counter == NUM_DRAWMODEL_ERROR)
   {
		    fprintf(stderr, "%s: Number of error bigger than %i. No more vill be printed.\n", caller, NUM_DRAWMODEL_ERROR);
		    draw_error_counter++;
   }
}

// This code makes a lot of calls for rebinding variables just in case,
// and to get attribute locations. This is clearly not optimal, but the
// goal is stability.

void DrawModel(Model *m, GLuint program, const char* vertexVariableName, const char* normalVariableName, const char* texCoordVariableName)
{
	if (m != NULL)
	{
		GLint loc;
		
		glBindVertexArray(m->vao);	// Select VAO
		glUseProgram(program); // Added 2022-03

		glBindBuffer(GL_ARRAY_BUFFER, m->vb);
		loc = glGetAttribLocation(program, vertexVariableName);
		if (loc >= 0)
		{
			glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, 0); 
			glEnableVertexAttribArray(loc);
		}
		else
			LOLError("DrawModel", vertexVariableName);
		
		if (normalVariableName!=NULL)
		{
			loc = glGetAttribLocation(program, normalVariableName);
			if (loc >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, m->nb);
				glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(loc);
			}
			else
				LOLError("DrawModel", normalVariableName);
		}
	
		if ((m->texCoordArray != NULL)&&(texCoordVariableName != NULL))
		{
			loc = glGetAttribLocation(program, texCoordVariableName);
			if (loc >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, m->tb);
				glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(loc);
			}
			else
				LOLError("DrawModel", texCoordVariableName);
		}

		glDrawElements(GL_TRIANGLES, m->numIndices, GL_UNSIGNED_INT, 0L);
	}
}

void DrawWireframeModel(Model *m, GLuint program, const char* vertexVariableName, const char* normalVariableName, const char* texCoordVariableName)
{
	if (m != NULL)
	{
		GLint loc;
		
		glBindVertexArray(m->vao);	// Select VAO
		glUseProgram(program); // Added 2022-03

		glBindBuffer(GL_ARRAY_BUFFER, m->vb);
		loc = glGetAttribLocation(program, vertexVariableName);
		if (loc >= 0)
		{
			glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, 0); 
			glEnableVertexAttribArray(loc);
		}
		else
			LOLError("DrawWireframeModel", vertexVariableName);
		
		if (normalVariableName!=NULL)
		{
			loc = glGetAttribLocation(program, normalVariableName);
			if (loc >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, m->nb);
				glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(loc);
			}
			else
				LOLError("DrawWireframeModel", normalVariableName);
		}
	
		if ((m->texCoordArray != NULL)&&(texCoordVariableName != NULL))
		{
			loc = glGetAttribLocation(program, texCoordVariableName);
			if (loc >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, m->tb);
				glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(loc);
			}
			else
				LOLError("DrawWireframeModel", texCoordVariableName);
		}
		glDrawElements(GL_LINE_STRIP, m->numIndices, GL_UNSIGNED_INT, 0L);
	}
}
	
// Called from LoadModel, LoadModelSet and LoadDataToModel
// VAO and VBOs must already exist!
// Useful by its own when the model changes on CPU
void ReloadModelData(Model *m)
{
	glBindVertexArray(m->vao);
	
	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, m->vb);
	glBufferData(GL_ARRAY_BUFFER, m->numVertices*3*sizeof(GLfloat), m->vertexArray, GL_STATIC_DRAW);
	
	// VBO for normal data
	glBindBuffer(GL_ARRAY_BUFFER, m->nb);
	glBufferData(GL_ARRAY_BUFFER, m->numVertices*3*sizeof(GLfloat), m->normalArray, GL_STATIC_DRAW);
	
	// VBO for texture coordinate data NEW for 5b
	if (m->texCoordArray != NULL)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m->tb);
		glBufferData(GL_ARRAY_BUFFER, m->numVertices*2*sizeof(GLfloat), m->texCoordArray, GL_STATIC_DRAW);
	}
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->numIndices*sizeof(GLuint), m->indexArray, GL_STATIC_DRAW);
}

static void GenModelBuffers(Model *m)
{
	glGenVertexArrays(1, &m->vao);
	glGenBuffers(1, &m->vb);
	glGenBuffers(1, &m->ib);
	glGenBuffers(1, &m->nb);
	if (m->texCoordArray != NULL)
		glGenBuffers(1, &m->tb);

	ReloadModelData(m);
}

// For simple models
Model* LoadModel(const char* name)
{
	Model* model = NULL;
	Mesh* mesh = LoadOBJ(name);
	DecomposeToTriangles(mesh);
	GenerateNormals(mesh);
	model = GenerateModel(mesh);
	DisposeMesh(mesh);

	GenModelBuffers(model);
	model->externalData = 0;
	return model;
}

// For multiple part models
Model** LoadModelSet(const char* name)
{
	Mesh* mesh = LoadOBJ(name);
	Mesh **mm = SplitToMeshes(mesh);
	int i, ii;
	for (i = 0; mm[i] != NULL; i++) {}
	Model **md = (Model **)calloc(sizeof(Model *), i+1);
	for (ii = 0; mm[ii] != NULL; ii++)
	{
		DecomposeToTriangles(mm[ii]);
		GenerateNormals(mm[ii]);
		md[ii] = GenerateModel(mm[ii]);
		DisposeMesh(mm[ii]);
	}
	free(mm);
	DisposeMtlList(gMaterials);
	DisposeMesh(mesh);
	gMtlNameList = NULL;
	gMaterials = NULL;
	
	for (int i = 0; md[i] != NULL; i++)
	{
		GenModelBuffers(md[i]);
		md[i]->externalData = 0;
	}

	return md;
}

// Loader for inline data to Model (almost same as LoadModelPlus)
Model* LoadDataToModel(
			vec3 *vertices,
			vec3 *normals,
			vec2 *texCoords,
			vec3 *colors,
			GLuint *indices,
			int numVert,
			int numInd)
{
	Model* m = (Model *)malloc(sizeof(Model));
	memset(m, 0, sizeof(Model));
	
	m->vertexArray = vertices;
	m->texCoordArray = texCoords;
	m->normalArray = normals;
	m->indexArray = indices;
	m->numVertices = numVert;
	m->numIndices = numInd;
	m->externalData = 1;
	
	GenModelBuffers(m);
	
	return m;
}

// Cleanup function, not tested!
void DisposeModel(Model *m)
{
	if (m != NULL)
	{
		if (m->externalData == 0)
		{
			if (m->vertexArray != NULL)
				free(m->vertexArray);
			if (m->normalArray != NULL)
				free(m->normalArray);
			if (m->texCoordArray != NULL)
				free(m->texCoordArray);
			if (m->colorArray != NULL) // obsolete?
				free(m->colorArray);
			if (m->indexArray != NULL)
				free(m->indexArray);
		}
		
		// Lazy error checking here since "glDeleteBuffers silently ignores 0's and names that do not correspond to existing buffer objects."
		glDeleteBuffers(1, &m->vb);
		glDeleteBuffers(1, &m->ib);
		glDeleteBuffers(1, &m->nb);
		glDeleteBuffers(1, &m->tb);
		glDeleteVertexArrays(1, &m->vao);
		
		if (m->material != NULL)
			free(m->material);
	}
	free(m);
}

#endif
#endif
