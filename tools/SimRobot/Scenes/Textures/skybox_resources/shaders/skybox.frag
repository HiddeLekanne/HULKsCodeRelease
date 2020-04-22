#version 320 es
precision mediump float;
in vec3 TexCoords;
out vec4 color;

vec3 AlteredTexCoords;
uniform samplerCube skybox;

void main()
{
    AlteredTexCoords[0] = TexCoords[0];
    AlteredTexCoords[1] = TexCoords[2];
    AlteredTexCoords[2] = TexCoords[1];
    color = texture(skybox, AlteredTexCoords);
}

