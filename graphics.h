#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <unistd.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
//#include <math.h>

#define sign(a) ( ( (a) < 0 )  ?  -1   : ( (a) > 0 ) )

using namespace std;


enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

extern const unsigned int WIN_WIDTH;
extern const unsigned int WIN_HEIGHT;

extern long long RES_WIDTH;
extern long long RES_HEIGHT;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f; 
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#define MAX_BONE_INFLUENCE 4

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();		
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();			
            // if geometry shader path is present, also load a geometry shader
            if(geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if(geometryPath != nullptr)
        {
            const char * gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if(geometryPath != nullptr)
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(geometryPath != nullptr)
            glDeleteShader(geometry);

    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) 
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    int width;
    int height;
    string type;
    string path;
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;

    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader &shader) 
    {
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to string
            else if(name == "texture_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to string
             else if(name == "texture_height")
                number = std::to_string(heightNr++); // transfer unsigned int to string

            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }
};

unsigned int TextureFromFile(const char *path,  string &directory, bool gamma = false);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};
//
//
//class object3D 
//{
//    public :
//    
//    void Load();
//    void Draw();
//
//
//    private:
//    
//}

class sprite 
{

    public:
        sprite(void) {
        };

        sprite(int originx, int originy, const char* imagename) {
        this->x = originx; 
        this->y = originy;
        this->shader = Shader("sprite.vs","sprite.fs");
        this->LoadTexture(imagename,"");
            // configure VAO/VBO
        unsigned int VBO;
        float spritevertices[] = { 
            // pos      // tex
            0.0f, (float)this->height,             0.0f, 1.0f,
            (float)this->width, 0.0f,              1.0f, 0.0f,
            0.0f, 0.0f,                          0.0f, 0.0f, 
        
            0.0f, (float)this->height,             0.0f, 1.0f,
            (float)this->width, (float)this->height, 1.0f, 1.0f,
            (float)this->width, 0.0f,              1.0f, 0.0f
        };

        glGenVertexArrays(1, &this->spriteVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(spritevertices), spritevertices, GL_STATIC_DRAW);

        glBindVertexArray(this->spriteVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);  
        glBindVertexArray(0);
        }

        void init(int originx, int originy, const char* imagename) {
        this->x = originx; 
        this->y = originy;
        this->shader = Shader("sprite.vs","sprite.fs");
        this->LoadTexture(imagename,"");
            // configure VAO/VBO
        unsigned int VBO;
        float spritevertices[] = { 
            // pos      // tex
            0.0f, (float)this->height,             0.0f, 1.0f,
            (float)this->width, 0.0f,              1.0f, 0.0f,
            0.0f, 0.0f,                          0.0f, 0.0f, 
        
            0.0f, (float)this->height,             0.0f, 1.0f,
            (float)this->width, (float)this->height, 1.0f, 1.0f,
            (float)this->width, 0.0f,              1.0f, 0.0f
        };

        glGenVertexArrays(1, &this->spriteVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(spritevertices), spritevertices, GL_STATIC_DRAW);

        glBindVertexArray(this->spriteVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);  
        glBindVertexArray(0);
        }



        float x,y;
        int width, height;
        Texture texture;
        Shader shader = Shader("sprite.vs","sprite.fs");
        unsigned int spriteVAO;
        void LoadTexture(const char* path,std::string directory);
        void LoadShader(const char* vspath, const char* fspath);
        //this one adds sprite to the draw call order
        void Draw(glm::vec2 pos, glm::vec2 scale, float rotate, float depth);
        //this one draw sprite without any changes basically meaning no-sorting
        void GraphicDraw(glm::vec2 pos, glm::vec2 scale, float rotate, float depth);
        
};

typedef struct {
    float x,y,depth,xscale,yscale, rotation;
    sprite* ptr2sprite;
}   spriteinfo;

class CAM {
    public :
    CAM(int xx,int yy) {
        this->x = xx;
        this->y = yy;
    };
   int x, y;
};

class drawsort {

    public :
    drawsort(void) {
        this->spritecount = 0;
    };

        int spritecount;
        std::vector<spriteinfo> arrayofsprites;

    void addspritetostack(sprite* ptr2spr, float xx, float yy, float xscale, float yscale, float rotate, float depth) {
        this->arrayofsprites.emplace_back();
        this->arrayofsprites[this->spritecount].x = xx;
        this->arrayofsprites[this->spritecount].y = yy;
        this->arrayofsprites[this->spritecount].depth = depth;
        this->arrayofsprites[this->spritecount].xscale = xscale;
        this->arrayofsprites[this->spritecount].yscale = yscale;
        this->arrayofsprites[this->spritecount].rotation = rotate;
        this->arrayofsprites[this->spritecount].ptr2sprite = ptr2spr;

        this->spritecount += 1;
    };


    void resetstack(void) {
        this->spritecount = 0;
        arrayofsprites.clear();
    }

    void drawstack(void) {
        std::sort(arrayofsprites.begin(), arrayofsprites.end(), +[](const spriteinfo& left, const spriteinfo& right) { return left.depth > right.depth; });

        for (spriteinfo& spritetbd : arrayofsprites)
            {
            spritetbd.ptr2sprite->GraphicDraw(glm::vec2(spritetbd.x,spritetbd.y),  glm::vec2(spritetbd.xscale,spritetbd.yscale), spritetbd.rotation,spritetbd.depth);
            }
    }
    
    private :
};

extern std::vector<sprite> globaltilespritearray;
extern std::vector<sprite> globalbgspritearray;
extern std::vector<sprite> globalobjectspritesarray;

extern std::vector<sprite> bobsprite;
extern std::vector<sprite> zergsprite;

extern CAM GLOBCAM;

//std::vector<blocktile> walgreens;

    extern GLFWwindow* window;


class blocktile {
    public :
    blocktile(void) {

    };
    blocktile(int x, int y,int depth, int sizex, int sizey, int tileid, int layer) {
        if(layer == 0)this->bsprite = &globaltilespritearray[tileid];
        if(layer == 1)this->bsprite = &globalbgspritearray[tileid];

        this->type = tileid;
        this->x = x;
        this->y = y;
        this->xsize = sizex;
        this->ysize = sizey;
        this->depth = depth;
    };

    int x, y, depth, xsize, ysize, type;
    sprite *bsprite;
    
    void Draw(void) {
        this->bsprite->Draw(glm::vec2((float)this->x,(float)this->y),glm::vec2(1.f),0,this->depth);
    };
};

class object {
    public :
    object(void) {

    };
    object(int x, int y,int depth, int sizex, int sizey, int tileid) {
        this->bsprite = &globalobjectspritesarray[tileid];
        this->type = tileid;
        this->behaviour = 0;
        this->x = x;
        this->y = y;
        this->xsize = sizex;
        this->ysize = sizey;
        this->depth = depth;
    };

    int x, y, depth, xsize, ysize, behaviour, type, xsp, ysp;
    sprite *bsprite;
    
    void Draw(void) {
        this->bsprite->Draw(glm::vec2((float)this->x,(float)this->y),glm::vec2(1.f),0,this->depth);
    };
};

extern std::vector<blocktile> walgreens;

int distancecalc(int x1, int x2);

class zerg {
    public :
    zerg(void) {
       x = 0;
       y = 0; 
    }
    zerg(int x, int y, int width, int height, int speed) {
        this->x = x;
        this->y = y;
        this->spd = speed;
        this->height = height;
        this->width = width;
        spritoid[0] = &zergsprite[0];
        spritoid[1] = &zergsprite[1];
    }
    
    sprite *spritoid[2];
    int x, y, direction, width, height, xsp, ysp, spd, activated;
    float frame;

    void isActive(void) {

            if(
                this->x - GLOBCAM.x < (int)RES_WIDTH
            ) activated = 1; else activated = 0;
            long long borbis = GLOBCAM.x;
            std::cout << "distance " << GLOBCAM.x << "      " << x + GLOBCAM.x << std::endl;
    }

    void DoStuff(void) {

            frame += 0.01;

            //if(frame >= 2) x += 1 * direction;
            if(frame >= 2) frame = 0;

    }

    void Draw(void) {
        this->spritoid[(int)frame]->Draw(glm::vec2((float)this->x,(float)this->y),glm::vec2(1.f),0,0);
    };
};

extern std::vector<zerg> zergvec;

class player {
    public :

    player(void) {
       this->gnd = 1;
       this->jmpval = 3;
       this->acc = 0.25f;
       this->dcc = 0.125;
       this->xsp = 0;
       this->ysp = 0;
       this->mxx = 1.f;
       this->steps = 0;
       this->lastdir = 1;
       this->isjumping = false;
        this->mxtimerjmp = 7;
        this->minjmptimer = 4;
        this->presseddownstill = false;
    };

    player(int x, int y, int sizex, int sizey, int depth, int framecount) { //framecount doesnt start from zero, this is because we initialize a vector here.
        this->x = x;
        this->y = y;
        this->sizex = sizex; 
        this->sizey = sizey; 
        this->depth = depth;
        this->xsp = 0;
        this->ysp = 0;
        this->frame = 0;
        this->framecount = framecount;
        this->playersprite.resize(framecount);
        this->acc = 0.25f;
        this->dcc = 0.125;
        this->jmpval = 1;
        this->gnd = 1;
        this->mxx = 1.f;
        this->lastdir = 1;
        this->isjumping = false;
        this->mxtimerjmp = 15;
        this->minjmptimer = 5;
        this->presseddownstill = false;
    };

    int x, y, sizex, sizey, depth, framecount, gnd, action, jmpval, steps, lastdir, offsetofx;
    float acc, dcc, xsp, ysp, mxx, frame, jmptimer, mxtimerjmp, minjmptimer;
    bool isjumping;
    bool presseddownstill;
    std::vector<sprite *> playersprite;

    void SetFrameSprite(int frame, sprite * spritestuff) { //counts from 0

        if(frame <= playersprite.size())this->playersprite[frame] = spritestuff;
        if(frame > playersprite.size()) this->playersprite.push_back(spritestuff);
    };

    bool BCol(float xoff, float yoff, blocktile rect) {
        
            return (
            this->x + xoff < rect.x + 16 && 
            this->x + xoff + 14 > rect.x &&
            this->y + yoff < rect.y + 16 && 
            this->y + yoff + 22 > rect.y);

    }



    void Control(void) {
        if((glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)) {
            if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gnd == 1)this->xsp -= this->acc*1.25; else if(this->gnd == 1)this->xsp -= this->acc; else if(this->gnd == 0) this->xsp -= acc/1.25f;

            if(this->gnd == 1)this->action = 1;
        }

        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS && this->action != -1) {
            if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)this->xsp += this->acc*1.25; else if(this->gnd == 1)this->xsp += this->acc; else if(this->gnd == 0) this->xsp += acc/1.25f;            
            if(this->gnd == 1)this->action = 1;
        }

        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && this->gnd == 1) {
            this->action = -1;
        }

        if(!glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && this->gnd == 1) {
            if(this->xsp == 0 && this->ysp == 0) this->action = 0;
        }


        if(this->action == 1 && xsp != 0 && gnd == 1) {
            if(this->xsp > 0 && glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS) action = 0;
            if(this->xsp < 0 && glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS) action = 0;
        }
        
        if((this->action == 0 || action == -1) && this->gnd == 1) {
            if(this->xsp > 0) this->xsp -= this->dcc;
            if(this->xsp < 0) this->xsp += this->dcc;

        }

        if((this->action == 0 || this->action == 1) && this->gnd == 1 && abs(xsp) > 0) {
            lastdir = sign(xsp);
        }

        if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) mxx = 2.0f; else mxx = 1.f;

        if(this->xsp > this->mxx) {
            this->xsp = this->mxx;
        }
       if(this->xsp < -this->mxx) {
            this->xsp = -this->mxx;
        }



        if(glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && this->gnd == 1 && presseddownstill == false) {
            this->ysp = this->jmpval;
            this->isjumping = true;
            this->gnd = 0;
            this->action = 2;
            presseddownstill = true;
        }


        if(isjumping == true) {
            jmptimer += 0.5f;
            if((((glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) && jmptimer < mxtimerjmp*(1+abs(xsp)*0.15))) || jmptimer < minjmptimer) isjumping = true; else isjumping = false;
        }
        int timesgndded = 0;
        for(int i = 0; i < walgreens.size(); i++) {
            if(BCol(0.f,ysp,walgreens[i]) == true) {
                gnd = 1;
                action = 0;
                jmptimer = 0;
                isjumping = false;
                //std::cout << "meepy   " << timesgndded + 1 << std::endl;
                timesgndded += 1;
            };
        };

        
        if(glfwGetKey(window, GLFW_KEY_J) != GLFW_PRESS && this->gnd == 1) presseddownstill = false;


        if(timesgndded == 0) this->gnd = 0;

        bool xcollided = false;

        if(gnd == 0 && isjumping == false) {
            ysp -= 0.425f;
        }

        for(int i = 0; i < walgreens.size(); i++) {
        //Horizontal collisions
        if(BCol(xsp,1.f,walgreens[i])) {
            while(!BCol(sign(xsp),1.f,walgreens[i])) {
                x += sign(xsp);
            }
            xcollided = true;
            //std::cout << "UR TOUCHING WALLS BABY" << std::endl;
        }};

        if(xcollided == false)this->x += (int)round(this->xsp);

        for(int i = 0; i < walgreens.size(); i++) {
        if(BCol(0.f,ysp +1.f,walgreens[i])) {
            while(!BCol(0.f,sign(ysp) + 1.f,walgreens[i])) {
                y += sign(ysp);
            }
            ysp = 0;
            //std::cout << "UR TOUCHING GRASS BABY" << std::endl;
        };
        };


        if(lastdir < 0) {
            offsetofx = 14;
        } else offsetofx = 0;

        if(this->action == 1 || this->action == 0 && abs(this->xsp) > 0) {
            frame += 0.125 * abs(xsp);
            if(frame >= 4) frame = 0;
        }

        if(this->action == 0 && this->xsp == 0) {
            this->frame = 4;
        }
        
        if(this->action == 2) {
            this->frame = 6;
        }

        //std::cout << "amount of x speed  " << this->xsp << std::endl;
        this->y += (int)round(this->ysp);
        
    }

    void Draw(void) {
        this->playersprite[(int)this->frame]->Draw(glm::vec2((float)this->x + offsetofx,(float)this->y),glm::vec2(this->lastdir,1),0,(float)this->depth);
    };

};

extern drawsort globalsorter;

extern unsigned int TextureFromFile(const char *path, string &directory, bool gamma);


#endif
