/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "disable_glm_warnings.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "reenable_warnings.h"

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
        GLuint primitiveType;
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
}

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
        uint32_t primitiveType;
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
                  | aiProcess_JoinIdenticalVertices
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
                    << importer.GetErrorString() << std::endl;
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

    void InitFromScene(const aiScene* pSrcScene, const std::string& /*filename*/)
    {
        m_Entries.resize(pSrcScene->mNumMeshes);

        // Counters
        for (unsigned int i = 0; i < m_Entries.size(); i++)
        {
            m_Entries[i].vertexBase = numVertices;
            numVertices += pSrcScene->mMeshes[i]->mNumVertices;
        }

        // Initialize the meshes in the scene one by one
        for (unsigned int i = 0; i < m_Entries.size(); i++)
        {
            const aiMesh* paiMesh = pSrcScene->mMeshes[i];
            InitMesh(i, paiMesh, pSrcScene);
        }
    }

    void InitMesh(unsigned int index, const aiMesh* paiMesh, const aiScene* pSrcScene)
    {
        m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;

        aiColor3D pColor(0.f, 0.f, 0.f);
        pSrcScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

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

        switch (paiMesh->mPrimitiveTypes) {
          case aiPrimitiveType_POINT:
            m_Entries[index].primitiveType = GL_POINTS ;
            break;
          case aiPrimitiveType_LINE:
            m_Entries[index].primitiveType = GL_LINES;
            break;
          case aiPrimitiveType_TRIANGLE:
            m_Entries[index].primitiveType = GL_TRIANGLES;
            break;
          default: assert(false); // Shouldn't happen because of triangulate.
        }
    }

    void CreateBuffers(glMeshLoader::MeshBuffer& meshBuffer,
                       std::vector<glMeshLoader::VertexLayout> layout,
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
        meshBuffer.primitiveType = m_Entries[0].primitiveType;

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

