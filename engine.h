
#ifndef __ENGINE_H
#define __ENGINE_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <Windows.h>

#include "glad/glad.h"
#include "glfw/glfw3.h"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glfw3.lib")
#pragma comment (lib, "glfw3dll.lib")

enum MODEL_RENDER_MODE
{
    RENDER_FLAT,
    RENDER_SMOOTH
};

struct Model
{
    int vertex_length, index_length, normal_length;
    float** vertices;
    unsigned int* indices;
    float** normals;
    unsigned int* normal_indices;
    MODEL_RENDER_MODE render_mode;
    //in flat mode, normal_indices length is index_length / 3
    //in smooth mode, normal_indices length is index_length
};

class Matrix
{
public:
    int cols, rows;
    float data[4][4];

    static Matrix* Identity(Matrix* M);
    static Matrix* Translation(Matrix* M, float x, float y, float z);
    static Matrix* Scale(Matrix* M, float sx, float sy, float sz);
    static Matrix* Projection(Matrix* M, float Width, float Height, float Near, float Far);
    static Matrix* RotationX(Matrix* M, float theta);
    static Matrix* RotationY(Matrix* M, float theta);
    static Matrix* RotationZ(Matrix* M, float theta);
    static float* mul(float* V2, Matrix* M, float* V1);
    static Matrix* mul(Matrix* MO, Matrix* ML, Matrix* MR);

};

unsigned long long GetTimeHns();

Model* LoadOBJ(const char* path);

std::string LoadFile(const char* src);
int LoadShader(int *prog, const char* vs, const char* ps);

int EngineInit(GLFWwindow** window);

#endif
