///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include "camera.h"

extern Camera* g_pCamera;

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	m_FOV = 45.f;
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	m_isPerspective = true;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	//Adding line per announcement
	glfwSetWindowUserPointer(window, this);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	//Adding callback for the mouse scroll to add or decrease speed
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	//After first mouse event is received, Data needs to be stored so all further moves offset properly
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	//Calculate x and y offset for moving the camera
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos;	//Has to be reversed to match x-y coordinate system

	//Set the current positions as the last positions recorded
	gLastX = xMousePos;
	gLastY = yMousePos;

	//Move the camera according to the offsets calculated
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse Scroll Callback()
 *
 *  This method is called to process when the mouse scroll wheel moves
 *  that may be waiting in the event queue.
 ***********************************************************/

void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	//Get the instance object for the window
	ViewManager* instance = static_cast<ViewManager*>(glfwGetWindowUserPointer(window));

	//Ensure the instance isn't a null value
	if (instance != nullptr)
	{
		//Increase or decrease the speed of the speed variable
		g_pCamera->MovementSpeed += (float)yOffset * 0.5;

		instance->m_FOV -= (float)yOffset;

		//Clamp value
		if (instance->m_FOV < 1.0f) {
			instance->m_FOV = 1.0f;
		}
		if (instance->m_FOV > 45.0f) {
			instance->m_FOV = 45.0f;
		}
	}

}

glm::mat4 ViewManager::GetProjectionMatrix()
{
	float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	if (m_isPerspective)
	{
		return glm::perspective(glm::radians(m_FOV), aspect, 0.1f, 100.0f);
	}
	else {
		return glm::ortho(-10.0f * aspect, 10.f * aspect, -10.f, 10.f, 0.1f, 100.f);
	}
	return glm::mat4();
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// if the camera object is null, then exit this method
	if (NULL == g_pCamera)
	{
		return;
	}

	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	/********************************************
	 *											*
	 *      Up versus Down Camera Movement		*
	 *											*
	 * ******************************************/

	 //Begin adding controls to move camera up and down using keys then mouse controls
	 //process the camera moving up and down using Q and E keys
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	/********************************************
	 *											*
	 *      perspective versus Orthographic		*
	 *											*
	 * ******************************************/
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		m_isPerspective = true;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		m_isPerspective = false;
	}


	/********************************************
	 *											*
	 *      Tab System to Unfocus Screen		*
	 *											*
	 * ******************************************/

	 //Create a bool to determine if the TAB key was pressed to escape the system if needed
	static bool tabPressedLastFrame = false;

	//Bool to check if the tab key itself was pressed
	bool tabIsPressed = (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_PRESS);

	//If both above bools are true, then execute; resets the cursor to a normal state
	if (tabIsPressed && !tabPressedLastFrame)
	{
		// Toggle logic here
		if (glfwGetInputMode(m_pWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	//reset the bool to a default false state
	tabPressedLastFrame = tabIsPressed;


}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}

bool ViewManager::IsPerspective()
{
	return m_isPerspective;
}

float ViewManager::GetFOV()
{
	return g_pCamera->Zoom; ;
}
