// Std. Includes
#include <string>

// GLEW
#include "Platform/OpenGL.h"

// GLFW
//#include <GLFW/glfw3.h>

// GL includes
#include "Shader.h"

// GLM Mathemtics
//#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
// Other Libs
//#include "SOIL2/SOIL2.h"

#include "SkyBoxTexture.h"

class SkyBox
{
public:
  // Setup and compile our shaders
  Shader skyboxShader = Shader("Textures/skybox_resources/shaders/skybox.vs", "Textures/skybox_resources/shaders/skybox.frag");
  GLuint skyboxVAO = 0;
  GLuint skyboxVBO = 0;
//  float cameraTransformation[16];


  GLuint cubemapTexture = 0;
  static GLfloat skyBoxVertices[108];

  SkyBox();

  void Render(float cameraTransformation[16], float projection[16]);
};
