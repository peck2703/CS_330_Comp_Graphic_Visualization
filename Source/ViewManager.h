///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#pragma once
#include <GL/glew.h>        
#include <GLFW/glfw3.h>     

#include "ShaderManager.h"
#include "camera.h"


class ViewManager
{
public:
	// constructor
	ViewManager(
		ShaderManager* pShaderManager);
	// destructor
	~ViewManager();

	// mouse position callback for mouse interaction with the 3D scene
	static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);

	//Adjust the speed with the mouse scroll wheel
	static void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset);

	glm::mat4 GetProjectionMatrix();

private:

	//Create FOV range
	float m_FOV = 45.0f;

	//Create bool for perspective
	bool m_isPerspective = true;	//Default state is perspective, not orthographic

	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// active OpenGL display window
	GLFWwindow* m_pWindow;

	// process keyboard events for interaction with the 3D scene
	void ProcessKeyboardEvents();

public:
	// create the initial OpenGL display window
	GLFWwindow* CreateDisplayWindow(const char* windowTitle);

	// prepare the conversion from 3D object display to 2D scene display
	void PrepareSceneView();

	//Return the m_isPerspective value
	bool IsPerspective();

	//return the m_FOV value
	float GetFOV();
	;
};