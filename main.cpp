#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include "graphics.h"
#include "jackal.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>
#include <vector>
#include "config.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <chrono>

#ifdef _WIN32 
#include "mingw.thread.h"
#endif

#ifdef __linux__
#include <threads>
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

std::vector<sprite> globaltilespritearray;
std::vector<sprite> globalobjectspritesarray;
std::vector<sprite> globalbgspritearray;
std::vector<sprite> zergsprite;
std::vector<sprite> bobsprite;

CAM GLOBCAM(0,0);

// settings
const unsigned int WIN_WIDTH = 256*4;
const unsigned int WIN_HEIGHT = 224*4;

long long RES_WIDTH = 256;
long long RES_HEIGHT = 224;

int bg[3] = {0,0,0};


// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WIN_WIDTH / 2.0f;
float lastY = WIN_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

float rectangleVertices[] = {
    1.0f, -1.0f,  1.0f,  0.0f,
   -1.0f, -1.0f,  0.0f,  0.0f,
   -1.0f,  1.0f,  0.0f,  1.0f,

    1.0f,  1.0f,  1.0f, 1.0f,
    1.0f, -1.0f,  1.0f, 0.0f,
   -1.0f,  1.0f,  0.0f, 1.0f
};

int distancecalc(int x1, int x2)
{
    // Calculating distance
    return (x2 - x1);
}

int postransfer[2];


drawsort globalsorter = drawsort();
player PIKO;
std::vector<blocktile> walgreens;
std::vector<blocktile> bgtiles;
std::vector<zerg> zergvec;

GLFWwindow* window;

        void LoadLVL(const char* readpath) {
            walgreens.clear();
            bgtiles.clear();
            zergvec.clear();
            //objectvec.clear();
            int layer = 1;

            int bgcount;
            int tlcount;
            int obcount;

            std::ifstream readfile(readpath, std::ios::binary);
            readfile.seekg (0, readfile.end);
            int length = readfile.tellg();
            readfile.seekg (0, readfile.beg);
            char * buffer = new char [length];

            readfile.read (buffer,length);
            readfile.close();


            int offset = 0;
            int rr;
            int gg;
            int bb;
            memcpy(&rr,&buffer[offset],sizeof(int));
            offset += sizeof(int);
            memcpy(&gg,&buffer[offset],sizeof(int));
            offset += sizeof(int);
            memcpy(&bb,&buffer[offset],sizeof(int));
            offset += sizeof(int);

            bg[0] = rr;
            bg[1] = gg;
            bg[2] = bb;

            memcpy(&bgcount,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&tlcount,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&obcount,&buffer[offset],sizeof(int));
            offset += sizeof(int);

            bgtiles.resize(bgcount);
            for(int i = 0; i < bgcount; i++) {
            int idi, xi, yi;
            memcpy(&idi,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&xi,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&yi,&buffer[offset],sizeof(int));
            offset += sizeof(int);

            bgtiles[i] = blocktile(xi, -(yi)-16,2,16,16,idi,1);
            }

            walgreens.resize(tlcount);
            for(int i = 0; i < tlcount; i++) {
            int idi;
            int xi;
            int yi;
            memcpy(&idi,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&xi,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&yi,&buffer[offset],sizeof(int));
            offset += sizeof(int);

            walgreens[i] = blocktile(xi,-(yi)-16,1,16,16,idi,0);

            }

            //objectvec.resize(obcount);
            for(int i = 0; i < obcount; i++) {
            int id, posx, posy;
            memcpy(&id,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&posx,&buffer[offset],sizeof(int));
            offset += sizeof(int);            
            memcpy(&posy,&buffer[offset],sizeof(int));
            offset += sizeof(int);

            if(id == 0) {
                PIKO.x = posx;
                PIKO.y = -posy-33;
            }

            if(id == 1) { //zerk snake
                zergvec.push_back(zerg(posx,-posy-16,16,16,1)); 
            }

            if(id == 2) { // bob

            }

            if(id == 3) { // life coupon

            }

            if(id == 4) { // money

            }

            if(id == 5) { // bomb

            }
            }

            delete[] buffer;
        }

        
int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



    // glfw window creation
    // --------------------
    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "jackal_engine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    stbi_set_flip_vertically_on_load(true);
    Shader fbShader("FrameBuffer.vs","FrameBuffer.fs");

    globaltilespritearray = {sprite(0,0,"GND1.png"), sprite(0,0,"BRICK.png"), sprite(0,0,"PBOX.png"), sprite(0,0,"EBOX.png"), sprite(0,0,"YLWTILE.png"),sprite(0,0,"BRG.png")};
    globalbgspritearray = {sprite(0,0,"CL1.png"), sprite(0,0,"CL2.png"), sprite(0,0,"CL3.png"), sprite(0,0,"CL4.png"),sprite(0,0,"BH1.png"), sprite(0,0,"BH2.png"), sprite(0,0,"TR1.png"), sprite(0,0,"TR2.png"),sprite(0,0,"FN1.png"), sprite(0,0,"FN2.png"), sprite(0,0,"FN3.png")};
    globalobjectspritesarray = {sprite(0,0,"PIKO.png"), sprite(0,0,"ZSNK.png"), sprite(0,0,"BOB.png"), sprite(0,0,"ELIF.png"), sprite(0,0,"MONY.png"),sprite(0,0,"BOMB.png")};
    std::vector<sprite> pikodefaultsprites{sprite(0,0,"PKN_1.png"),sprite(0,0,"PKN_2.png"),sprite(0,0,"PKN_3.png"),sprite(0,0,"PKN_4.png"),sprite(0,0,"PKN_5.png"),sprite(0,0,"PKN_6.png"),sprite(0,0,"PKN_7.png")};
    zergsprite = {sprite(0,0,"ZERP_1.png"),sprite(0,0,"ZERP_2.png")};
    bobsprite = {sprite(0,0,"BOB_1.png"), sprite(0,0,"BOB_2.png")};

    for(int i = 0; i < 7; i++) {
    
    PIKO.playersprite.push_back(&pikodefaultsprites[i]);
    }

    std::cout << "cumwater" << std::endl;
    unsigned int FBO;
    glGenFramebuffers(1,&FBO);
    glBindFramebuffer(GL_FRAMEBUFFER,FBO);

    std::cout << "level loading" << std::endl;
    LoadLVL("lvl");
    std::cout << "level successfully loaded" << std::endl;


    unsigned int framebufferTexture;
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D,framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,RES_WIDTH,RES_HEIGHT,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,framebufferTexture,0);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(fboStatus != GL_FRAMEBUFFER_COMPLETE) 
        std::cout << "FrameBuffer Error : " << fboStatus << std::endl;

    
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4 * sizeof(float), (void*)(2 * sizeof(float)));

    fbShader.use();
    fbShader.setInt("screenTexture",0);

    sprite player(0,0,"foxchan_5.png");
    sprite gnddes(0,0,"GND1.png");

    int frames = 0;
    auto start = std::chrono::steady_clock::now();
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);
        glBindFramebuffer(GL_FRAMEBUFFER,FBO);
        glViewport(0, 0, RES_WIDTH, RES_HEIGHT);
        float rrr = bg[0];
        float ggg = bg[1];
        float bbb = bg[2];
        glClearColor(rrr/256, ggg/256, bbb/256, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT); 
        ++frames;
        auto now = std::chrono::steady_clock::now();
        auto diff = now - start;
        auto end = now + std::chrono::milliseconds(16);
        if(diff >= std::chrono::seconds(1))
        {
            start = now;
            std::cout << "FPS: " << frames << std::endl;
            frames = 0;

        }
        // render the sprite

        //player.Draw(glm::vec2(0,40),glm::vec2(1),0,1.0f);
        PIKO.Control();

        for(int i = 0; i < zergvec.size(); i++) {
            zergvec[0].DoStuff();
        }

            //printf("BIG CHUNGUS %d                      \n", GLOBCAM.x*2);

        if(PIKO.x - GLOBCAM.x > 256/2 && PIKO.x > GLOBCAM.x) {
            GLOBCAM.x += ((PIKO.x - GLOBCAM.x) - 256/2);
        }
        postransfer[0] = PIKO.x;
        postransfer[1] = PIKO.y;
        for(int i = 0; i < bgtiles.size(); i++) {
            bgtiles[i].Draw();
        }


        for(int i = 0; i < walgreens.size(); i++) {
            walgreens[i].Draw();
        }
 
        for(int i = 0; i < zergvec.size(); i++) {
            zergvec[i].Draw();
        }

        PIKO.Draw();


        globalsorter.drawstack();
        globalsorter.resetstack();
        
        glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        fbShader.use();
        glBindVertexArray(rectVAO);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,framebufferTexture);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        std::this_thread::sleep_until(end);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
   // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
   //     glfwSetWindowShouldClose(window, true);
//
   // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
   //     camera.ProcessKeyboard(FORWARD, deltaTime);
   // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
   //     camera.ProcessKeyboard(BACKWARD, deltaTime);
   // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
   //     camera.ProcessKeyboard(LEFT, deltaTime);
   // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
   //     camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

 //   camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
