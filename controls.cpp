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
* Main file for controlling the camera and keyboard
* inputs for the openGL platform with several objects.
* 
* References:
* Tutorial 9 Base Code from https://www.opengl-tutorial.org/
*
*/

// include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow* window;

// include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;
using namespace std;

#include "controls.hpp"

mat4 ViewMatrix;
mat4 ProjectionMatrix;

mat4 getViewMatrix() 
{
	return ViewMatrix;
}

mat4 getProjectionMatrix() 
{
	return ProjectionMatrix;
}

// origin
vec3 origin = vec3(0, 0, 0);
// initial position : on +Z
vec3 position = vec3(0, 0, 5);
// initial horizontal angle : toward -Z
float theta = 3.14f;
// initial vertical angle : toward origin
float phi = 0.0f;
// initial field of view
float initialFoV = 45.0f;

float radius = 15.0f;
float deltaTime = 0.0f;

float speed = 3.0f; // 3 units / second

vec3 front = vec3(-0.5f, -0.5f, -1.0f);
vec3 up = vec3(0.0f, 1.0f, 0.0f);

// variable to begin 3D object movement
bool moving = false;

void computeMatricesFromInputs() 
{
	// set the camera to constantly view the origin
	position = vec3(10.0f, 20.0f, 20.0f);
	front = vec3(-0.5f, -0.5f, -1.0f);

	/* keyboard inputs */

	// close window and escape program
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
	{
		glfwSetWindowShouldClose(window, true);
	}

	// move radially forward
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) 
	{
		radius -= 0.025f;
	}
	// move radially backward
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) 
	{
		radius += 0.025f;
	}

	// rotate left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		phi -= radians(0.1f);
	}

	// rotate right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) 
	{
		phi += radians(0.1f);
	}

	// rotate up
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) 
	{
		// check if theta < 0
		if (theta < 0)
		{
			theta = radians(180.0f);
		}
		else
		{
			theta -= 0.0025f;
		}
	}

	// rotate down
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
	{
		// check if theta > 180
		if (theta > radians(180.0f))
		{
			theta = radians(0.0f);
		}
		else
		{
			theta += 0.0025;
		}
	}

	// start object movement
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		moving = true;
	}

	// recalculate position
	position.x = radius * sin(theta) * cos(phi);
	position.y = radius * sin(theta) * sin(phi);
	position.z = radius * cos(theta);

	float FoV = initialFoV;	// - 5 * glfwGetMouseWheel();

	// projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = perspective(FoV, 4.0f / 3.0f, 0.1f, 100.0f);
	// camera matrix
	ViewMatrix = lookAt
	(
		position,				// camera is here
		origin,					// looks here
		vec3(0,0,1)				// where the head points
	);
} // end compute matrices from inputs method