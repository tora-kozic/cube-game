//CSCI5607 Final Project
//Tora Kozic - kozic009 - 5404403
//May 2021

const char* INSTRUCTIONS = " ";

//Mac OS build: g++ multiObjectTest.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o MultiObjTest
//Linux build:  g++ multiObjectTest.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o MultiObjTest

#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#include <SDL_image.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "objects.h"

#include <time.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
using namespace std;

#pragma region [GLOBAL PARAMS]

// world params
//int grid_width = 16;	// y
//int grid_height = 16;	// z
//int grid_depth = 16;	// x
//float grid_scale = 0.3;

int screenWidth = 1600; 
int screenHeight = 1000;
float curTime = 0;
float lastTime = 0;
float timePast = 0;

// camera params
float yaw = 90.0f;
float pitch = 0.0f;

// player data
ObjData playerData = ObjData(1, 0, 0, 1, 1, 1, .5, 1.4, .5, .2);
glm::vec3 cameraFront = glm::vec3(0, 0, -1.0f);
glm::vec3 camUp = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 camPos = glm::vec3(0.6f + playerData.objx, playerData.objy, playerData.objz);
bool canJump = true;

// world data
BlockData blockData = BlockData();
float gravity = -0.15;
int curr_block = 2; // default dirt
float bottom = -30.0f;

bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

#pragma endregion

// forward declaration
void drawGeometry(int shaderProgram, Model cube, BlockData blockData, glm::vec3 lookat, glm::vec3 camDir);
glm::vec3 PositionToIdx(glm::vec3 pos);
glm::vec3 GetLookIndex(glm::vec3 camPos, glm::vec3 camFront);
bool CheckIndexValid(glm::vec3 i);
glm::vec3 GetLookHelper(glm::vec3 camPos, glm::vec3 camFront, glm::vec3 lookat_index, glm::vec3 plane_origin);

int main(int argc, char *argv[]){
	srand(time(NULL)); // initialize random seed

	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 35, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}

	// load cube model
	ifstream modelFile;
	modelFile.open("../models/cube.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* cubeModel = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> cubeModel[i];
	}
	//printf("%d\n", numLines);
	int numVertsCube = numLines / 8;
	modelFile.close();

	// copy cube model into model data
	float* modelData = new float[(numVertsCube)*8];
	copy(cubeModel, cubeModel + numVertsCube * 8, modelData);

	Model cube = Model(0, numVertsCube, cubeModel);

#pragma region [TEXTURE ALLOCATION]

	//// **********************
	//// ALLOCATE TEXTURES
	//// **********************
	// 0 = stone
	// 1 = dirt
	// 2 = grass top
	// 3 = grass side
	// 4 = coal
	// 5 = iron
	// 6 = diamond
	// 7 = bedrock
	// 8 = log top
	// 9 = log side
	// 10 = leaves
	// 11 = planks
	// 12 = cobblestone

	//// Allocate Texture 0 (Stone) ///////
	SDL_Surface* surface = IMG_Load("../textures/stone.png");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex0;
    glGenTextures(1, &tex0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    //Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_FreeSurface(surface);
    //// End Allocate Texture ///////

	//// Allocate Texture 1 (Dirt) ///////
	SDL_Surface* surface1 = IMG_Load("../textures/dirt.png");
	if (surface1==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex1;
    glGenTextures(1, &tex1);
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE1);
    
    glBindTexture(GL_TEXTURE_2D, tex1);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w,surface1->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,surface1->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_FreeSurface(surface1);
	// End Allocate Texture ///////

	//// Allocate Texture 2 (Grass top) ///////
	SDL_Surface* surface2 = IMG_Load("../textures/grass_full.png");
	if (surface2 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex2;
	glGenTextures(1, &tex2);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE2);

	glBindTexture(GL_TEXTURE_2D, tex2);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface2->w, surface2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface2->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface2);
	//// End Allocate Texture ///////

	//// Allocate Texture 3 (Grass side) ///////
	SDL_Surface* surface3 = IMG_Load("../textures/grass.png");
	if (surface3 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex3;
	glGenTextures(1, &tex3);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE3);

	glBindTexture(GL_TEXTURE_2D, tex3);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface3->w, surface3->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface3->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface3);
	//// End Allocate Texture ///////

	//// Allocate Texture 4 (Coal ore) ///////
	SDL_Surface* surface4 = IMG_Load("../textures/coal.png");
	if (surface4 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex4;
	glGenTextures(1, &tex4);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE4);

	glBindTexture(GL_TEXTURE_2D, tex4);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface4->w, surface4->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface4->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface4);
	//// End Allocate Texture ///////

	//// Allocate Texture 5 (Iron ore) ///////
	SDL_Surface* surface5 = IMG_Load("../textures/iron.png");
	if (surface5 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex5;
	glGenTextures(1, &tex5);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE5);

	glBindTexture(GL_TEXTURE_2D, tex5);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface5->w, surface5->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface5->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface5);
	//// End Allocate Texture ///////

	//// Allocate Texture 6 (Diamond ore) ///////
	SDL_Surface* surface6 = IMG_Load("../textures/diamond.png");
	if (surface6 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex6;
	glGenTextures(1, &tex6);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE6);

	glBindTexture(GL_TEXTURE_2D, tex6);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface6->w, surface6->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface6->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface6);
	//// End Allocate Texture ///////

	//// Allocate Texture 7 (Bedrock) ///////
	SDL_Surface* surface7 = IMG_Load("../textures/bedrock.png");
	if (surface7 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex7;
	glGenTextures(1, &tex7);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE7);

	glBindTexture(GL_TEXTURE_2D, tex7);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface7->w, surface7->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface7->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface7);
	//// End Allocate Texture ///////

	//// Allocate Texture 8 (Log Top) ///////
	SDL_Surface* surface8 = IMG_Load("../textures/log.png");
	if (surface8 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex8;
	glGenTextures(1, &tex8);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE8);

	glBindTexture(GL_TEXTURE_2D, tex8);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface8->w, surface8->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface8->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface8);
	//// End Allocate Texture ///////

	//// Allocate Texture 9 (Log Side) ///////
	SDL_Surface* surface9 = IMG_Load("../textures/log_side.png");
	if (surface9 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex9;
	glGenTextures(1, &tex9);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE9);

	glBindTexture(GL_TEXTURE_2D, tex9);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface9->w, surface9->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface9->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface9);
	//// End Allocate Texture ///////

	//// Allocate Texture 10 (leaf) ///////
	SDL_Surface* surface10 = IMG_Load("../textures/leaf.png");
	if (surface10 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex10;
	glGenTextures(1, &tex10);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE10);

	glBindTexture(GL_TEXTURE_2D, tex10);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface10->w, surface10->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface10->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface10);
	//// End Allocate Texture ///////

	//// Allocate Texture 11 (planks) ///////
	SDL_Surface* surface11 = IMG_Load("../textures/planks.png");
	if (surface11 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex11;
	glGenTextures(1, &tex11);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE11);

	glBindTexture(GL_TEXTURE_2D, tex11);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface11->w, surface11->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface11->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface11);
	//// End Allocate Texture ///////
	
	//// Allocate Texture 12 (cobblestone) ///////
	SDL_Surface* surface12 = IMG_Load("../textures/cobblestone.png");
	if (surface12 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex12;
	glGenTextures(1, &tex12);

	//Load the texture into memory
	glActiveTexture(GL_TEXTURE12);

	glBindTexture(GL_TEXTURE_2D, tex12);
	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface12->w, surface12->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface12->pixels);
	glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

	SDL_FreeSurface(surface12);
	//// End Allocate Texture ///////
#pragma endregion 

#pragma region [VBO & VAO]
	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)

	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, numVertsCube*8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used
	
	int texturedShader = InitShader("../textured-Vertex.glsl", "../textured-Fragment.glsl");	
	
	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);
	
	//GLint colAttrib = glGetAttribLocation(phongShader, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);
	
	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	

#pragma endregion
	
	glEnable(GL_DEPTH_TEST);  

	printf("%s\n",INSTRUCTIONS);
	
#pragma region [WORLD DATA]

	// populate world generation data 
	int chance;
	int i, j, k;
	// initialize with flat world generation
	for (i = 0; i < grid_depth; i++) {
		for (j = 0; j < grid_width; j++) {
			for (k = 0; k < grid_height; k++) {
				chance = rand() % 99;
				if (i == 0) blockData.blocks[i][j][k] = 7; // bedrock
				else if (i == 1) {
					if (chance < 8) blockData.blocks[i][j][k] = 6; // diamond
					else blockData.blocks[i][j][k] = 1; // stone
				}
				else if (i == 2) {
					if (chance < 15) blockData.blocks[i][j][k] = 5; // iron
					else blockData.blocks[i][j][k] = 1;
				}
				else if (i == 3) {
					if (chance < 30) blockData.blocks[i][j][k] = 4; // coal
					else if (chance < 30 + 15 / 2) blockData.blocks[i][j][k] = 5;
					else blockData.blocks[i][j][k] = 1;
				}
				else if (i == 4) {
					if (chance < 75) blockData.blocks[i][j][k] = 2; // dirt
					else if (chance < 80) blockData.blocks[i][j][k] = 4;
					else blockData.blocks[i][j][k] = 1;
				}
				else if (i == 5) {
					if (chance < 30) blockData.blocks[i][j][k] = 1;
					else blockData.blocks[i][j][k] = 2;
				}
				else if (i == 6) {
					blockData.blocks[i][j][k] = 2;
				}
				else if (i == 7) {
					blockData.blocks[i][j][k] = 3; // grass
				}
				else blockData.blocks[i][j][k] = 0;
			}
		}
	}
	// generate landscape depth
	for (i = 1; i < grid_width-1; i++) {
		for (j = 1; j < grid_height-1; j++) {
			chance = rand() % 999;
			if (chance < 25) {
				blockData.blocks[9][i][j] = 3; // grass
				int m, n;
				for (m = -1; m <= 1; m++) {
					for (n = -1; n <= 1; n++) {
						chance = rand() % 99;
						if (chance < 50) {
							if (m == 0)	blockData.blocks[9][i][j + n] = 3;
							if (n == 0) blockData.blocks[9][i + m][j] = 3;
						}
					}
				}
			}
		}
	}
	for (i = 1; i < grid_width; i++) {
		for (j = 1; j < grid_height; j++) {
			if (blockData.blocks[9][i][j] != 0) {
				blockData.blocks[8][i][j] = 2;
				int m, n;
				for (m = -2; m <= 2; m++) {
					for (n = -2; n <= 2; n++) {
						chance = rand() % 99;
						// boundary check
						if (i + m < 0 || i + m > grid_width) continue;
						if (j + n < 0 || j + n > grid_height) continue;
						if (m == -2 || m == 2 || n == -2 || n == 2) {
							if (m != n && m + n != 0) {
								if (chance < 20) blockData.blocks[8][i + m][j + n] = 3;
							}
						}
						else {
							if (chance < 90) {
								blockData.blocks[8][i + m][j + n] = 3;
							}
						}
					}
				}
			}
		}
	}
	for (i = 0; i < grid_width; i++) {
		for (j = 0; j < grid_height; j++) {
			if (blockData.blocks[8][i][j] != 0) {
				blockData.blocks[7][i][j] = 2;
			}
		}
	}
	// generate trees
	int trees = rand() % 5 + 3;
	for (int t = 0; t < trees; t++) {
		int x, y, z;
		x = 7;
		y = rand() % 27 + 2;
		z = rand() % 27 + 2;
		while (blockData.blocks[x][y][z] != 0) x += 1;
		
		// draw tree
		// draw logs
		blockData.blocks[x][y][z] = 8;
		blockData.blocks[x + 1][y][z] = 8;
		blockData.blocks[x + 2][y][z] = 8;
		blockData.blocks[x + 3][y][z] = 8;
		blockData.blocks[x + 4][y][z] = 9;
		// draw leaves
		for (i = -1; i <= 1; i++) {
			for (j = -1; j <= 1; j++) {
				if (i != 0 || j != 0) blockData.blocks[x + 3][y + i][z + j] = 9;
				if (i != j && i + j != 0) blockData.blocks[x + 4][y + i][z + j] = 9;
			}
		}

	}

#pragma endregion

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_Event windowEvent;
	bool quit = false;
	glm::vec3 wDir, aDir, sDir, dDir;
	wDir = glm::vec3(0, 0, 0);
	aDir = glm::vec3(0, 0, 0);
	sDir = glm::vec3(0, 0, 0);
	dDir = glm::vec3(0, 0, 0);
	while (!quit){
		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue
			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) 
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){ //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}

			//SJG: Use key input to change the state of the object
			//     We can use the ".mod" flag to see if modifiers such as shift are pressed
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_d){ //If "down key" is pressed
				//playerData.velz = playerData.linSpeed;
				dDir = glm::normalize(glm::cross(cameraFront, camUp)) * playerData.linSpeed;
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_d) {
				dDir = glm::vec3(0, 0, 0);
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_a){ //If "up key"/w is pressed
				//playerData.velz = playerData.linSpeed * -1;
				aDir = glm::normalize(glm::cross(cameraFront, camUp)) * playerData.linSpeed * glm::vec3(-1, -1, -1);
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_a) {
				aDir = glm::vec3(0, 0, 0);
				//playerData.velz = 0;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_w) { //If "left key"/a is pressed
				wDir = playerData.linSpeed * cameraFront;
				//playerData.vely = playerData.linSpeed * -1;
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_w) {
				//playerData.vely = 0;
				wDir = glm::vec3(0, 0, 0);
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_s){ //If "right key"/d is pressed
				//playerData.vely = playerData.linSpeed;
				sDir = playerData.linSpeed * cameraFront * glm::vec3(-1,-1,-1);
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_s) {
				//playerData.vely = 0;
				sDir = glm::vec3(0, 0, 0);
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_p) { //If "p" is pressed print position
				cout << "Player's current coordinate position: " << playerData.objx << ", " << playerData.objy << ", " << playerData.objz << endl;
				glm::vec3 test = PositionToIdx(glm::vec3(playerData.objx, playerData.objy, playerData.objz));
				cout << "Player Indices: (" << test.x << ", " << test.y << ", " << test.z << ") " << endl;
			}
			if (canJump) {
				if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_SPACE) { // If space is pressed & can jump, jump
					playerData.velx += 2.2;
					canJump = false;
				}
			}
			if (windowEvent.type == SDL_MOUSEMOTION) {
				const float sens = 0.1f;

				float xoffset, yoffset;
				xoffset = windowEvent.motion.xrel;
				yoffset = windowEvent.motion.yrel;

				yaw -= xoffset * sens;
				pitch -= yoffset * sens;
				if (pitch > 89.0f)
					pitch = 89.0f;
				if (pitch < -89.0f)
					pitch = -89.0f;
			}
			if (windowEvent.type == SDL_MOUSEBUTTONUP && windowEvent.button.button == SDL_BUTTON_LEFT) { //If mouse left click
				glm::vec3 look_index = GetLookIndex(camPos, cameraFront);
				//cout << "left mouse clicked" << endl;
				if (CheckIndexValid(look_index)) {
					//cout << "(" << look_index.x << ", " << look_index.y << ", " << look_index.z << ")" << endl;
					if (blockData.blocks[(int)look_index.x][(int)look_index.y][(int)look_index.z] != 7) {
						curr_block = blockData.blocks[(int)look_index.x][(int)look_index.y][(int)look_index.z];  // set hand block to last broken
						if (curr_block == 3) curr_block = 2; // turn grass into dirt
						if (curr_block == 8) curr_block = 10; // turn log into wood
						if (curr_block == 1) curr_block = 11; // turn stone into cobble
						blockData.blocks[(int)look_index.x][(int)look_index.y][(int)look_index.z] = 0;
					}
				}
			}
			if (windowEvent.type == SDL_MOUSEBUTTONUP && windowEvent.button.button == SDL_BUTTON_RIGHT) { //If mouse right click
				glm::vec3 look_index = GetLookIndex(camPos, cameraFront);
				if (CheckIndexValid(look_index)) {
					if (blockData.blocks[(int)look_index.x + 1][(int)look_index.y][(int)look_index.z] == 0) {
						blockData.blocks[(int)look_index.x + 1][(int)look_index.y][(int)look_index.z] = curr_block;
					}
				}
			}
		}
		// check if player has fallen off map
		if (playerData.objx < bottom) {
			cout << "You've fallen off the map!" << endl;
			quit = true;
		}


		playerData.velx += gravity;
		playerData.vely = wDir.y + aDir.y + sDir.y + dDir.y;
		playerData.velz = wDir.z + aDir.z + sDir.z + dDir.z;

		glm::vec3 player_pos = glm::vec3(
			playerData.objx + (playerData.velx * timePast),
			playerData.objy + (playerData.vely * timePast),
			playerData.objz + (playerData.velz * timePast));
		glm::vec3 curr_pos = glm::vec3(playerData.objx, playerData.objy, playerData.objz);
		glm::vec3 player_idx = PositionToIdx(player_pos);
		glm::vec3 curr_idx = PositionToIdx(curr_pos);
		int xBlock, yBlock, zBlock;
		xBlock = blockData.getXCollision(player_pos, player_idx);
		yBlock = blockData.getYCollision(playerData.vely, curr_pos, curr_idx, player_pos, player_idx);
		zBlock = blockData.getZCollision(playerData.velz, curr_pos, curr_idx, player_pos, player_idx);

		if (xBlock != 0 && playerData.velx < 0) {
			playerData.velx = 0;
			canJump = true;
		}
		if (yBlock != 0) { 
			if (playerData.vely < 0) playerData.vely = .001; 
			else if (playerData.vely > 0) playerData.vely = -.001; 
			//playerData.vely = 0; 
		}
		if (zBlock != 0) {
			if (playerData.velz < 0) playerData.velz = .001; 
			else if (playerData.velz > 0) playerData.velz = -.001;
			//playerData.velz = 0;
		}

		playerData.objx += playerData.velx * timePast;
		playerData.objy += playerData.vely * timePast;
		playerData.objz += playerData.velz * timePast;
      
		// camera stuff
		glm::vec3 camDir = glm::vec3(0, 0, 0);
		camDir.x = sin(glm::radians(pitch));
		camDir.y = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		camDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(camDir);
		camPos = glm::vec3(0.6f + playerData.objx, playerData.objy, playerData.objz);
		//glm::vec3 look_point = glm::vec3(camPos + cameraFront);
		
		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);

		lastTime = curTime;
		curTime = SDL_GetTicks()/1000.f; 
		timePast = curTime - lastTime;

		// set view vector
		glm::mat4 view = glm::lookAt(
			// View map
			//glm::vec3(3.f, 0.f, 0.f),		//Cam Position
			//glm::vec3(0.0f, 0.0f, 0.0f),  //Look at point
			//glm::vec3(0.0f, 0.0f, 1.0f)); //Up
			// View first-person
			camPos,					//Cam Position
			camPos + cameraFront,	//Look at point
			camUp);					//Up
		
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 proj = glm::perspective(3.14f/4, screenWidth / (float) screenHeight, 0.01f, 30.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

#pragma region [TEXTURE BINDING]

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glUniform1i(glGetUniformLocation(texturedShader, "tex2"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, tex3);
		glUniform1i(glGetUniformLocation(texturedShader, "tex3"), 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, tex4);
		glUniform1i(glGetUniformLocation(texturedShader, "tex4"), 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, tex5);
		glUniform1i(glGetUniformLocation(texturedShader, "tex5"), 5);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, tex6);
		glUniform1i(glGetUniformLocation(texturedShader, "tex6"), 6);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, tex7);
		glUniform1i(glGetUniformLocation(texturedShader, "tex7"), 7);

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, tex8);
		glUniform1i(glGetUniformLocation(texturedShader, "tex8"), 8);

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, tex9);
		glUniform1i(glGetUniformLocation(texturedShader, "tex9"), 9);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, tex10);
		glUniform1i(glGetUniformLocation(texturedShader, "tex10"), 10);

		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, tex11);
		glUniform1i(glGetUniformLocation(texturedShader, "tex11"), 11);

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, tex12);
		glUniform1i(glGetUniformLocation(texturedShader, "tex12"), 12);

#pragma endregion

		glBindVertexArray(vao);
		drawGeometry(texturedShader, cube, blockData, glm::vec3(camPos + (cameraFront * .01f)), camPos);

		SDL_GL_SwapWindow(window); //Double buffering
	}
	
	//Clean Up
	glDeleteProgram(texturedShader);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

void drawGeometry(int shaderProgram, Model cube, BlockData blockData, glm::vec3 lookat, glm::vec3 camDir){
	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");  
	glm::vec3 colVec(1, 1, 1);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");	  
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glm::mat4 model = glm::mat4(1);

	//cout << "lookat vec: " << lookat.x << ", " << lookat.y << ", " << lookat.z << endl;
#pragma region [TEST TEXTURE]
	//**************
	// Angled cube for texturing test
	//**************
	//model = glm::translate(model, glm::vec3(0, 0, 0));

	//// rotate to see texture mapping
	//model = glm::rotate(model, 3.14f / 4, glm::vec3(0.0f, 1.0f, 1.0f));

	//model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); //scale this model

	//// Set which texture to use 
	//glUniform1i(uniTexID, 7);
	//glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//// Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//glDrawArrays(GL_TRIANGLES, cube.startIndex, cube.numVerts); //(Primitive Type, Start Vertex, Num Verticies)

	// Draw each face of grass cube separately

	//// there are 36 vertices per cube, 6 per face
	//int i, vert_idx;
	//int step = cube.numVerts / 6;
	//for (i = 0; i < 6; i++) {
	//	vert_idx = step * i;
	//	if (i  == 0) {
	//		// Set which texture to use 
	//		glUniform1i(uniTexID, 1);
	//		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//		// Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//		glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
	//	}
	//	else if (i == 1){
	//		// Set which texture to use 
	//		glUniform1i(uniTexID, 2);
	//		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//		// Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//		glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
	//	}
	//	else {
	//		// Set which texture to use 
	//		glUniform1i(uniTexID, 3);
	//		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//		// Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//		glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
	//	}
	//}

#pragma endregion

	//**********
	//DRAW FLAT WORLD
	//**********
	int idx;
	float x, y, z, max_depth;
	glm::vec3 pos;
	glm::vec3 cube_scale = glm::vec3(grid_scale, grid_scale, grid_scale);

	int depth, i, j;
	int block;
	for (depth = 0; depth < grid_depth; depth++) {
		max_depth = (grid_scale * (float)(grid_depth) * -1.0f) / 2.0f; // forward/back (depth)
		x = max_depth + (grid_scale * (float)depth);

		for (int i = 0; i < grid_width; i++) {
			for (int j = 0; j < grid_height; j++) {

				idx = (j * grid_width) + i;

				// calculate grid translation coordinates
				y = grid_scale * (float)i - ((float)grid_width / 2 * grid_scale); // right/left
				z = grid_scale * (float)j - ((float)grid_height / 2 * grid_scale); // up/down

				glm::mat4 model = glm::mat4(1);
				pos = glm::vec3(x, y, z);
				model = glm::translate(model, pos);
				model = glm::scale(model, cube_scale); 

				block = blockData.blocks[depth][i][j];

				// Assign texture based on block number
				if (block == 0) continue;
				else if (block == 11) glUniform1i(uniTexID, 12); // cobble
				else if (block == 10) glUniform1i(uniTexID, 11); // planks
				else if (block == 9) glUniform1i(uniTexID, 10); // leaves
				else if (block == 8) { // log
					// there are 36 vertices per cube, 6 per face
					int face, vert_idx;
					int step = cube.numVerts / 6;
					for (face = 0; face < 6; face++) {
						vert_idx = step * face;
						if (face == 5 || face == 3) {
							// Set which texture to use 
							glUniform1i(uniTexID, 8);
							glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

							// Draw an instance of the model (at the position & orientation specified by the model matrix above)
							glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
						}
						else {
							// Set which texture to use 
							glUniform1i(uniTexID, 9);
							glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

							// Draw an instance of the model (at the position & orientation specified by the model matrix above)
							glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
						}
					}
				}
				else if (block == 7) glUniform1i(uniTexID, 7); // bedrock
				else if (block == 6) glUniform1i(uniTexID, 6); // diamond
				else if (block == 5) glUniform1i(uniTexID, 5); // iron
				else if (block == 4) glUniform1i(uniTexID, 4); // coal
				else if (block == 3) {
					// there are 36 vertices per cube, 6 per face
					int face, vert_idx;
					int step = cube.numVerts / 6;

					for (face = 0; face < 6; face++) {
						vert_idx = step * face;
						if (face == 5) {
							// Set which texture to use 
							glUniform1i(uniTexID, 1);
							glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

							// Draw an instance of the model (at the position & orientation specified by the model matrix above)
							glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
						}
						else if (face == 3) {
							// Set which texture to use 
							glUniform1i(uniTexID, 2);
							glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

							// Draw an instance of the model (at the position & orientation specified by the model matrix above)
							glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
						}
						else {
							// Set which texture to use 
							glUniform1i(uniTexID, 3);
							glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

							// Draw an instance of the model (at the position & orientation specified by the model matrix above)
							glDrawArrays(GL_TRIANGLES, cube.startIndex + vert_idx, step); //(Primitive Type, Start Vertex, Num Verticies)
						}
					}

					// GRID TESTING TEXTURE (alternating)
					//if (abs(i - j) == 0 || abs(i - j) % 2 == 0) {
					//	glUniform1i(uniTexID, 1);
					//} else glUniform1i(uniTexID, 2);
					//glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

					//// Draw an instance of the model (at the position & orientation specified by the model matrix above)
					//glDrawArrays(GL_TRIANGLES, cube.startIndex, cube.numVerts); //(Primitive Type, Start Vertex, Num Verticies)
				}
				else if (block == 2) glUniform1i(uniTexID, 1); // dirt
				else if (block == 1) glUniform1i(uniTexID, 0); // stone
				else glUniform1i(uniTexID, 1); // default dirt

				if (block != 3 && block != 8) {
					glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

					//Draw an instance of the model (at the position & orientation specified by the model matrix above)
					glDrawArrays(GL_TRIANGLES, cube.startIndex, cube.numVerts); //(Primitive Type, Start Vertex, Num Verticies)
				}
			}
		}
	}	

	// Draw crosshair
	model = glm::mat4(1);
	model *= glm::inverse(glm::lookAt(lookat, camPos, camUp));
	model = glm::scale(model, glm::vec3(.0001, .0001, .0001));
	glUniform1i(uniTexID, -1);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, cube.startIndex+6, 6);  // single face of the cube model

	// Draw current block
	int curr_block_tex = curr_block;
	if (curr_block_tex == 2 || curr_block_tex == 3) curr_block_tex = 1;
	else if (curr_block_tex == 1) curr_block_tex = 0;
	else if (curr_block_tex == 11) curr_block_tex = 12;
	else if (curr_block_tex == 10) curr_block_tex = 11;
	else if (curr_block_tex == 9)  curr_block_tex = 10;
	model = glm::mat4(1);
	model *= glm::inverse(glm::lookAt(lookat, camPos, camUp));
	model = glm::rotate(model,(float)(M_PI / 4.0f), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(-.013f,-.006f,-.001f));
	model = glm::scale(model, glm::vec3(.004, .004, .004));
	glUniform1i(uniTexID, curr_block_tex);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, cube.startIndex, cube.numVerts);
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}

// Convert position to 3D array index
glm::vec3 PositionToIdx(glm::vec3 pos) {
	int x, y, z;
	float f;
	float posRange, idxRange;
	float posMin, posMax;
	float half_scale = grid_scale / 2.0f;

	// x coord
	x = (grid_depth / 2 - 1) + (int)((pos.x + half_scale) / grid_scale);
	// y coord
	y = (int)((pos.y + half_scale + ((float)grid_width / 2) * grid_scale) / grid_scale);
	// z coord
	z = (int)((pos.z + half_scale + ((float)grid_height / 2) * grid_scale) / grid_scale);

	return glm::vec3(x, y, z);
}

glm::vec3 GetLookIndex(glm::vec3 camPos, glm::vec3 camFront) {
	float h = grid_depth / 2 * grid_scale;
	glm::vec3 plane_origin = glm::vec3(-.15+h, 0, 0);
	glm::vec3 plane_normal = glm::vec3(1, 0, 0);
	float t = glm::dot(plane_origin - camPos, plane_normal) / (glm::dot(camFront, plane_normal));
	glm::vec3 pos = camPos + (t * camFront);
	glm::vec3 lookat_idx = PositionToIdx(pos);
	if (CheckIndexValid(lookat_idx)) lookat_idx = GetLookHelper(camPos, camFront, lookat_idx, plane_origin); // call helper
	return lookat_idx;
}

// recursive helper function
glm::vec3 GetLookHelper(glm::vec3 camPos, glm::vec3 camFront, glm::vec3 lookat_idx, glm::vec3 plane_origin) {
	glm::vec3 plane_normal = glm::vec3(1, 0, 0);
	glm::vec3 pos = glm::vec3(0, 0, 0);
	float t;

	if (blockData.blocks[(int)lookat_idx.x][(int)lookat_idx.y][(int)lookat_idx.z] == 0) {
		t = glm::dot(plane_origin - camPos, plane_normal) / (glm::dot(camFront, plane_normal));
		pos = camPos + (t * camFront);
		lookat_idx = PositionToIdx(pos);
		if (blockData.blocks[(int)lookat_idx.x + 1][(int)lookat_idx.y][(int)lookat_idx.z] == 0) {
			if (blockData.blocks[(int)lookat_idx.x][(int)lookat_idx.y][(int)lookat_idx.z] != 0) return lookat_idx;
			else {
				plane_origin.x -= 0.3f;
				return GetLookHelper(camPos, camFront, lookat_idx, plane_origin);
			}
		}
	}
	return lookat_idx;
}

bool CheckIndexValid(glm::vec3 i) {
	if (i.x < 0 || i.x > grid_depth) return false;
	if (i.y < 0 || i.y > grid_width) return false;
	if (i.z < 0 || i.z > grid_height) return false; 
	return true; 
}
