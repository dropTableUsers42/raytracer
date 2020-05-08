#ifndef _TEXT_RENDERER_HPP
#define _TEXT_RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <algorithm>
#include <iostream>

#include "Shader.hpp"

struct Character
{
    float tx;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};

class TextRenderer
{
    Shader shader;
    GLuint VAO, VBO;
    GLuint atlasTex;
    std::map<GLchar, Character> Characters;
    int atlasWidth, atlasHeight;
    GLuint width, height;

public:
    TextRenderer(GLuint width, GLuint height);
    void RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
};

#endif