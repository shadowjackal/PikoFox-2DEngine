#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

#include "graphics.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

unsigned int TextureFromFile(const char *path, string &directory, bool gamma)
{
    string filename = string(path);
    string newdir = directory;
    int lenght = newdir.length();

    int lastof = newdir.find_last_of("/\\");
    if( lastof != -1) {
    newdir.erase(lastof+1,lenght-lastof+1);
    } else newdir = "";
    for(int i = 0; i < lenght; i++) {
    auto finder = newdir.rfind("\\");

    if(finder == -1) {
        break;
    }

    newdir.replace(finder,2,"/");
    }

    filename = newdir + filename;

    auto chungussy = filename.rfind("\\");
        if(chungussy != -1) {filename.replace(chungussy,1,"/");};
    

    std::cout << filename << std::endl;
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


        stbi_image_free(data);
    }
    else
    {
        std::cout << stbi_failure_reason() << std::endl;
        std::cout << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}







void sprite::LoadTexture(const char *path,std::string directory) {
    glGenTextures(1, &this->texture.id);
    this->texture.id = TextureFromFile(path,directory,false);

    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&this->texture.width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&this->texture.height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&this->width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&this->height);
    std::cout << "size of texture" << this->width << std::endl;
    this->texture.path = path;
}

void sprite::LoadShader(const char *vspath, const char *fspath) {
    this->shader = Shader(vspath,fspath);
}



void sprite::Draw(glm::vec2 pos, glm::vec2 scale, float rotate, float depth)
{
    globalsorter.addspritetostack(this, pos.x, pos.y, scale.x, scale.y, rotate, depth);
}

void sprite::GraphicDraw(glm::vec2 pos, glm::vec2 scale, float rotate, float depth)
{
    this->shader.use();
    glm::mat4 projection = glm::ortho(0.f, (float)RES_WIDTH, 0.f, (float)RES_HEIGHT, -50.f, 100.f);
        
    glm::mat4 model = glm::mat4(1.0f);
    model = projection * glm::translate(model, glm::vec3(this->x + pos.x + -GLOBCAM.x, this->y + pos.y + -GLOBCAM.y, depth));  
    model = glm::scale(model,glm::vec3(scale.x,scale.y,1));
    this->shader.setMat4("model", model);
    this->shader.setVec3("spriteColor", glm::vec4(1.0f,1.0f,1.0f,0.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,this->texture.id);
    glBindVertexArray(this->spriteVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

}