#pragma once

// GL Includes
#include "Platform/OpenGL.h"

#include <vector>
#include <cstring>
#include <cstdio>

#include "Platform/Assert.h"
#include <QImage>


class TextureLoading
{
public:

  unsigned int textureId; /**< The OpenGL id of the texture */
  int height; /**< The height of the texture */
  int width; /**< The width of the texture */
  unsigned char* imageData = nullptr; /**< The pixel data of the texture image */
  unsigned int byteOrder; /**< The byte order of texture image (GL_BGR, GL_BGRA, ...) */
  bool hasAlpha; /**< Whether the texture has an alpha channel or not */
  TextureLoading() = default;

  GLuint loadCubemap( std::vector<const GLchar * > faces)
  {
    GLuint textureID;
    ASSERT(!imageData);
    glGenTextures( 1, &textureID );

    glBindTexture( GL_TEXTURE_CUBE_MAP, textureID );

    for ( GLuint i = 0; i < faces.size( ); i++ )
    {
      load(faces[i]);
      glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData );
    }
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
  }

  bool load(const std::string& file)
  {

    if(file.length() >= 4)
    {
      std::string suffix = file.substr(file.length() - 4);
      if(strcasecmp(suffix.c_str(), ".tga") == 0)
        return loadTGA(file);
    }

    QImage image;
    if(image.load(file.c_str()))
    {
      if(image.format() != QImage::Format_ARGB32 &&
         image.format() != QImage::Format_RGB32 &&
         image.format() != QImage::Format_RGB888)
        return false;

      width = image.width();
      height = image.height();
      byteOrder = image.format() == QImage::Format_RGB888 ? GL_BGR : GL_BGRA;
      hasAlpha = image.hasAlphaChannel();
      imageData = new GLubyte[image.byteCount()];
      if(!imageData)
        return false;

      GLubyte* p = imageData;
      for(int y = height; y-- > 0;)
      {
        memcpy(p, image.scanLine(y), image.bytesPerLine());
        p += image.bytesPerLine();
      }
      return true;
    }
    else
      return false;
  }


  bool loadTGA(const std::string& name)
  {
    GLubyte TGAheader[12] = {0,0,2,0,0,0,0,0,0,0,0,0};   // Uncompressed TGA header
    GLubyte TGAcompare[12];                              // Used to compare TGA header
    GLubyte header[6];                                   // First 6 useful bytes of the header
    GLuint  bytesPerPixel;
    GLuint  imageSize;
    GLuint  bpp;

    FILE *file = fopen(name.c_str(), "rb");               // Open the TGA file

    // Load the file and perform checks
    if(file == nullptr ||                                                      // Does file exist?
       fread(TGAcompare,1,sizeof(TGAcompare),file) != sizeof(TGAcompare) ||  // Are there 12 bytes to read?
       memcmp(TGAheader,TGAcompare,sizeof(TGAheader)) != 0 ||                // Is it the right format?
       fread(header,1,sizeof(header),file) != sizeof(header))                // If so then read the next 6 header bytes
    {
      if(file == nullptr) // If the file didn't exist then return
        return false;
      else
      {
        fclose(file); // If something broke then close the file and return
        return false;
      }
    }

    // Determine the TGA width and height (highbyte*256+lowbyte)
    width  = header[1] * 256 + header[0];
    height = header[3] * 256 + header[2];

    // Check to make sure the targa is valid
    if(width <= 0 || height <= 0)
    {
      fclose(file);
      return false;
    }
    // Only 24 or 32 bit images are supported
    if( (header[4] != 24 && header[4] != 32) )
    {
      fclose(file);
      return false;
    }

    bpp = header[4];  // Grab the bits per pixel
    bytesPerPixel = bpp/8;  // Divide by 8 to get the bytes per pixel
    imageSize = width * height * bytesPerPixel; // Calculate the memory required for the data

    // Allocate the memory for the image data
    imageData = new GLubyte[imageSize];
    if(!imageData)
    {
      fclose(file);
      return false;
    }

    // Load the image data
    if(fread(imageData, 1, imageSize, file) != imageSize)  // Does the image size match the memory reserved?
    {
      delete [] imageData;  // If so, then release the image data
      fclose(file); // Close the file
      return false;
    }

    // We are done with the file so close it
    fclose(file);

    byteOrder = bpp == 24 ? GL_BGR : GL_BGRA;
    hasAlpha = byteOrder == GL_BGRA;
    return true;
  }
};
