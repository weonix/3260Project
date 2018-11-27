/*********************************************************
FILE : main.cpp (csci3260 2018-2019 Project)
*********************************************************/
/*********************************************************
Student Information
Student ID: 1155092438 & 1155093276
Student Name: Ling Yiu & Cheung Kam Shing
*********************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include "Dependencies\glew\glew.h"
#include "Dependencies\freeglut\freeglut.h"
#include "Dependencies\glm\glm.hpp"
#include "Dependencies\glm\gtc\matrix_transform.hpp"
#include "Dependencies\glm\gtc\type_ptr.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <mmsystem.h>
#include <time.h>

using namespace std;
using glm::vec3;
using glm::mat4;

GLint programID;
// Could define the Vao&Vbo and interaction parameter here
//maximum number of models and texture
const int NUM_OF_OBJECT = 10;
const int NUM_OF_TEXTURE = 10;

//constant bit flags for indicating status of objects
const int ST_VISIBLE = 2;
const int ST_COLLIDABLE = 4;

//bound for the plane movement
const float RIGHT_BOUND = 20.0f;
const float LEFT_BOUND = -20.0f;
const float TOP_BOUND = -20.0f;
const float BOT_BOUND = 20.0f;

//storing the index of the entities
const float FLY_SPEED = 0.7f;
const float ROCK_ORBIT_RATE = 0.01f;
const float PLANET_TURN_RATE = 0.001f;
const float ROCK_TURN_RATE = 0.0007f;
int Floor;
int SpaceCraft;
int Planet1, Planet2;
int RingStart, RingEnd;
int RockStart, RockEnd;
int Sun;

//stroring gobal game state variables

float mouseX;
float mouseY;
glm::vec3 camPos = glm::vec3(0.0,20.0,20.0);
float camY = 0.0;
float camX = 45.0;
float t = 0.0f;
vec3 lightPosition;
float diff = 1.0; //diffuse light intensity
float spec = 1.0; //specular light intensity

//vao vbos
GLuint textureID[NUM_OF_TEXTURE];
GLuint VertexArrayID[NUM_OF_OBJECT];
GLuint vertexbuffer[NUM_OF_OBJECT];
GLuint uvbuffer[NUM_OF_OBJECT];
GLuint normalbuffer[NUM_OF_OBJECT];
GLuint drawSize[NUM_OF_OBJECT];

//entities system
typedef struct entity {
	int objID;
	int texture;
	float collisionRadius;
	int collisionHandler;
	vec3 location;
	glm::mat4 transform; //for aditional scaling or rotational visual transform
	int status;
	float orbitRadius;
	float radian;
	float orbitVOffset;
	vec3 orbitCentre;
	glm::mat4 scale;
}entity;

entity* EntityList[100];
int entityCount = 0;


//custom function prototypes
void bufferObject(int objectID, const char * Path);
void setupLight();
void drawTextureObject(int index, int Texture, glm::mat4 transformMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
int initEntity(int objID, int texture, int x, int y, int z, float radius, int collisionHandler);
void drawEntity(entity* e, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
int checkCollision(entity * e1, entity * e2);
int handleCollision(entity * primary, entity * secondary);
int initEntity(int objID, int texture, int x, int y, int z, glm::mat4 transform, float radius, int collisionHandler);
int initRock(float radiusMin, float radiusMax, float vOffset, vec3 centre);
GLuint loadBMP_data(const char * imagepath, unsigned char ** image, int *width, int *height);


//a series utilities for setting shader parameters 
void setMat4(const std::string &name, glm::mat4& value)
{
	unsigned int transformLoc = glGetUniformLocation(programID, name.c_str());
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(value));
}

void setVec4(const std::string &name, glm::vec4 value)
{
	glUniform4fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
}
void setVec3(const std::string &name, glm::vec3 value)
{
	glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
}
void setFloat(const std::string &name, float value)
{
	glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}
void setInt(const std::string &name, int value)
{
	glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

bool checkStatus(
	GLuint objectID,
	PFNGLGETSHADERIVPROC objectPropertyGetterFunc,
	PFNGLGETSHADERINFOLOGPROC getInfoLogFunc,
	GLenum statusType)
{
	GLint status;
	objectPropertyGetterFunc(objectID, statusType, &status);
	if (status != GL_TRUE)
	{
		GLint infoLogLength;
		objectPropertyGetterFunc(objectID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* buffer = new GLchar[infoLogLength];

		GLsizei bufferSize;
		getInfoLogFunc(objectID, infoLogLength, &bufferSize, buffer);
		cout << buffer << endl;

		delete[] buffer;
		return false;
	}
	return true;
}

bool checkShaderStatus(GLuint shaderID)
{
	return checkStatus(shaderID, glGetShaderiv, glGetShaderInfoLog, GL_COMPILE_STATUS);
}

bool checkProgramStatus(GLuint programID)
{
	return checkStatus(programID, glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS);
}

string readShaderCode(const char* fileName)
{
	ifstream meInput(fileName);
	if (!meInput.good())
	{
		cout << "File failed to load..." << fileName;
		exit(1);
	}
	return std::string(
		std::istreambuf_iterator<char>(meInput),
		std::istreambuf_iterator<char>()
	);
}

void installShaders()
{
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar* adapter[1];
	string temp = readShaderCode("VertexShaderCode.glsl");
	adapter[0] = temp.c_str();
	glShaderSource(vertexShaderID, 1, adapter, 0);
	temp = readShaderCode("FragmentShaderCode.glsl");
	adapter[0] = temp.c_str();
	glShaderSource(fragmentShaderID, 1, adapter, 0);

	glCompileShader(vertexShaderID);
	glCompileShader(fragmentShaderID);

	if (!checkShaderStatus(vertexShaderID) || !checkShaderStatus(fragmentShaderID))
		return;

	programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	if (!checkProgramStatus(programID))
		return;

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	glUseProgram(programID);
}



void keyboard(unsigned char key, int x, int y)
{
	if(key == 'f')
		lightPosition = lightPosition + vec3(-2.0f, 0.0f, 0.0f);
	else if (key == 'h')
		lightPosition = lightPosition + vec3(2.0f, 0.0f, 0.0f);
	else if (key == 'g')
		lightPosition = lightPosition + vec3(0.0f, 0.0f, 2.0f);
	else if (key == 't')
		lightPosition = lightPosition + vec3(0.0f, 0.0f, -2.0f);
	else if (key == 'r')
		lightPosition = lightPosition + vec3(0.0f, 2.0f, 0.0f);
	else if (key == 'y')
		lightPosition = lightPosition + vec3(0.0f, -2.0f, 0.0f);
	else if (key == 'q') {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4(0.0f, FLY_SPEED, 0.0f, 1.0f));
	}
	else if (key == 'e') {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4(0.0f, -FLY_SPEED, 0.0f, 1.0f));
	}
}

void move(int key, int x, int y)
{
	if (key == GLUT_KEY_RIGHT) {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4(FLY_SPEED, 0.0f, 0.0f, 1.0f));		
	}
	else if (key == GLUT_KEY_LEFT) {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4( -FLY_SPEED, 0.0f, 0.0f, 1.0f));
	}
	else if (key == GLUT_KEY_UP) {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4(0.0f, 0.0f, -FLY_SPEED, 1.0f));
	}
	else if (key == GLUT_KEY_DOWN) {
		EntityList[SpaceCraft]->location =
			vec3(glm::translate(glm::mat4(), vec3(EntityList[SpaceCraft]->location)) * EntityList[SpaceCraft]->transform *  glm::vec4(0.0f, 0.0f, FLY_SPEED, 1.0f));
	}


}
int oldx = 0;
float r = 0.0f;
void PassiveMouse(int x, int y)
{
	//TODO: Use Mouse to do interactive events and animation
	mouseX = x;
	mouseY = y;
	if (x<oldx)
	{
		r = 2.0f;
		EntityList[SpaceCraft]->transform *= glm::rotate(glm::mat4(1.0f), glm::radians(r), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	if (x>oldx)
	{
		r = -2.0f;
		EntityList[SpaceCraft]->transform *= glm::rotate(glm::mat4(1.0f), glm::radians(r), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	oldx = x;
}

bool loadOBJ(
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
) {
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 6 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

				   // else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y;
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else {
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}

	return true;
}

GLuint loadBMP_custom(const char * imagepath) {

	printf("Reading image %s\n", imagepath);

	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	unsigned char * data;

	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; }

	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		return 0;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}
	if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    return 0; }
	if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    return 0; }

	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	if (imageSize == 0)    imageSize = width * height * 3;
	if (dataPos == 0)      dataPos = 54;

	data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);


	GLuint textureID = 0;
	//TODO: Create one OpenGL texture and set the texture parameter 


	glGenTextures(1, &textureID);
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
		GL_UNSIGNED_BYTE, data);
	// OpenGL has now copied the data. Free our own version
	delete[] data;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	return textureID;
}

GLuint loadBMP_data(const char * imagepath, unsigned char** image, int *widthR, int *heightR) {

	printf("Reading image %s\n", imagepath);

	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	unsigned char * data;

	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; }

	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		return 0;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}
	if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    return 0; }
	if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    return 0; }

	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	if (imageSize == 0)    imageSize = width * height * 3;
	if (dataPos == 0)      dataPos = 54;

	data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);


	GLuint textureID = 0;
	//TODO: Create one OpenGL texture and set the texture parameter 


	glGenTextures(1, &textureID);
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
		GL_UNSIGNED_BYTE, data);
	// OpenGL has now copied the data. Free our own version
	delete[] data;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);


	return textureID;
}


void sendDataToOpenGL()
{
	//Generate buffers
	glGenVertexArrays(NUM_OF_OBJECT, VertexArrayID);
	glGenBuffers(NUM_OF_OBJECT, vertexbuffer);
	glGenBuffers(NUM_OF_OBJECT, uvbuffer);
	glGenBuffers(NUM_OF_OBJECT, normalbuffer);

	//Load texture
	textureID[0] = loadBMP_custom("sources\\jeep_texture.bmp");
	textureID[1] = loadBMP_custom("sources\\block_texture.bmp");
	textureID[2] = loadBMP_custom("sources\\texture\\spacecraftTexture.bmp");
	textureID[3] = loadBMP_custom("sources\\texture\\WonderStarTexture.bmp");
	textureID[4] = loadBMP_custom("sources\\texture\\RockTexture.bmp");
	textureID[5] = loadBMP_custom("sources\\texture\\earthTexture.bmp");
	textureID[6] = loadBMP_custom("sources\\white_texture.bmp");
	textureID[7] = loadBMP_custom("sources\\texture\\ringTexture.bmp");
	//Load obj files
	//bufferObject(0, "sources\\block.obj");
	bufferObject(1, "sources\\spaceCraft.obj");
	bufferObject(2, "sources\\plane.obj");
	bufferObject(3, "sources\\rock.obj");
	bufferObject(4, "sources\\planetCentered.obj");
	bufferObject(5, "sources\\ringCentered.obj");
	//bufferObject(6, "sources\\jeep.obj");
}

mat4 LookAtRH(vec3 eye, vec3 target, vec3 up)
{
	vec3 zaxis = glm::normalize(eye - target);    // The "forward" vector.
	vec3 xaxis = glm::normalize(cross(up, zaxis));// The "right" vector.
	vec3 yaxis = cross(zaxis, xaxis);     // The "up" vector.

										  // Create a 4x4 orientation matrix from the right, up, and forward vectors
										  // This is transposed which is equivalent to performing an inverse 
										  // if the matrix is orthonormalized (in this case, it is).
	mat4 orientation = {
		glm::vec4(xaxis.x, yaxis.x, zaxis.x, 0),
		glm::vec4(xaxis.y, yaxis.y, zaxis.y, 0),
		glm::vec4(xaxis.z, yaxis.z, zaxis.z, 0),
		glm::vec4(0,       0,       0,     1)
	};

	// Create a 4x4 translation matrix.
	// The eye position is negated which is equivalent
	// to the inverse of the translation matrix. 
	// T(v)^-1 == T(-v)
	mat4 translation = {
		glm::vec4(1,      0,      0,   0),
		glm::vec4(0,      1,      0,   0),
		glm::vec4(0,      0,      1,   0),
		glm::vec4(-eye.x, -eye.y, -eye.z, 1)
	};

	// Combine the orientation and translation to compute 
	// the final view matrix. Note that the order of 
	// multiplication is reversed because the matrices
	// are already inverted.
	return (orientation * translation);
}

void paintGL(void)
{
	//General Upkeepings
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.8f, 0.8f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	t += 0.1;
	setupLight();

	//Set transformation matrix

	//set up projection matrix
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::perspective((float)glm::radians(90.0f), 1.0f / 1.0f, 0.5f, 200.0f);

	//send eye position
	GLint eyePosUniformLocation = glGetUniformLocation(programID, "eyePositionWorld");
	glm::vec4 campos4v = glm::vec4(camPos, 0.0);
	glUniform4fv(eyePosUniformLocation, 1, &campos4v[0]);

	//set up view matrix
	glm::mat4 viewMatrix = LookAtRH(camPos, EntityList[SpaceCraft]->location, vec3(0.0f, 1.0f, 0.0f));//glm::mat4(1.0f);
	//viewMatrix = glm::translate(mat4(), -camPos) * viewMatrix;
	//viewMatrix = glm::inverse(EntityList[Plane]->transform) * viewMatrix;//glm::rotate(mat4(), glm::radians(camY), glm::vec3(0.0f, 0.0f, 1.0f));
	//viewMatrix = glm::rotate(mat4(), glm::radians(camX), glm::vec3(1.0f, 0.0f, 0.0f)) * viewMatrix;


	//update entity state and location etc
	//make a sphere follow the light source
	EntityList[Sun]->location = lightPosition;
	//make the camera follow the plane
	//camPos = EntityList[Plane]->location;
	camPos = vec3(EntityList[SpaceCraft]->transform * glm::translate(glm::mat4(),vec3(0.0f,+10.0f,+10.0f)) * glm::vec4(1.0));
	camPos = vec3(glm::translate(glm::mat4(), EntityList[SpaceCraft]->location) * glm::vec4(camPos, 1.0));

	//make the rock oribts
	for (int i = RockStart; i <= RockEnd; i++) {
		EntityList[i]->radian += ROCK_ORBIT_RATE / EntityList[i]->orbitRadius;
		EntityList[i]->location = vec3(glm::translate(glm::mat4(), EntityList[i]->orbitCentre)
			* glm::translate(glm::mat4(), vec3(0.0f, EntityList[i]->orbitVOffset, 0.0f))
			* glm::rotate(mat4(), EntityList[i]->radian, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(), vec3(EntityList[i]->orbitRadius, 0.0f, 0.0f))
			* glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		EntityList[i]->transform = glm::rotate(mat4(), ROCK_TURN_RATE, glm::vec3(0.0f, 1.0f, 0.0f)) *  EntityList[i]->transform;
	}

	//rotate the planets
	EntityList[Planet1]->transform = glm::rotate(mat4(), PLANET_TURN_RATE, glm::vec3(0.0f, 1.0f, 0.0f)) *  EntityList[Planet1]->transform;
	EntityList[Planet2]->transform = glm::rotate(mat4(), PLANET_TURN_RATE, glm::vec3(0.0f, 1.0f, 0.0f)) *  EntityList[Planet2]->transform;

	//centralised drawing and collision detection for each entity
	for (int i = 0; i < entityCount; i++) {
		if (EntityList[i]->status & ST_VISIBLE)
			drawEntity(EntityList[i], viewMatrix, projectionMatrix);
		if (EntityList[i]->status & ST_COLLIDABLE) {
			for (int j = i; j < entityCount; j++) {
				if (EntityList[j]->status & ST_COLLIDABLE)
					checkCollision(EntityList[i], EntityList[j]);
			}
		}

	}

	//post drawing upkeeping
	glFlush();
	glutPostRedisplay();
}

void setupLight()
{
	//Set up lighting information
	GLint lightPositonUniformLocation = glGetUniformLocation(programID, "lightPositionWorld");
	glUniform3fv(lightPositonUniformLocation, 1, &lightPosition[0]);

	GLint ambLightUniformLocation = glGetUniformLocation(programID, "ambientLight");
	glm::vec4 ambientLight(0.25f, 0.25f, 0.25f, 1.0f);
	glUniform4fv(ambLightUniformLocation, 1, &ambientLight[0]);


	GLint diffuseLightUniformLocation = glGetUniformLocation(programID, "diffuseLight");
	glm::vec4 diffuseLight(diff, diff, diff, 1.0f);
	glUniform4fv(diffuseLightUniformLocation, 1, &diffuseLight[0]);

	GLint specularLightUniformLocation = glGetUniformLocation(programID, "specularLight");
	glm::vec4 specularLight(spec, spec, spec, 1.0f);
	glUniform4fv(specularLightUniformLocation, 1, &specularLight[0]);
}

void bufferObject(int objectID, const char* Path) {
	//read from an obj file and send its data to the VAO VBOS at the designated ID
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(Path, vertices, uvs, normals);

	glBindVertexArray(VertexArrayID[objectID]);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[objectID]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
		&vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, // attribute
		3, // size
		GL_FLOAT, // type
		GL_FALSE, // normalized?
		0, // stride
		(void*)0 // array buffer offset
	);

	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer[objectID]);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0],
		GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, // attribute
		2, // size
		GL_FLOAT, // type
		GL_FALSE, // normalized?
		0, // stride
		(void*)0 // array buffer offset
	);

	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer[objectID]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0],
		GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, // attribute
		3, // size
		GL_FLOAT, // type
		GL_FALSE, // normalized?
		0, // stride
		(void*)0 // array buffer offset
	);

	drawSize[objectID] = vertices.size();
}

void drawTextureObject(int index, int Texture, glm::mat4 transformMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
	//function to help draw object, given obj ID, texture and various transform matrix
	glBindVertexArray(VertexArrayID[index]);

	GLint modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
	glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &transformMatrix[0][0]);

	GLint viewUniformLocation = glGetUniformLocation(programID, "viewMatrix");
	glUniformMatrix4fv(viewUniformLocation, 1, GL_FALSE, &viewMatrix[0][0]);

	GLint projectionMatrixUniformLocation = glGetUniformLocation(programID, "projectionMatrix");
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	GLuint localTextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID[Texture]);
	glUniform1i(localTextureID, 0);

	glDrawArrays(GL_TRIANGLES, 0, drawSize[index]);

}

int initEntity(int objID, int texture, int x, int y, int z, float radius, int collisionHandler) {
	return initEntity(objID, texture, x, y, z, glm::mat4(1.0f), radius, collisionHandler);
}

int initEntity(int objID, int texture, int x, int y, int z, glm::mat4 transform, float radius, int collisionHandler)
{
	//initialise entities
	entity* e = (entity*)malloc(sizeof(entity));
	e->objID = objID;
	e->texture = texture;
	e->location = vec3(x, y, z);
	e->transform = transform;
	e->collisionRadius = radius;
	e->collisionHandler = collisionHandler;
	EntityList[entityCount] = e;
	entityCount++;
	e->status = ST_VISIBLE | ST_COLLIDABLE;
	e->scale = glm::mat4(1.0f);
	return entityCount - 1;
}

int initRock(float radiusMin, float radiusMax, float vOffset, vec3 centre) {
	float turnx, turny, turnz;
	turnx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 360;
	turny = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 360;
	turnz = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 360;
	int i = initEntity(3, 4, 0, 0, 0, 2, 2);
	EntityList[i]->transform = glm::rotate(mat4(), glm::radians(turnx), glm::vec3(1.0f, 0.0f, 0.0f))
		*glm::rotate(mat4(), glm::radians(turny), glm::vec3(0.0f, 1.0f, 0.0f))
		*glm::rotate(mat4(), glm::radians(turnz), glm::vec3(0.0f, 0.0f, 1.0f));

	EntityList[i]->orbitRadius = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * (radiusMax - radiusMin)  + radiusMin;
	EntityList[i]->radian = glm::radians(static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 360);
	EntityList[i]->orbitVOffset = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2 * vOffset - vOffset;
	EntityList[i]->orbitCentre = centre;

	EntityList[i]->location = vec3(glm::translate(glm::mat4(), EntityList[i]->orbitCentre)
		* glm::translate(glm::mat4(), vec3(0.0f, EntityList[i]->orbitVOffset, 0.0f))
		* glm::rotate(mat4(), EntityList[i]->radian, glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::translate(glm::mat4(), vec3(EntityList[i]->orbitRadius, 0.0f, 0.0f))
		* glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	return i;
}

void drawEntity(entity* e, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	modelTransformMatrix = glm::translate(mat4(), glm::vec3(e->location.x, e->location.y, e->location.z)) * e->transform * e->scale;
	drawTextureObject(e->objID, e->texture, modelTransformMatrix, viewMatrix, projectionMatrix);
}

void initializedGL(void) //run only once
{
	glewInit();
	installShaders();
	sendDataToOpenGL();
}


void initialiseEntities() {

	srand(time(NULL));
	SpaceCraft = initEntity(1, 2, 0, 10, 0, 2.0f, 1); //the plane
	//EntityList[Plane]->scale = glm::scale(glm::mat4(), glm::vec3(0.01, 0.01, 0.01));
	Floor = initEntity(2, 0, 0, 0, 0, glm::scale(glm::mat4(), glm::vec3(3, 1, 4.9)), 2.0f, 0); // the floor
	EntityList[Floor]->status = EntityList[Floor]->status ^ ST_COLLIDABLE;
	Planet1 = initEntity(4, 5, -100, 10, 0, 2.0f, 2); // the earth
	
	Planet2 = initEntity(4, 3, +100, 10, 0, 2.0f, 2); // the wonder planet
	Sun = initEntity(4, 6, 0, 30, 0, 2.0f, 0); // sphere that indicate light position
	lightPosition = vec3(0.0f, 30.0f, 0.0f);
	EntityList[Sun]->scale = glm::scale(glm::mat4(), glm::vec3(0.010, 0.0110, 0.0110));
	RockStart = entityCount; //initialise all the rocks
	for (int i = 0; i < 20; i++) {
		RockEnd = initRock(50.0f, 60.0f, 5.0f, EntityList[Planet2]->location);
	}

	RingStart = entityCount;//initialise all the rings
	for (int i = 0; i < 3; i++) {
		RingEnd = initEntity(5, 7, 0, +10, -10 + -15 * i,  glm::rotate(mat4(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 2.0f, 3);
	}
	//glm::scale(glm::mat4(), vec3(0.15,0.15,0.15))	*

}

int checkCollision(entity* e1, entity* e2) {
	vec3 distance = e1->location - e2->location;
	if (glm::length(distance) < e1->collisionRadius + e2->collisionRadius) {
		handleCollision(e1, e2);
		handleCollision(e2, e1);
	}
	return 0;
}

int handleCollision(entity* primary, entity* secondary) {
	if (primary->collisionHandler == 1 && secondary->collisionHandler == 2) {
		

	}
	return 0;
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitWindowSize(900, 900);
	glutCreateWindow("Assignment 2");
	initialiseEntities();
	//TODO:
	/*Register different CALLBACK function for GLUT to response
	with different events, e.g. window sizing, mouse click or
	keyboard stroke */
	initializedGL();
	glutDisplayFunc(paintGL);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(move);
	glutPassiveMotionFunc(PassiveMouse);

	glutMainLoop();

	return 0;

}