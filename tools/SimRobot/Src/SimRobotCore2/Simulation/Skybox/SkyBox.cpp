//
// Created by hidde on 20/04/2020.
//
#include "SkyBox.h"

SkyBox::SkyBox()
{
  // OpenGL options
  glEnable(GL_DEPTH_TEST);
  // Setup skybox VAO
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyBoxVertices), &skyBoxVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glBindVertexArray(0);

  // Cubemap (Skybox)
  std::vector<const GLchar*> faces;
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/right.tga");
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/left.tga");
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/top.tga");
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/bottom.tga");
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/front.tga");
  faces.push_back("Textures/skybox_resources/skyboxes/artist_lab/back.tga");
  TextureLoading something = TextureLoading();
  cubemapTexture = something.loadCubemap(faces);
}

void SkyBox::Render(float cameraTransformation[16], float projection[16])
{
  // Render
  glDisable(GL_CULL_FACE);
  skyboxShader.Use();

  cameraTransformation[14] = 0;
  cameraTransformation[13] = 0;
  cameraTransformation[12] = 0;

  glUniformMatrix4fv( glGetUniformLocation( skyboxShader.Program, "view" ), 1, GL_FALSE, cameraTransformation);
  glUniformMatrix4fv( glGetUniformLocation( skyboxShader.Program, "projection" ), 1, GL_FALSE, projection);

  // skybox cube
  glBindVertexArray(skyboxVAO);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  skyboxShader.DeSelect();
//  glDepthFunc(GL_LEQUAL); // Change depth function so depth test passes when values are equal to
  glEnable(GL_CULL_FACE);
}

float SkyBox::skyBoxVertices[108] = {
// Positions
-1.0f, 1.0f,  -1.0f,
-1.0f, -1.0f, -1.0f,
1.0f,  -1.0f, -1.0f,
1.0f,  -1.0f, -1.0f,
1.0f,  1.0f,  -1.0f,
-1.0f, 1.0f,  -1.0f,

-1.0f, -1.0f, 1.0f,
-1.0f, -1.0f, -1.0f,
-1.0f, 1.0f,  -1.0f,
-1.0f, 1.0f,  -1.0f,
-1.0f, 1.0f,  1.0f,
-1.0f, -1.0f, 1.0f,

1.0f,  -1.0f, -1.0f,
1.0f,  -1.0f, 1.0f,
1.0f,  1.0f,  1.0f,
1.0f,  1.0f,  1.0f,
1.0f,  1.0f,  -1.0f,
1.0f,  -1.0f, -1.0f,

-1.0f, -1.0f, 1.0f,
-1.0f, 1.0f,  1.0f,
1.0f,  1.0f,  1.0f,
1.0f,  1.0f,  1.0f,
1.0f,  -1.0f, 1.0f,
-1.0f, -1.0f, 1.0f,

-1.0f, 1.0f,  -1.0f,
1.0f,  1.0f,  -1.0f,
1.0f,  1.0f,  1.0f,
1.0f,  1.0f,  1.0f,
-1.0f, 1.0f,  1.0f,
-1.0f, 1.0f,  -1.0f,

-1.0f, -1.0f, -1.0f,
-1.0f, -1.0f, 1.0f,
1.0f,  -1.0f, -1.0f,
1.0f,  -1.0f, -1.0f,
-1.0f, -1.0f, 1.0f,
1.0f,  -1.0f, 1.0f
};

