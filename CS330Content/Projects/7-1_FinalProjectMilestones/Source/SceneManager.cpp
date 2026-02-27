///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
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
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 model;
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
	translation = glm::translate(positionXYZ + offset);

	model = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, model);
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
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// load all scene textures 
	CreateGLTexture("textures/snowman_texture.jpg", "snowman_body");
	CreateGLTexture("textures/wood_sticks_texture.jpg", "snowman_arms");
	CreateGLTexture("textures/carrot_texture.jpg", "snowman_nose");
	CreateGLTexture("textures/house_sides.jpg", "house_sides");
	CreateGLTexture("textures/house_front.jpg", "house_front");
	CreateGLTexture("textures/house_roof.jpg", "house_roof");
	CreateGLTexture("textures/snow_texture.png", "ground");
	CreateGLTexture("textures/leaves.png", "leaves");
	CreateGLTexture("textures/tree_bark.jpg", "tree_bark");
	CreateGLTexture("textures/streetlamp.png", "light");
	CreateGLTexture("textures/bg.jpg", "background");

	// bind all loaded textures to texture slots
	BindGLTextures();
}

/***********************************************************
*  DefineObjectMaterials()
*
*  This method is used for configuring the various material
*  settings for all of the objects within the 3D scene.
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Base material to simulate a moderate, middle-ground surface that is neither too glossy nor too dull
	OBJECT_MATERIAL baseMaterial;
	baseMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	baseMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	baseMaterial.shininess = 32.0;
	baseMaterial.tag = "base";

	m_objectMaterials.push_back(baseMaterial);

	// Snow material to simulate the snow on the ground and the snowman
	OBJECT_MATERIAL snowMaterial;
	snowMaterial.diffuseColor = glm::vec3(0.77f, 0.77f, 0.77f);
	snowMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.77f);
	snowMaterial.shininess = 75.0;
	snowMaterial.tag = "snow";

	m_objectMaterials.push_back(snowMaterial);

	// Metal material to simulate the pole and top of the streetlamp
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.77f, 0.77f, 0.77f);
	metalMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	metalMaterial.shininess = 85.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	// Glass material to simulate the light of the streetlamp
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	// Wood material to simulate the house wood (slightly shinier due to the snow on it) 
	OBJECT_MATERIAL houseWoodMaterial;
	houseWoodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.2f);
	houseWoodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	houseWoodMaterial.shininess = 12.0;
	houseWoodMaterial.tag = "house_wood";

	m_objectMaterials.push_back(houseWoodMaterial);

	// Wood material to simulate the rest of the wooden objects
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 4.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// Leaves  material to simulate the leaves of the tree
	OBJECT_MATERIAL leavesMaterial;
	leavesMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.2f);
	leavesMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.7f);
	leavesMaterial.shininess = 42.0;
	leavesMaterial.tag = "foliage";

	m_objectMaterials.push_back(leavesMaterial);

	// Material for the background
	OBJECT_MATERIAL backgroundMaterial;
	backgroundMaterial.diffuseColor = glm::vec3(0.5f, 0.65f, 0.85f);
	backgroundMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.22f);
	backgroundMaterial.shininess = 10.0;
	backgroundMaterial.tag = "background";

	m_objectMaterials.push_back(backgroundMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light to simulate clouded sunlight on a snowy afternoon 
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -0.4f, -0.4f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.5f, 0.55f, 0.55f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.5f, 0.5f, 0.65f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Directional light to simulate slight moonlight on a snowy night
	//m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -0.4f, -0.4f);
	//m_pShaderManager->setVec3Value("directionalLight.ambient", 0.06f, 0.064f, 0.094f);
	//m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.06f, 0.06f, 0.095f);
	//m_pShaderManager->setVec3Value("directionalLight.specular", 0.05f, 0.05f, 0.05f);
	//m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1 (index 0) to simulate a warm light spilling out of the house window onto the surrounding environment
	m_pShaderManager->setVec3Value("pointLights[0].position", 11.0f, 1.001f, -4.5f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.05f, 0.0f);  
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.35f, 0.525f, 0.05f);  
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.15f, 0.1f, 0.05f);  
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Spotlight to simulate light falling onto the snow-covered ground outside the house through the window
	m_pShaderManager->setVec3Value("spotLights[0].position", 7.7f, 5.5f, -0.5f);
	m_pShaderManager->setVec3Value("spotLights[0].direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("spotLights[0].ambient", 0.2f, 0.1f, 0.0f);
	m_pShaderManager->setVec3Value("spotLights[0].diffuse", 2.7f, 1.05f, 0.1f);
	m_pShaderManager->setVec3Value("spotLights[0].specular", 0.6f, 0.4f, 0.2f);
	m_pShaderManager->setFloatValue("spotLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLights[0].quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLights[0].cutOff", glm::cos(glm::radians(5.0f)));
	m_pShaderManager->setFloatValue("spotLights[0].outerCutOff", glm::cos(glm::radians(40.0f)));
	m_pShaderManager->setBoolValue("spotLights[0].bActive", true);

	// Spotlight to simulate light from the streetlamp
	m_pShaderManager->setVec3Value("spotLights[1].position", 2.2f, 7.7f, 7.0f);
	m_pShaderManager->setVec3Value("spotLights[1].direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("spotLights[1].ambient", 0.2f, 0.1f, 0.0f);
	m_pShaderManager->setVec3Value("spotLights[1].diffuse", 3.7f, 2.15f, 1.1f);
	m_pShaderManager->setVec3Value("spotLights[1].specular", 0.6f, 0.4f, 0.2f);
	m_pShaderManager->setFloatValue("spotLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLights[1].quadratic", 0.062f);
	m_pShaderManager->setFloatValue("spotLights[1].cutOff", glm::cos(glm::radians(10.0f)));
	m_pShaderManager->setFloatValue("spotLights[1].outerCutOff", glm::cos(glm::radians(30.0f)));
	m_pShaderManager->setBoolValue("spotLights[1].bActive", true);

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
	//load the scene textures
	LoadSceneTextures();
	// define the materials that will be used for the objects in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	{
		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set texture for the ground
		SetShaderTexture("ground");
		
		// set material to snow
		SetShaderMaterial("snow");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();
		/****************************************************************/
	}

	{
		//Snowman for the scene
		/****************************************************************/
		//offset vector for snowman
		glm::vec3 offsetVecForSnowman = glm::vec3(0.0f, 1.5f, 6.0f);

		// -------- Bottom sphere for the snowman --------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set texture for the snowman's body
		SetShaderTexture("snowman_body");
		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		// --------- Top sphere for the snowman ---------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.85f, 0.85f, 0.85f);
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 2.095f, 0.0f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set texture for the snowman's body
		SetShaderTexture("snowman_body");
		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		// ------------ Cone for the snowman ------------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.109f, 0.45f, 0.109f);
		// Rotate the cone along the X axis
		XrotationDegrees = 100.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 1.9f, 0.82f);
		// set the transformations into memory
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set texture for the snowman's nose
		SetShaderTexture("snowman_nose");
		// set material for the carrot (neither too glossy nor too dull)
		SetShaderMaterial("base");
		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		// ------- Right eyeball for the snowman --------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.109f, 0.109f, 0.109f);
		// Rotate the eyeball along the X axis
		XrotationDegrees = 85.0f;
		// Rotate the eyeball along the Y axis
		YrotationDegrees = 20.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.29f, 2.25f, 0.75f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set eyeball color to 0.2f to match it to the eyeball in the 2d image
		SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
		// draw the mesh with transformation values
		m_basicMeshes->DrawHalfSphereMesh();

		// ------- Left eyeball for the snowman ---------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.109f, 0.109f, 0.109f);
		// Rotate the eyeball along the X axis
		XrotationDegrees = 85.0f;
		// Rotate the eyeball along the Y axis
		YrotationDegrees = -20.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-0.29f, 2.25f, 0.75f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set eyeball color to 0.2f to match it to the eyeball in the 2d image
		SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
		// draw the mesh with transformation values
		m_basicMeshes->DrawHalfSphereMesh();

		// ------- Right cylinder for the snowman --------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.065f, 1.5f, 0.065f);
		// Rotate the cylinder along the X axis
		XrotationDegrees = 15.0f;
		// Rotate the cylinder along the Y axis
		YrotationDegrees = 70.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(1.1f, 0.9f, 0.2f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set texture for the snowman's arm
		SetShaderTexture("snowman_arms");
		// set material for the wood
		SetShaderMaterial("wood");
		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// -------- Left cylinder for the snowman --------
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.065f, 1.5f, 0.065f);
		// Rotate the cylinder along the X axis
		XrotationDegrees = 325.0f;
		// Rotate the cylinder along the Y axis
		YrotationDegrees = 70.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-1.1f, 0.9f, 0.2f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForSnowman);
		// set texture for the snowman's arm
		SetShaderTexture("snowman_arms");
		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
		/****************************************************************/
	}

	{
		//House for the scene
		/****************************************************************/
		//offset vector for house
		glm::vec3 offsetVecForHouse = glm::vec3(11.0f, 4.001f, -4.5f);

		// --------- Box for the house body ---------
		scaleXYZ = glm::vec3(8.0f, 8.0f, 8.0f);
		// set rotation values
		XrotationDegrees = 0.0f;
		YrotationDegrees = -40.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForHouse);
		// set the texture for the house front
		SetShaderTexture("house_front");
		// set the material for the house wood
		SetShaderMaterial("house_wood");
		// draw the front-facing side of the house 
		m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);
		// set texture for the house body
		SetShaderTexture("house_sides");
		// draw the sides of the house with transformation values
		m_basicMeshes->DrawBoxMesh();

		// -------- Pyramid for the house roof --------
		scaleXYZ = glm::vec3(9.9f, 5.5f, 9.5f);
		// set the XYZ position 
		positionXYZ = glm::vec3(0.0f, 6.0f, 0.0f);
		// set the transformations into memory 
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForHouse);
		// set texture for the house roof
		SetShaderTexture("house_roof");
		// draw the mesh with transformation values
		m_basicMeshes->DrawPyramid4Mesh();
	}

	{
		//Tree for the scene
		/****************************************************************/
		//offset vector for tree
		glm::vec3 offsetVecForTree = glm::vec3(-9.5f, 0.0f, 1.0f);

		//------------Tree Trunk--------------
		scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.01f, 0.0f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForTree);
		// set texture for the trunk
		SetShaderTexture("tree_bark");
		// set material to trunk
		SetShaderMaterial("wood");
		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		//------------Tree Leaves--------------
		scaleXYZ = glm::vec3(3.6f, 10.5f, 3.6f);
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 2.01f, 0.0f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForTree);
		// set texture for the leaves
		SetShaderTexture("leaves");
		// set material to leaves
		SetShaderMaterial("foliage");
		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();
	}

	{
		//Streetlamp for the scene
		/****************************************************************/
		//offset vector for streetlamp
		glm::vec3 offsetVecForStreetlamp = glm::vec3(2.0f, 0.0f, 7.0f);

		//--------------- Pole -----------------
		scaleXYZ = glm::vec3(0.2f, 7.7f, 0.2f);
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.01f, 0.0f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForStreetlamp);
		// set color for the pole
		SetShaderColor(0.1, 0.1, 0.1, 1);
		// set material
		SetShaderMaterial("metal");
		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		//-------------- Light -----------------
		scaleXYZ = glm::vec3(0.55f, 1.15f, 0.55f);
		// rotate the cylinder upside down
		XrotationDegrees = 180.0f;
		// rotate the cylinder along the y axis
		YrotationDegrees = 85.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 7.71f, 0.0f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForStreetlamp);
		// set texture for the streetlamp light
		SetShaderTexture("light");
		// set material
		SetShaderMaterial("glass");
		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		//--------------- Top ---------------
		scaleXYZ = glm::vec3(0.55f, 0.35f, 0.55f);
		// reset x rotation degrees
		XrotationDegrees = 0.0f;
		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 7.71f, 0.0f);
		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ, offsetVecForStreetlamp);
		// set color
		SetShaderColor(0.1, 0.1, 0.1, 1);
		// set material
		SetShaderMaterial("metal");
		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();
	}

	{
		// Background for the scene
		/******************************************************************/
		// set the XYZ scale 
		scaleXYZ = glm::vec3(30.0f, 1.0f, 20.0f);
		// set rotation values
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		// set the XYZ position 
		positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		// set texture for the background
		SetShaderTexture("background");
		// set material 
		SetShaderMaterial("background");
		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();
		/****************************************************************/
	}
}
