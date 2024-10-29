/*
*************************************************************************
* 							 FINAL PROJECT
*************************************************************************
*
* Author: Elisa Miller
* Class : ECE 4122
* Date : 12/05/2023
*
* 3D Animated Scene with 
* Custom Classes, Mulithreading, & OpenGL
* 
* Description:
* Main file for loading in various object files to create a 3D animated scene
* with static and non-static objects using custom classes, multithreading, and openGL
* 
* Features:
*	- oversized floor with textured image
*	- moving 3D objects rendered with lighting and material properties
*	- ambient and diffuse lighting effects with intensities that change randomly with time
*	- static 3D objects and a background to give scene depth
*	- camera view points to scenter of scene (controls.cpp)
*	- 'up' and 'down' arrow keys zoom in and out (controls.cpp)
*	- 'left' and 'right' arrow keys rotate camera view left and right (controls.cpp)
*	- 'u' and 'd' keys rotate camera up and down (controls.cpp)
*	- 'esc' key ends application
*	- objects do not move until 'g' key is pressed (controls.cpp)
*	- objects move and rotate randomly about the area
*	- objects collide and bounce off each other and the floor
* 
* References:
* Tutorial 9 Base Code from https://www.opengl-tutorial.org/
*
*/

// include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <thread>
#include <mutex>
#include <ctime>
#include <cstdlib>

// include GLEW
#include <GL/glew.h>

// include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <iostream>

using namespace std;
using namespace glm;

/* 
***********************************************
*		Custom Class Declarations 
***********************************************
*/
/* Object Class - for objects that move and collide with each other*/
class Object
{
public:
	vector<vec3> vertices;
	vector<vec2> uvs;
	vector<vec3> normals;
	vector<unsigned short> indices;
	bool moving;
	vec3 position;
	vec3 velocity;
	vec3 rotation;
	vec3 rotationSpeed;
	float radius;

	// constructor for Object class
	Object(const vector<vec3>& vertices, const vector<vec2>& uvs, const vector<vec3>& normals, 
		const vector<unsigned short>& indices, vec3 position, vec3 velocity, vec3 rotation, vec3 rotationSpeed, float radius) :
		vertices(vertices), uvs(uvs), normals(normals), indices(indices), moving(false), position(position),
		velocity(velocity), rotation(rotation), rotationSpeed(rotationSpeed), radius(radius) {}

}; // end class definition for 3D Object
/* StaticObject Class - for static objects that add to the scene */
class StaticObject
{
public:
	vector<vec3> vertices;
	vector<vec2> uvs;
	vector<vec3> normals;
	vector<unsigned short> indices;
	
	// constructor for Static_Object class
	StaticObject(const vector<vec3>& vertices, const vector<vec2>& uvs, const vector<vec3>& normals, 
		const vector<unsigned short>& indices) :
		vertices(vertices), uvs(uvs), normals(normals), indices(indices) {}

}; // end class definition for 3D Static Object
/* Floor Class */
class Floor
{
public:
	vector<vec3> floorVertices;
	vector<vec2> floorUVs;
	vector<vec3> floorNormals;
	vector<unsigned short> floorIndices;

	// constructor for Floor class
	Floor (float width, float height) :
		floorVertices 
		({
			vec3(-width / 2, -height / 2, 0.0f),
			vec3(width / 2, -height / 2, 0.0f),
			vec3(width / 2, height / 2, 0.0f),
			vec3(-width / 2, height / 2, 0.0f)
		}),
		floorUVs
		({
			vec2(0.0f, 0.0f),
			vec2(1.0f, 0.0f),
			vec2(1.0f, 1.0f),
			vec2(0.0f, 1.0f)
		}),
		floorNormals
		({
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f)
		}),
		floorIndices
		({
			0, 1, 2,
			0, 2, 3
		}) {}

}; // end class definition for the floor
/* Background Class */
class Background
{
public:
	vector<vec3> backgroundVertices;
	vector<vec2> backgroundUVs;
	vector<vec3> backgroundNormals;
	vector<unsigned short> backgroundIndices;

	// constructor for Background class
	Background(float width, float height) :
		backgroundVertices
		({
			vec3(-22.5f, -width / 2, 0.0f),  // bottom-left
			vec3(-22.5f, width / 2, 0.0f),   // bottom-right
			vec3(-22.5f, width / 2, height), // top-right
			vec3(-22.5f, -width / 2, height) // top-left
		}),
		backgroundUVs
		({
			vec2(0.0f, 0.0f),
			vec2(1.0f, 0.0f),
			vec2(1.0f, 1.0f),
			vec2(0.0f, 1.0f)
		}),
		backgroundNormals
		({
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f),
			vec3(0.0f, 0.0f, 2.0f)
		}),
		backgroundIndices
		({
			0, 1, 2,
			0, 2, 3
		}) {}

}; // end class definition for the background

/*
***********************************************
*			Global Variables
***********************************************
*/
// list of moving objects in the scene
vector<Object> objects;
// randon internal light implementation (in fragment shader)
float currentTimePassShader = 0.0f;
// window boundaries
const float minX = -35.5f, maxX = 35.5f;
const float minY = 3.0f, maxY = 20.0f;
const float minZ = -12.0f, maxZ = 20.5f;



/*
***********************************************
*		Multithreading
***********************************************
*/
// mutex used for locking critical sections
mutex objectsMutex;
/* thread method - one thread to calculate movement for all object */
void calculateMovements(float deltaTime, vector<Object>& objects)
{
	// adjust position - use sine function for smooth motion
	float speed = 2.0f + static_cast<float>(rand() % 100) / 5.0f;		// random speed of oscillation
	float amplitude = 1.0f + static_cast<float>(rand() % 50) / 2.5f;	// random amplitude of motion
	float offset = static_cast<float>(rand() % 5);						// random offset for initial position
	for (auto& object : objects)
	{
		if (object.moving)
		{
			/* pumpkin 1 - moves and rotates randomly */
			if (&object == &objects[0])
			{
				// adjust position - use sine function for smooth motion
				speed = 1.0f + static_cast<float>(rand() % 100) / 25.0f;		// random speed of oscillation
				amplitude = 1.0f + static_cast<float>(rand() % 50) / 2.5f;	// random amplitude of motion
				offset = static_cast<float>(rand() % 5);					// random offset for initial position
				lock_guard<mutex> lock(objectsMutex);  // lock the mutex to protect the critical section
				{
					// update "Y" position
					objects[0].position.z = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[0].position.z = clamp(objects[0].position.z, minY, maxY);
					// update "X" position
					objects[0].position.y = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[0].position.y = clamp(objects[0].position.y, minX, maxX);
					// update "Z" position
					objects[0].position.x = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[0].position.x = clamp(objects[0].position.x, minZ, maxZ);
					// generate random rotation speed 
					objects[0].rotationSpeed = vec3
					(
						static_cast<float>(rand() % 360) / 50.0f,
						static_cast<float>(rand() % 360) / 75.0f,
						static_cast<float>(rand() % 360) / 100.0f
					);
					// perform rotation
					objects[0].rotation += objects[0].rotationSpeed * static_cast<float>(deltaTime);
					objects[0].rotation = mod(objects[0].rotation, 360.0f); // keep rotation within 0-360 degrees
				} // end critical section
			}
			/* pumpkin 2 - objects[1] */
			if (&object == &objects[1])
			{
				// adjust position - use sine function for smooth motion
				speed = 1.0f + static_cast<float>(rand() % 100) / 25.0f;		// random speed of oscillation
				amplitude = 1.0f + static_cast<float>(rand() % 50) / 2.5f;	// random amplitude of motion
				offset = static_cast<float>(rand() % 5);					// random offset for initial position
				lock_guard<mutex> lock(objectsMutex);  // lock the mutex to protect the critical section
				{
					// update "Y" position
					objects[1].position.z = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[1].position.z = clamp(objects[1].position.z, minY, maxY);
					// update "X" position
					objects[1].position.y = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[1].position.y = clamp(objects[1].position.y, minX, maxX);
					// update "Z" position
					objects[1].position.x = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[1].position.x = clamp(objects[1].position.x, minZ, maxZ);
					// generate random rotation speed 
					objects[1].rotationSpeed = vec3
					(
						static_cast<float>(rand() % 360) / 75.0f,
						static_cast<float>(rand() % 360) / 100.0f,
						static_cast<float>(rand() % 360) / 125.0f
					);
					// perform rotation
					objects[1].rotation += objects[1].rotationSpeed * static_cast<float>(deltaTime);
					objects[1].rotation = mod(objects[1].rotation, 360.0f); // keep rotation within 0-360 degrees
				} // end critical section
			} // end if
			/* pumpkin 3 - objects[2] */
			if (&object == &objects[2])
			{
				// adjust position - use sine function for smooth motion
				speed = 1.0f + static_cast<float>(rand() % 100) / 25.0f;	// random speed of oscillation
				amplitude = 1.0f + static_cast<float>(rand() % 50) / 2.5f;	// random amplitude of motion
				offset = static_cast<float>(rand() % 5);					// random offset for initial position
				lock_guard<mutex> lock(objectsMutex);  // lock the mutex to protect the critical section
				{
					// update "Y" position
					objects[2].position.z = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[2].position.z = clamp(objects[2].position.z, minY, maxY);
					// update "X" position
					objects[2].position.y = offset + sin(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[2].position.y = clamp(objects[2].position.y, minX, maxX);
					// update "Z" position
					objects[2].position.x = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[2].position.x = clamp(objects[2].position.x, minZ, maxZ);
					// generate random rotation speed 
					objects[2].rotationSpeed = vec3
					(
						static_cast<float>(rand() % 360) / 50.0f,
						static_cast<float>(rand() % 360) / 75.0f,
						static_cast<float>(rand() % 360) / 100.0f
					);
					// perform rotation
					objects[2].rotation += objects[2].rotationSpeed * static_cast<float>(deltaTime);
					objects[2].rotation = mod(objects[2].rotation, 360.0f); // keep rotation within 0-360 degrees
				} // end critical section
			} // end if
			/* ghost - objects[3] */
			if (&object == &objects[3])
			{
				// adjust position - use sine function for smooth motion
				speed = 1.0f + static_cast<float>(rand() % 100) / 50.0f;	// random speed of oscillation
				amplitude = 1.0f + static_cast<float>(rand() % 75) / 10.f;	// random amplitude of motion
				offset = static_cast<float>(rand() % 5);					// random offset for initial position
				lock_guard<mutex> lock(objectsMutex);  // lock the mutex to protect the critical section
				{
					// update "Y" position
					objects[3].position.z = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[3].position.z = clamp(objects[3].position.z, minY, maxY + 15.0f);
					// update "X" position
					objects[3].position.y = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[3].position.y = clamp(objects[3].position.y, minX - 5.0f, maxX + 5.0f);
					// update "Z" position
					objects[3].position.x = offset + cos(glfwGetTime() * speed) * amplitude;
					// clamp the position within boundaries
					objects[3].position.x = clamp(objects[3].position.x, minZ - 5.0f, maxZ + 5.0f);
					// generate random rotation speed 
					objects[3].rotationSpeed = vec3
					(
						static_cast<float>(rand() % 360) / 15.0f,
						static_cast<float>(rand() % 360) / 30.0f,
						static_cast<float>(rand() % 360) / 45.0f
					);
					// perform rotation
					objects[3].rotation += objects[3].rotationSpeed * static_cast<float>(deltaTime);
					objects[3].rotation = mod(objects[3].rotation, 360.0f); // keep rotation within 0-360 degrees
				} // end critical section
			} // end if
		} // end if
		// handle collisions
		for (auto& other : objects)
		{
			if (&object != &other)
			{
				lock_guard<mutex> lock(objectsMutex);  // lock the mutex to protect the critical section
				{
					vec3 diff = other.position - object.position;
					float distance = length(diff);
					if (distance < object.radius + other.radius + 0.2f)
					{
						vec3 normal = normalize(diff);
						// reflect velocities based on collision normal
						object.velocity = reflect(object.velocity, normal);
						other.velocity = reflect(other.velocity, -normal);
						// move objects slightly apart to avoid sticking
						float pushApart = (object.radius + other.radius + 5.0f - distance) / 2.0f;
						object.position -= normal * pushApart;
						other.position += normal * pushApart;
					} // end if
				} // end critical section
			} // end if
		} // end inner for loop
	} // end outer for loop
} // end calculateMovements method

/*
*************************************************
*				Main Method
*************************************************
*/
int main(void)
{
	// initialize GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	// open a window and create its OpenGL context
	window = glfwCreateWindow(1640, 1240, "Final Project - 3D Animation, Multithreading, & OpenGL", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 2040 / 2, 1640 / 2);

	// dark gray background
	glClearColor(0.1f, 0.1f, 0.1f, 0.5f);

	// enable depth test
	glEnable(GL_DEPTH_TEST);
	// accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");

	// get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// get a handle for our buffers
	GLuint vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	GLuint vertexUVID = glGetAttribLocation(programID, "vertexUV");
	GLuint vertexNormal_modelspaceID = glGetAttribLocation(programID, "vertexNormal_modelspace");

	// get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	// get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// load the texture for the pumpking objects
	GLuint Texture = loadDDS("uvmap.DDS");

	// load the texture for the ghost object
	GLuint GhostTexture = loadDDS("specular.DDS");

	// load texture for tree objects
	GLuint TreeTexture = loadDDS("diffuse.DDS");

	// load floor texture
	GLuint FloorTexture = loadDDS("diffuse.DDS");

	// load background texture
	GLuint BackgroundTexture = loadDDS("uvmap.DDS");

	// create instance of Floor object
	Floor floor(45.0f, 60.0f);

	// create instance of Background object
	Background background(60.0f, 20.0f);

	// defining the translation distance
	float translateDist = 1.95f;

	// speed calculations
	double previousTime = glfwGetTime();
	int numFrames = 0;
	float deltaTime = 0.1f;

	// specular & diffuse values
	float diffuseDefault = 1.0f;
	float specularDefault = 1.0f;

	// create pumpkin object and index the VBO
	vector<vec3> vertices;
	vector<vec2> uvs;
	vector<vec3> normals;
	bool res = loadOBJ("pumpkin.obj", vertices, uvs, normals);
	vector<unsigned short> indices;
	vector<vec3> indexed_vertices;
	vector<vec2> indexed_uvs;
	vector<vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);
	Object pumpkin(indexed_vertices, indexed_uvs, indexed_normals, indices,
		vec3(15.0f, 0.0f, 0.0f), vec3(0.1f, 0.0f, 0.0f), vec3(0.0f), vec3(0.1f, 0.1f, 0.1f), 1.0f);
	// add 3 pumpkins to list of moving objects
	objects.emplace_back(pumpkin); // middle pumpkin
	pumpkin.position = vec3(15.0f, 8.0f, 4.0f);
	objects.emplace_back(pumpkin); // right pumpkin
	pumpkin.position = vec3(15.0f, -8.0f, 4.0f);
	objects.emplace_back(pumpkin); // left pumpkin
	// create and bind buffers for pumpkin object
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, pumpkin.vertices.size() * sizeof(vec3), &pumpkin.vertices[0], GL_STATIC_DRAW);
	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, pumpkin.uvs.size() * sizeof(vec2), &pumpkin.uvs[0], GL_STATIC_DRAW);
	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, pumpkin.normals.size() * sizeof(vec3), &pumpkin.normals[0], GL_STATIC_DRAW);
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pumpkin.indices.size() * sizeof(unsigned short), &pumpkin.indices[0], GL_STATIC_DRAW);

	// create ghost object and index the VBO
	vector<vec3> ghostVertices;
	vector<vec2> ghostUVs;
	vector<vec3> ghostNormals;
	res = loadOBJ("Halloween_Ghost.obj", ghostVertices, ghostUVs, ghostNormals);
	vector<unsigned short> ghostIndices;
	vector<vec3> indexed_ghost_vertices;
	vector<vec2> indexed_ghost_uvs;
	vector<vec3> indexed_ghost_normals;
	indexVBO(ghostVertices, ghostUVs, ghostNormals, ghostIndices, indexed_ghost_vertices, indexed_ghost_uvs, indexed_ghost_normals);
	Object ghost(indexed_ghost_vertices, indexed_ghost_uvs, indexed_ghost_normals, ghostIndices, 
		vec3(0.0f, 0.0f, 2.0f), vec3(0.1f, 0.0f, 0.0f), vec3(0.0f), vec3(0.1f, 0.1f, 0.1f), 1.0f);
	// add ghost to list of moving objects
	objects.emplace_back(ghost);
	// create and bind buffers for ghost object
	GLuint ghostVertexBuffer;
	glGenBuffers(1, &ghostVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ghostVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, ghost.vertices.size() * sizeof(vec3), &ghost.vertices[0], GL_STATIC_DRAW);
	GLuint ghostUVBuffer;
	glGenBuffers(1, &ghostUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ghostUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, ghost.uvs.size() * sizeof(vec2), &ghost.uvs[0], GL_STATIC_DRAW);
	GLuint ghostNormalBuffer;
	glGenBuffers(1, &ghostNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ghostNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, ghost.normals.size() * sizeof(vec3), &ghost.normals[0], GL_STATIC_DRAW);
	GLuint ghostElementBuffer;
	glGenBuffers(1, &ghostElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghostElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ghost.indices.size() * sizeof(unsigned short), &ghost.indices[0], GL_STATIC_DRAW);

	// create and bind buffers for the floor
	GLuint floorVertexBuffer;
	glGenBuffers(1, &floorVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, floorVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, floor.floorVertices.size() * sizeof(vec3), &floor.floorVertices[0], GL_STATIC_DRAW);
	GLuint floorUVBuffer;
	glGenBuffers(1, &floorUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, floorUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, floor.floorUVs.size() * sizeof(vec2), &floor.floorUVs[0], GL_STATIC_DRAW);
	GLuint floorNormalBuffer;
	glGenBuffers(1, &floorNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, floorNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, floor.floorNormals.size() * sizeof(vec3), &floor.floorNormals[0], GL_STATIC_DRAW);
	GLuint floorElementBuffer;
	glGenBuffers(1, &floorElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, floor.floorIndices.size() * sizeof(unsigned short), &floor.floorIndices[0], GL_STATIC_DRAW);

	// create and bind buffers for the background
	GLuint backgroundVertexBuffer;
	glGenBuffers(1, &backgroundVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, background.backgroundVertices.size() * sizeof(vec3), &background.backgroundVertices[0], GL_STATIC_DRAW);
	GLuint backgroundUVBuffer;
	glGenBuffers(1, &backgroundUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, background.backgroundUVs.size() * sizeof(vec2), &background.backgroundUVs[0], GL_STATIC_DRAW);
	GLuint backgroundNormalBuffer;
	glGenBuffers(1, &backgroundNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, background.backgroundNormals.size() * sizeof(vec3), &background.backgroundNormals[0], GL_STATIC_DRAW);
	GLuint backgroundElementBuffer;
	glGenBuffers(1, &backgroundElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, background.backgroundIndices.size() * sizeof(unsigned short), &background.backgroundIndices[0], GL_STATIC_DRAW);

	/*
	*******************************************************************************
	*				EXTRA CREDIT - Static Tree Objects
	*******************************************************************************
	*/
	vector<vec3> treeVertices;
	vector<vec2> treeUVs;
	vector<vec3> treeNormals;
	res = loadOBJ("tree.obj", treeVertices, treeUVs, treeNormals);
	vector<unsigned short> treeIndices;
	vector<vec3> indexed_tree_vertices;
	vector<vec2> indexed_tree_uvs;
	vector<vec3> indexed_tree_normals;
	indexVBO(treeVertices, treeUVs, treeNormals, treeIndices, indexed_tree_vertices, indexed_tree_uvs, indexed_tree_normals);
	// create instance of Static_Object for background trees
	StaticObject tree(indexed_tree_vertices, indexed_tree_uvs, indexed_tree_normals, treeIndices);
	// create and bind buffers for tree object
	GLuint treeVertexBuffer;
	glGenBuffers(1, &treeVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, tree.vertices.size() * sizeof(vec3), &tree.vertices[0], GL_STATIC_DRAW);
	GLuint treeUVBuffer;
	glGenBuffers(1, &treeUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, tree.uvs.size() * sizeof(vec2), &tree.uvs[0], GL_STATIC_DRAW);
	GLuint treeNormalBuffer;
	glGenBuffers(1, &treeNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, tree.normals.size() * sizeof(vec3), &tree.normals[0], GL_STATIC_DRAW);
	GLuint treeElementBuffer;
	glGenBuffers(1, &treeElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tree.indices.size() * sizeof(unsigned short), &tree.indices[0], GL_STATIC_DRAW);

	/* rendering loop */
	do
	{
		// get the current time to pass into fragment shader
		currentTimePassShader = glfwGetTime();
		// measure speed
		float currentTime = glfwGetTime();
		// get a handle for our "time" uniform
		GLuint timeID = glGetUniformLocation(programID, "time");
		// set 'time' unfirom in shader
		glUniform1f(timeID, currentTimePassShader);
		numFrames++;
		if (currentTime - previousTime >= 1.0)  // if last prinf() was more than 1sec ago
		{
			// printf and reset
			printf("%f ms/frame\n", 1000.0 / double(numFrames));
			numFrames = 0;
			previousTime += 1.0;
		}
		if (moving == true) // external boolean defined in controls.hpp
		{
			objects[0].moving = true;
			objects[1].moving = true;
			objects[2].moving = true;
			objects[3].moving = true;
		}

		
		// seed the random number generator
		srand(time(0));

		/* update position & rotation of each object */
		if (moving == true)
		{
			// create thread to calculate object movements
			thread movementThread(&calculateMovements, deltaTime, ref(objects));
			// wait for movement thread to finish before exiting
			if (movementThread.joinable())
			{
				movementThread.join();
			}
		}

		// clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// compute the MVP matrix from keyboard input
		computeMatricesFromInputs();
		mat4 ProjectionMatrix = getProjectionMatrix();
		mat4 ViewMatrix = getViewMatrix();
		mat4 ModelMatrix = mat4(1.0);

		// use our shader
		glUseProgram(programID);
		GLuint diffuseLocation = glGetUniformLocation(programID, "diffuseIntensity");
		GLuint specularLocation = glGetUniformLocation(programID, "specularIntensity");
		vec3 lightPos = vec3(5, 5, 5);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		/*
		**************************************************
		*			Render the Full Scene
		**************************************************
		*/
		/* render the ghost and pumpkin objects! */
		/* ghost! */
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, objects[3].position);
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[3].rotation.y), vec3(0.0f, 1.0f, 0.0f));
		mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GhostTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, ghostVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, ghostUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, ghostNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghostElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			ghostIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end ghost rendering
		/* pumpkin 1 - middle */
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, objects[0].position);
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[0].rotation.x), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[0].rotation.y), vec3(0.0f, 1.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[0].rotation.z), vec3(0.0f, 0.0f, 1.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);
		// end rendering of 1st pumpkin
		/* pumpkin 2 - right */
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, objects[1].position);
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.65f, 0.9f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[1].rotation.x), vec3(-1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[1].rotation.y), vec3(0.0f, -1.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[1].rotation.z), vec3(0.0f, 0.0f, -1.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);
		// end rendering of 2nd pumpkin
		/* pumpkin 3 - left */
		ModelMatrix = mat4(1.0);
		//ModelMatrix = translate(ModelMatrix, vec3(15.0f, -3.75f, 1.25f));
		ModelMatrix = translate(ModelMatrix, objects[2].position);
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.65f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[2].rotation.x), vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[2].rotation.y), vec3(0.0f, 1.0f, 0.0f));
		ModelMatrix = rotate(ModelMatrix, radians(objects[2].rotation.z), vec3(0.0f, 0.0f, 1.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);
		// end rendering of 3rd pumpkin
		/* end 3D moving object rendering */

		/* render the floor */
		mat4 floorModel = mat4(1.0f);
		mat4 floorMVP = ProjectionMatrix * ViewMatrix * floorModel;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &floorMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &floorModel[0][0]);
		// bind the floor texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FloorTexture);
		glUniform1i(TextureID, 1);
		// bind buffers for floor
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, floorVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                           // size
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, floorUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                  // The attribute we want to configure
			2,                           // size : U+V => 2
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, floorNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,   // The attribute we want to configure
			3,                           // size
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		// bind the element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorElementBuffer);
		// draw the floor
		glDrawElements
		(
			GL_TRIANGLES,				// mode
			floor.floorIndices.size(),	// count
			GL_UNSIGNED_SHORT,			// type
			(void*)0					// element array buffer offset
		);
		// end floor rendering

		/* render the background */
		mat4 backgroundModel = mat4(1.0f);
		mat4 backgroundMVP = ProjectionMatrix * ViewMatrix * backgroundModel;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &backgroundMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &backgroundModel[0][0]);
		// bind the background texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, BackgroundTexture);
		glUniform1i(TextureID, 1);
		// bind buffers for background
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, backgroundVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                           // size
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, backgroundUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                  // The attribute we want to configure
			2,                           // size : U+V => 2
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, backgroundNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,   // The attribute we want to configure
			3,                           // size
			GL_FLOAT,                    // type
			GL_FALSE,                    // normalized?
			0,                           // stride
			(void*)0                     // array buffer offset
		);
		// bind the element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundElementBuffer);
		// draw the background
		glDrawElements
		(
			GL_TRIANGLES,							// mode
			background.backgroundIndices.size(),	// count
			GL_UNSIGNED_SHORT,						// type
			(void*)0								// element array buffer offset
		);
		// end background rendering

		/* render the trees - EXTRA CREDIT */
		// 1st tree
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, vec3(-18.75f, 17.0f, 1.15f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TreeTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			treeIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end rendering of 1st tree
		// 2nd tree!
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, vec3(-18.75f, -17.0f, 1.15f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TreeTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			treeIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end rendering of 2nd tree
		// 3rd tree!
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, vec3(-18.75f, 0.0f, 1.15f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TreeTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			treeIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end rendering of 3rd tree
		// 4th tree!
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, vec3(-18.75f, -9.25f, 1.15f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TreeTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			treeIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end rendering of 4th tree
		// 5th tree!
		ModelMatrix = mat4(1.0);
		ModelMatrix = translate(ModelMatrix, vec3(-18.75f, 9.25f, 1.15f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = rotate(ModelMatrix, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
		MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix;
		// send our transformation to the currently bound shader
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		// bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TreeTexture);
		// set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);
		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeVertexBuffer);
		glVertexAttribPointer
		(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(vertexUVID);
		glBindBuffer(GL_ARRAY_BUFFER, treeUVBuffer);
		glVertexAttribPointer
		(
			vertexUVID,                       // The attribute we want to configure
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(vertexNormal_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, treeNormalBuffer);
		glVertexAttribPointer
		(
			vertexNormal_modelspaceID,        // The attribute we want to configure
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		// index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, treeElementBuffer);
		// draw the triangles !
		glDrawElements
		(
			GL_TRIANGLES,		// mode
			treeIndices.size(),// count
			GL_UNSIGNED_SHORT,	// type
			(void*)0			// element array buffer offset
		);
		// end rendering of 5th tree
		/* end tree rendering */

		// disable vertex attribute arrays 
		glDisableVertexAttribArray(vertexPosition_modelspaceID);
		glDisableVertexAttribArray(vertexUVID);
		glDisableVertexAttribArray(vertexNormal_modelspaceID);

		/* end scene rendering */

		// swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	// check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

	/* cleanup VBO and shader */
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteBuffers(1, &floorVertexBuffer);
	glDeleteBuffers(1, &floorUVBuffer);
	glDeleteBuffers(1, &floorNormalBuffer);
	glDeleteBuffers(1, &floorElementBuffer);
	glDeleteBuffers(1, &backgroundVertexBuffer);
	glDeleteBuffers(1, &backgroundUVBuffer);
	glDeleteBuffers(1, &backgroundNormalBuffer);
	glDeleteBuffers(1, &backgroundElementBuffer);
	glDeleteBuffers(1, &ghostVertexBuffer);
	glDeleteBuffers(1, &ghostUVBuffer);
	glDeleteBuffers(1, &ghostNormalBuffer);
	glDeleteBuffers(1, &ghostElementBuffer);
	glDeleteBuffers(1, &treeVertexBuffer);
	glDeleteBuffers(1, &treeUVBuffer);
	glDeleteBuffers(1, &treeNormalBuffer);
	glDeleteBuffers(1, &treeElementBuffer);
	glDeleteTextures(1, &Texture);
	glDeleteTextures(1, &BackgroundTexture);
	glDeleteTextures(1, &FloorTexture);
	glDeleteTextures(1, &GhostTexture);
	glDeleteProgram(programID);

	// close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
} // end main method
