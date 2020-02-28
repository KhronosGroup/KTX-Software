

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
using namespace std;

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
//#include <assimp/cimport.h>

#include <GL/glcorearb.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

//#include <glad/glad.h>

namespace glMeshLoader
{
    typedef enum VertexLayout {
        VERTEX_LAYOUT_POSITION = 0x0,
        VERTEX_LAYOUT_NORMAL = 0x1,
        VERTEX_LAYOUT_COLOR = 0x2,
        VERTEX_LAYOUT_UV = 0x3,
        VERTEX_LAYOUT_TANGENT = 0x4,
        VERTEX_LAYOUT_BITANGENT = 0x5,
        VERTEX_LAYOUT_DUMMY_FLOAT = 0x6,
        VERTEX_LAYOUT_DUMMY_VEC4 = 0x7
    } VertexLayout;

    struct MeshBufferInfo
    {
        GLuint name = 0;
        size_t size = 0;
    };

    struct MeshBuffer
    {
        GLuint vao = 0;
        MeshBufferInfo vertices;
        MeshBufferInfo indices;
        uint32_t indexCount = 0;
        glm::vec3 dim;
        glm::mat4 modelTransform;  // To display the model correctly in the
                                   // GL coordinate system.

        void FreeGLResources() {
            if (vertices.name)
                glDeleteBuffers(1, &vertices.name);
            if (indices.name)
                glDeleteBuffers(1, &indices.name);
            if (vao)
                glDeleteVertexArrays(1, &vao);
        }

        ~MeshBuffer() {
            FreeGLResources();
        }

        glm::mat4& getModelTransform() { return modelTransform; }

        void Draw() {
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, indexCount,  GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    };

    // Get vertex size from vertex layout
    static uint32_t vertexSize(std::vector<glMeshLoader::VertexLayout> layout)
    {
        uint32_t vSize = 0;
        for (auto& layoutDetail : layout)
        {
            switch (layoutDetail)
            {
            // UV only has two components
            case VERTEX_LAYOUT_UV:
                vSize += 2 * sizeof(float);
                break;
            default:
                vSize += 3 * sizeof(float);
            }
        }
        return vSize;
    }
};

#if 0
unsigned int TextureFromFile(const char *path, const string &directory,
                             bool gamma = false);

class Model
{
  public:
    /*  Model Data */
    vector<Texture> textures_loaded;
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;

    /*  Functions   */
    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

};
#endif

// Simple mesh class for getting all the necessary stuff from models
// loaded via ASSIMP.
class GLMeshLoader {
  private:
    struct Vertex
    {
        glm::vec3 m_pos;
        glm::vec2 m_tex;
        glm::vec3 m_normal;
        glm::vec3 m_color;
        glm::vec3 m_tangent;
        glm::vec3 m_binormal;

        Vertex() {}

        Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& color)
        {
            m_pos = pos;
            m_tex = tex;
            m_normal = normal;
            m_color = color;
            m_tangent = tangent;
            m_binormal = bitangent;
        }
    };

    struct MeshEntry {
        uint32_t NumIndices;
        uint32_t MaterialIndex;
        uint32_t vertexBase;
        std::vector<Vertex> Vertices;
        std::vector<unsigned int> Indices;
    };

    aiMatrix4x4 rootTransform;

  public:
    #if defined(__ANDROID__)
        AAssetManager* assetManager = nullptr;
    #endif

    std::vector<MeshEntry> m_Entries;

    struct Dimension
    {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
        glm::vec3 size;
    } dim;

    uint32_t numVertices = 0;

    Assimp::Importer importer;
    const aiScene* pScene;

    ~GLMeshLoader()
    {
        m_Entries.clear();
    }

    // Load a mesh with some default flags
    void LoadMesh(const std::string& filename)
    {
        int flags = aiProcess_Triangulate | aiProcess_PreTransformVertices
                  | aiProcess_CalcTangentSpace
                  | aiProcess_GenSmoothNormals;

        LoadMesh(filename, flags);
    }

    // Load the mesh with custom flags
    void LoadMesh(const std::string& filename, int flags)
    {
#if defined(__ANDROID__)
        // Meshes are stored inside the apk on Android (compressed)
        // So they need to be loaded via the asset manager

        AAsset* asset = AAssetManager_open(assetManager, filename.c_str(),
                                           AASSET_MODE_STREAMING);
        assert(asset);
        size_t size = AAsset_getLength(asset);

        assert(size > 0);

        void *meshData = malloc(size);
        AAsset_read(asset, meshData, size);
        AAsset_close(asset);

        pScene = importer.ReadFileFromMemory(meshData, size, flags);

        free(meshData);
#else
        pScene = importer.ReadFile(filename.c_str(), flags);
#endif
      if(!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
         || !pScene->mRootNode)
        {
            std::stringstream message;

            message << "  Import via ASSIMP from\"" << filename << "\" failed. "
                    << importer.GetErrorString() << endl;
            throw std::runtime_error(message.str());
        }

        InitFromScene(pScene, filename);
#if 0
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
#endif
        rootTransform = pScene->mRootNode->mTransformation;
    }

    void InitFromScene(const aiScene* pScene, const std::string& filename)
    {
        m_Entries.resize(pScene->mNumMeshes);

        // Counters
        for (unsigned int i = 0; i < m_Entries.size(); i++)
        {
            m_Entries[i].vertexBase = numVertices;
            numVertices += pScene->mMeshes[i]->mNumVertices;
        }

        // Initialize the meshes in the scene one by one
        for (unsigned int i = 0; i < m_Entries.size(); i++)
        {
            const aiMesh* paiMesh = pScene->mMeshes[i];
            InitMesh(i, paiMesh, pScene);
        }
    }

    void InitMesh(unsigned int index, const aiMesh* paiMesh, const aiScene* pScene)
    {
        m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;

        aiColor3D pColor(0.f, 0.f, 0.f);
        pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

        aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

        for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
            aiVector3D* pPos = &(paiMesh->mVertices[i]);
            aiVector3D* pNormal = &(paiMesh->mNormals[i]);
            aiVector3D *pTexCoord;
            if (paiMesh->HasTextureCoords(0))
            {
                pTexCoord = &(paiMesh->mTextureCoords[0][i]);
            }
            else {
                pTexCoord = &Zero3D;
            }
            aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ?
                                   &(paiMesh->mTangents[i]) : &Zero3D;
            aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ?
                                     &(paiMesh->mBitangents[i]) : &Zero3D;

            Vertex v(glm::vec3(pPos->x, -pPos->y, pPos->z),
                glm::vec2(pTexCoord->x , pTexCoord->y),
                glm::vec3(pNormal->x, pNormal->y, pNormal->z),
                glm::vec3(pTangent->x, pTangent->y, pTangent->z),
                glm::vec3(pBiTangent->x, pBiTangent->y, pBiTangent->z),
                glm::vec3(pColor.r, pColor.g, pColor.b)
                );

            dim.max.x = fmax(pPos->x, dim.max.x);
            dim.max.y = fmax(pPos->y, dim.max.y);
            dim.max.z = fmax(pPos->z, dim.max.z);

            dim.min.x = fmin(pPos->x, dim.min.x);
            dim.min.y = fmin(pPos->y, dim.min.y);
            dim.min.z = fmin(pPos->z, dim.min.z);

            m_Entries[index].Vertices.push_back(v);
        }

        dim.size = dim.max - dim.min;

        for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
        {
            const aiFace& Face = paiMesh->mFaces[i];
            if (Face.mNumIndices != 3)
                continue;
            m_Entries[index].Indices.push_back(Face.mIndices[0]);
            m_Entries[index].Indices.push_back(Face.mIndices[1]);
            m_Entries[index].Indices.push_back(Face.mIndices[2]);
        }
    }

    void CreateBuffers(glMeshLoader::MeshBuffer& meshBuffer,
                       vector<glMeshLoader::VertexLayout> layout,
                       float scale)
    {
        std::vector<float> vertexBuffer;
        for (uint32_t m = 0; m < m_Entries.size(); m++)
        {
            for (uint32_t i = 0; i < m_Entries[m].Vertices.size(); i++)
            {
                // Push vertex data depending on layout
                for (auto& layoutDetail : layout)
                {
                    // Position
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_POSITION)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
                    }
                    // Normal
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_NORMAL)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
                        vertexBuffer.push_back(-m_Entries[m].Vertices[i].m_normal.y);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
                    }
                    // Texture coordinates
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_UV)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
                    }
                    // Color
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_COLOR)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
                    }
                    // Tangent
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_TANGENT)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
                    }
                    // Bitangent
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_BITANGENT)
                    {
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
                        vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
                    }
                    // Dummy layout components for padding
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_DUMMY_FLOAT)
                    {
                        vertexBuffer.push_back(0.0f);
                    }
                    if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_DUMMY_VEC4)
                    {
                        vertexBuffer.push_back(0.0f);
                        vertexBuffer.push_back(0.0f);
                        vertexBuffer.push_back(0.0f);
                        vertexBuffer.push_back(0.0f);
                    }
                }
            }
        }
        meshBuffer.vertices.size = vertexBuffer.size() * sizeof(float);

        dim.min *= scale;
        dim.max *= scale;
        dim.size *= scale;

        meshBuffer.modelTransform[0][0] = rootTransform.a1;
        meshBuffer.modelTransform[0][1] = rootTransform.b1;
        meshBuffer.modelTransform[0][2] = rootTransform.c1;
        meshBuffer.modelTransform[0][3] = rootTransform.d1;
        meshBuffer.modelTransform[1][0] = rootTransform.a2;
        meshBuffer.modelTransform[1][1] = rootTransform.b2;
        meshBuffer.modelTransform[1][2] = rootTransform.c2;
        meshBuffer.modelTransform[1][3] = rootTransform.d2;
        meshBuffer.modelTransform[2][0] = rootTransform.a3;
        meshBuffer.modelTransform[2][1] = rootTransform.b3;
        meshBuffer.modelTransform[2][2] = rootTransform.c3;
        meshBuffer.modelTransform[2][3] = rootTransform.d3;
        meshBuffer.modelTransform[3][0] = rootTransform.a4;
        meshBuffer.modelTransform[3][1] = rootTransform.b4;
        meshBuffer.modelTransform[3][2] = rootTransform.c4;
        meshBuffer.modelTransform[3][3] = rootTransform.d4;

        std::vector<uint32_t> indexBuffer;
        for (uint32_t m = 0; m < m_Entries.size(); m++)
        {
            uint32_t indexBase = (uint32_t)indexBuffer.size();
            for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++)
            {
                indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
            }
        }
        meshBuffer.indices.size = indexBuffer.size() * sizeof(uint32_t);
        meshBuffer.indexCount = (uint32_t)indexBuffer.size();

          // Make the GL buffers

        glGenVertexArrays(1, &meshBuffer.vao);
        glBindVertexArray(meshBuffer.vao);

        // Setup vertices.
        glGenBuffers(1, &meshBuffer.vertices.name);
        glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.vertices.name);

        // Create the buffer data store and upload the vertices.
        glBufferData(GL_ARRAY_BUFFER, meshBuffer.vertices.size,
                     vertexBuffer.data(), GL_STATIC_DRAW);

        GLuint attribNumber = 0;
        GLsizeiptr attribOffset = 0;
        GLsizei stride = vertexSize(layout);
        for (auto& layoutDetail : layout)
        {
            // Position
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_POSITION) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 3, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 3;
                attribNumber++;
            }
            // Normal
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_NORMAL) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 3, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 3;
                attribNumber++;
            }
            // Texture coordinates
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_UV) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 2, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 2;
                attribNumber++;
            }
            // Color
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_COLOR) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 3, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 3;
                attribNumber++;
            }
            // Tangent
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_TANGENT) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 3, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 3;
                attribNumber++;
            }
            // Bitangent
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_BITANGENT) {
                glEnableVertexAttribArray(attribNumber);
                glVertexAttribPointer(attribNumber, 3, GL_FLOAT, GL_FALSE,
                                      stride, (void *)attribOffset);
                attribOffset += sizeof(float) * 3;
                attribNumber++;
            }
            // Dummy layout components for padding
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_DUMMY_FLOAT) {
                vertexBuffer.push_back(0.0f);
            }
            if (layoutDetail == glMeshLoader::VERTEX_LAYOUT_DUMMY_VEC4) {
                vertexBuffer.push_back(0.0f);
                vertexBuffer.push_back(0.0f);
                vertexBuffer.push_back(0.0f);
                vertexBuffer.push_back(0.0f);
            }
        }

        // Setup indices
        glGenBuffers(1, &meshBuffer.indices.name);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.indices.name);
        // Create the buffer data store and upload the elements.
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.indices.size,
                     indexBuffer.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
};

#if 0
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

        // Walk through each of the mesh's vertices
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
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
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


unsigned int
TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif
