///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "SceneManager.h"
#include "ViewManager.h"
#include "Camera.h"
#include <filesystem>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif


// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

extern Camera* g_pCamera;

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{

		// If the tag is empty, we "turn off" the texture
		if (textureTag.empty())
		{
			m_pShaderManager->setIntValue(g_UseTextureName, false);
			return;
		}

		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 10.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.06f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.f, 0.66f, 0.34f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -30.0f, 20.0f, -20.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.06f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.f, 0.66f, 0.34f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 1.0f, 10.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.06f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.f, 0.66f, 0.34f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[3].position", 10.0f, 0.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.06f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.f, 0.66f, 0.34f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	namespace fs = std::filesystem;

	bool bReturn = false;

	//This was added as a test to find the correct path to the folder

	std::string pathString = fs::current_path().string();
	std::cout << "DEBUG: Current path is: " << pathString << std::endl;

	fs::path texturePath = fs::current_path() / "Utilities\\textures\\marble.jpg";
	if (fs::exists(texturePath)) {
		std::cout << "SUCCESS: Found texture file!" << std::endl;
	}
	else {
		std::cout << "ERROR: File not found at " << texturePath.string() << std::endl;
	}

	//Floor texture
	bReturn = CreateGLTexture("Utilities\\textures\\wood_base.jpg", "floor");

	//Candle Base texture
	bReturn = CreateGLTexture("Utilities\\textures\\marble.jpg", "candle_base");

	//Metal Candle Lamp
	bReturn = CreateGLTexture("Utilities\\textures\\metal_polished.jpg", "lamp");

	//Candle material itself - Snow will imitate a wax-like substance
	bReturn = CreateGLTexture("Utilities\\textures\\snow.jpg", "wax");

	//Cardboard material itself
	bReturn = CreateGLTexture("Utilities\\textures\\cardboard.jpg", "cardboard");

	//Paper towel itself
	bReturn = CreateGLTexture("Utilities\\textures\\paper.jpg", "paper");

	//Gumdrop
	bReturn = CreateGLTexture("Utilities\\textures\\gumdrop.jpg", "gumdrop");

	//Diet Coke
	bReturn = CreateGLTexture("Utilities\\textures\\dietCoke.jpg", "soda");

	//Plastic Cap
	bReturn = CreateGLTexture("Utilities\\textures\\ribbed.jpg", "plastic");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	//Load object materials
	DefineObjectMaterials();

	//Prepare the lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	//Load all possible meshes needed for this assignment
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	//Do a check on null values
	if (m_pWindow == nullptr || m_pViewManager == nullptr) return;

	//Declaring variables needed to adjust the perspective versus ortho views
	int width, height;
	glfwGetFramebufferSize(m_pWindow, &width, &height);

	glViewport(0, 0, width, height);

	//Calculate the aspect Ratio
	float aspect = (float)width / (float)height;

	//Determine the projection matrix
	glm::mat4 projection;
	if (m_pViewManager->IsPerspective()) {
		projection = glm::perspective(glm::radians((float)m_pViewManager->GetFOV()), aspect, 0.1f, 100.0f);
	}
	else {
		float orthoSize = 10.0f;
		projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 100.0f);
	}

	// Pass to shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value("projection", projection);

		m_pShaderManager->setMat4Value("view", g_pCamera->GetViewMatrix());
	}

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9098, 0.8313, 0.7451, 1);

	//Set the shader to the floor texture from above
	SetShaderTexture("floor");

	//Set shader material
	SetShaderMaterial("wood");

	//Set the uv scale to match the floor
	SetTextureUVScale(4.0, 2.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Reset the UV scaling for next object
	SetTextureUVScale(1.0, 1.0);

	/****************************************************************/

	/****************************************************************
	*						Candle Hook (Cylinder)					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5, 8.0, 0.5);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(6.0, 2.0, -4.0);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.0, 1.0, 1.0, 1.0);

	//Set the shader to the floor texture from above
	SetShaderTexture("candle_base");

	//Set shader material
	SetShaderMaterial("cement");

	//Set the uv scale to match the floor
	SetTextureUVScale(1.0, 0.50);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Reset the UV scaling for next object
	SetTextureUVScale(1.0, 1.0);

	/****************************************************************
	*						Candle Hook (Half-Torus)				*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(2.5, 2.5, 2.50);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(8.5f, 10.f, -4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.0, 1.0, 1.0, 1.0);

	//Set the shader to the floor texture from above
	SetShaderTexture("candle_base");

	//Set shader material
	SetShaderMaterial("cement");

	//Set the uv scale to match the floor
	SetTextureUVScale(1.0, 0.50);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	//Reset the UV scaling for next object
	SetTextureUVScale(1.0, 1.0);

	/****************************************************************
	*						Candle lamp (Tapered Cylinder)			*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(3.0, 3.0, 3.0);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(11.f, 7.5f, -4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.0, 1.0, 1.0, 1.0);

	//Set the shader to the floor texture from above
	SetShaderTexture("lamp");

	//Set shader material
	SetShaderMaterial("glass");

	//Set the uv scale to match the floor
	SetTextureUVScale(1.0, 0.50);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(true, false);

	//Reset the UV scaling for next object
	SetTextureUVScale(1.0, 1.0);

	/****************************************************************
	*						Candle Cover Cylinder					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(2.5, 3.0, 2.5);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(11.f, 2.25f, -4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//DISABLE the first texture, ENABLE the second, so we can achieve the transparent glass texture
	GLuint proID = m_pShaderManager->m_programID;
	glUniform1i(glGetUniformLocation(proID, "bUseTexture"), false); // Tell shader: Use objectColor instead
	glUniform1i(glGetUniformLocation(proID, "bUseSecondaryTexture"), true); // Tell shader: Overlay the texture

	glUniform1f(glGetUniformLocation(proID, "blendAmount"), 0.75f);

	//Doing a check to see if useSecondaryTexture works
	GLint loc = glGetUniformLocation(proID, "bUseSecondaryTexture");
	if (loc == -1) {
		// This will print to the console if the name in GLSL doesn't match exactly
		std::cout << "DEBUG ERROR: 'bUseSecondaryTexture' not found in shader!" << std::endl;
	}
	else {
		// If it finds it, force it to true
		glUniform1i(loc, true);
	}

	int overlayID = FindTextureID("wax");
	if (overlayID != -1)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, overlayID);

		GLint secondarySamplerLocation = glGetUniformLocation(proID, "secondaryTexture");
		glUniform1i(secondarySamplerLocation, 1);
	}

	//Set blend amount higher
	glUniform1f(glGetUniformLocation(proID, "blendAmount"), 0.45f);

	//Set the shader color to a beige color
	//Change the alpha level so it appears glass-like
	SetShaderColor(0.00784, 0.18823, 0.12549, 0.5f);

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh(false, true);

	//Reset (Crucial so other objects don't overlap) ---
	glUniform1i(glGetUniformLocation(proID, "bUseSecondaryTexture"), false);
	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Candle Base (Box)						*
	* ***************************************************************/


	//For some reason, I cannot get the box to render

	//Set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(10.0, 1.0, 10.0);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This box will be the base
	positionXYZ = glm::vec3(9.f, 2.f, -4.f);

	//Set the transformation into memory to use be used on the drawn box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.0, 1.0, 1.0, 1.0);

	//Set the shader to the floor texture from above
	SetShaderTexture("candle_base");

	//Set shader material
	SetShaderMaterial("tile");

	//Set the uv scale to match the floor
	SetTextureUVScale(1.0, 1.0);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawBoxMesh();

	//Reset the UV scaling for next object
	SetTextureUVScale(1.0, 1.0);

	/****************************************************************
	*						Candle Cover Cylinder					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(2.f, 2.5, 2.f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(11.f, 2.25f, -4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	//Change the alpha level so it appears glass-like
	SetShaderColor(0.00784, 0.18823, 0.12549, 0.5f);

	//Set shader material
	SetShaderMaterial("wax");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Paper Towel Cylinder					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(1.5f, 4.5f, 1.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-7.0f, 0.25f, 0.0f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.00784, 0.18823, 0.12549, 1.f);

	//Set shader material
	SetShaderMaterial("paper");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Paper Towel Insert						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 4.51f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-7.0f, 0.25f, 0.0f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	//Change the alpha level so it appears glass-like
	SetShaderColor(0.f, 0.f, 0.f, 1.f);

	//Set shader material
	SetShaderMaterial("paper");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Box Rectangle							*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(20.f, 3.5f, 12.f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(5.0f, 0.f, -4.0f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("cardboard");

	//Set shader material
	SetShaderMaterial("clay");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawBoxMesh();

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Headphone Base							*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(0.f, 2.45f, -4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Headphone Left							*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 1.75f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-2.5f, 2.45f, -2.25f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						Headphone Left Ear						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.6f, 0.75f, 0.6f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-1.5f, 2.45f, -2.875f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						Headphone Left Ear Cup					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.6f, 0.6f, 1.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-1.375f, 2.45f, -2.875f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						Headphone Right							*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 1.95f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(2.5f, 2.25f, -2.25f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************
	*						Headphone Right Ear						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.6f, 0.95f, 0.6f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -90.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(1.5f, 2.45f, -2.875f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						Headphone Right Ear Cup					*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.6f, 0.6f, 1.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(1.5f, 2.45f, -2.875f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a gray color
	SetShaderColor(.2f, .2f, .2f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("gold");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						Gumdrop									*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 0.675f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(0.f, 1.75f, 0.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("gumdrop");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.0f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(true, true);

	SetTextureUVScale(0.5f, 0.5f);

	/****************************************************************
	*						2-Liter Soda Bottom						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(1.f, 2.f, 1.f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 0.f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.1490f, 0.08627f, 0.08235f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(1.f, 1.f);

	/****************************************************************
	*						2-Liter Soda Label						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(1.f, 2.f, 1.f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 2.f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("soda");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(2.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(1.f, 1.f);

	/****************************************************************
	*						2-Liter Soda Bottom						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(1.f, 2.f, 1.f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 4.f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.1490f, 0.08627f, 0.08235f, 1.f);

	//Set the shader to the floor texture from above
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	SetTextureUVScale(1.f, 1.f);

	/****************************************************************
	*						2-Liter Soda Upper						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 0.25f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 6.f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(0.1490f, 0.08627f, 0.08235f, 1.f);

	//Set the shader to nothing
	//SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(1.f, 1.f);

	/****************************************************************
	*						2-Liter Soda Lip						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.25f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 6.25f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.f, 1.f, 1.f, 1.f);

	//Set the shader to nothing
	SetShaderTexture("");

	//Set shader material
	SetShaderMaterial("glass");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(1.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();

	SetTextureUVScale(1.f, 1.f);

	/****************************************************************
	*						2-Liter Soda Cap						*
	* ***************************************************************/

	//Set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	//Set the XYZ rotation for the cylinder mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	//Set the XYZ position for the mesh. This cylinder will be hanging from the ceiling or from a half torus hook
	positionXYZ = glm::vec3(-4.f, 6.25f, 4.f);

	//Set the transformation into memory to use be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the shader color to a beige color
	SetShaderColor(1.f, 1.f, 1.f, 0.5f);

	//Set the shader to the floor texture from above
	SetShaderTexture("plastic");

	//Set shader material
	SetShaderMaterial("wood");

	//Set Texture UV Scale for overlay texture
	SetTextureUVScale(2.f, 1.0f);

	//draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();

	SetTextureUVScale(1.f, 1.f);
}