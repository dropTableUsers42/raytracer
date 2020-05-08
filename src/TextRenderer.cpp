#include "TextRenderer.hpp"

#define PADDING 2

TextRenderer::TextRenderer(GLuint width, GLuint height) : shader("src/shaders/text.vs", "src/shaders/text.fs"), atlasWidth(0), atlasHeight(0), width(width), height(height)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    FT_Face face;
    if(FT_New_Face(ft, "C:\\Windows\\Fonts\\AgencyR.ttf", 0, &face))
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
    
    FT_Set_Pixel_Sizes(face, 0, 48);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(GLubyte c = 0; c < 128; c++)
    {
        if(FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph:\t" << (char)c << std::endl;
            continue;
        }

        atlasWidth += face->glyph->bitmap.width;
        atlasHeight = std::max(atlasWidth, (int)face->glyph->bitmap.rows);

        Character character = {
            0,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint)face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    atlasWidth += 127 * PADDING;

    glGenTextures(1, &atlasTex);
    glBindTexture(GL_TEXTURE_2D, atlasTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        atlasWidth,
        atlasHeight,
        0, 
        GL_RED, 
        GL_UNSIGNED_BYTE, 
        0
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int x = 0;
    for(GLubyte c = 0; c < 128; c++)
    {
        if(FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph:\t" << (char)c << std::endl;
            continue;
        }
        glTexSubImage2D(
            GL_TEXTURE_2D, 
            0, 
            x, 
            0, 
            face->glyph->bitmap.width, 
            face->glyph->bitmap.rows, 
            GL_RED, 
            GL_UNSIGNED_BYTE, 
            face->glyph->bitmap.buffer
        );
        Characters[c].tx = (float)x/atlasWidth;
        x += face->glyph->bitmap.width + PADDING;
    }
    
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), 0);
    glBindVertexArray(0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void TextRenderer::RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    glm::mat4 projection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
    shader.use();
    shader.setVec3("textColor", color);
    shader.setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::string::const_iterator c;
    for(c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x;
        GLfloat w_s = w * scale;
        GLfloat h = ch.Size.y;
        GLfloat h_s = h * scale;
        GLfloat tx = ch.tx;

        GLfloat vertices[6][4] = {
            {xpos, ypos + h_s, tx, 0.0},
            {xpos, ypos, tx, h/atlasHeight},
            {xpos + w_s, ypos, tx + w/atlasWidth, h/atlasHeight},
            {xpos, ypos + h_s, tx, 0.0},
            {xpos + w_s, ypos, tx + w/atlasWidth, h/atlasHeight},
            {xpos + w_s, ypos + h_s, tx + w/atlasWidth, 0.0}
        };

        glBindTexture(GL_TEXTURE_2D, atlasTex);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}