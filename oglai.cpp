// oglai.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#define GLFW_INCLUDE_GLU
//#define GLFW_DLL

#include "engine.h"

using namespace std;

int main()
{
    int res;
    srand(25565);

    //initialize library
    GLFWwindow* window;
    res = EngineInit(&window);
    if (res) return res;
    
    //shaders
    int prog;
    res = LoadShader(&prog, "vs.glsl", "ps.glsl");
    if (res) return res;
    glUseProgram(prog);

    //models

    glViewport(0, 0, 800, 600);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    Model* mdl = LoadOBJ("map.obj");
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    int mdl_color_length = mdl->index_length / 3;;
    float** mdl_colors = new float* [mdl_color_length];

    for (unsigned int i = 0; i < mdl_color_length; ++i)
    {
        mdl_colors[i] = new float[3];
        for (unsigned int c = 0; c < 3; ++c)
            mdl_colors[i][c] = (float)rand() / (float)RAND_MAX;
    }

    unsigned long long InitTime = GetTimeHns();
    unsigned long long CurTime = InitTime;

    while (!glfwWindowShouldClose(window))
    {
        CurTime = GetTimeHns();
        float Time = (float)(CurTime - InitTime) / 10000000.0;

        Matrix World, wRS;
        Matrix Rotation, Translation, Scale;
        Matrix::Scale(&Scale, 1, 1, 1);
        Matrix::RotationY(&Rotation, 0);
        Matrix::Translation(&Translation, 0, -1, 0);
        Matrix::mul(&wRS, &Rotation, &Scale);
        Matrix::mul(&World, &Translation, &wRS);

        Matrix View, Proj, VP;
        Matrix ViewRotate, vRT;
        Matrix::Translation(&View, 0.0, 0.0, 30.0);
        Matrix::RotationX(&ViewRotate, 2.0 * 3.141592 / 8.0);
        Matrix::mul(&vRT, &View, &ViewRotate);
        Matrix::Projection(&Proj, 1.0, 3.0 / 4.0, 1.0, 100);
        Matrix::mul(&VP, &Proj, &vRT);

        float ViewPos[4] = { 0.0, 0.0, 0.0, 1.0 };
        float Viewmid[4], ViewOut[4];
        Matrix::mul(Viewmid, &View, ViewPos);
        Matrix::mul(ViewOut, &ViewRotate, Viewmid);

        glUniform3f(glGetUniformLocation(prog, "campos"), -ViewOut[0], -ViewOut[1], -ViewOut[2]);

        float MatrixData[16];
        for (int n = 0; n < 4; ++n)
            memcpy(MatrixData + 4 * n, World.data[n], 4 * sizeof(float));

        GLint GLWorldPos = glGetUniformLocation(prog, "world");
        glUniformMatrix4fv(GLWorldPos, 1, GL_TRUE, MatrixData);

        for (int n = 0; n < 4; ++n)
            memcpy(MatrixData + 4 * n, VP.data[n], 4 * sizeof(float));

        GLWorldPos = glGetUniformLocation(prog, "viewproj");
        glUniformMatrix4fv(GLWorldPos, 1, GL_TRUE, MatrixData);

        float ClearColor[4] = { 0.2, 0.3, 0.3, 1.0 };
        GLint amb = glGetUniformLocation(prog, "ambient");
        glUniform4f(amb, ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
        glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);

        for (int i = 0; i < mdl->index_length; ++i)
        {
            if (i % 3 == 0)
            {
                float* c = mdl_colors[i / 3];
                glColor3fv(c);
            }

            //get the normal
            if (mdl->render_mode == RENDER_FLAT)
                glNormal3fv(mdl->normals[mdl->normal_indices[i / 3]]);
            else
                glNormal3fv(mdl->normals[mdl->normal_indices[i]]);

            //get the vertex
            glVertex3fv(mdl->vertices[mdl->indices[i]]);
        }

        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    for (int i = 0; i < mdl_color_length; ++i)
        delete[] mdl_colors[i];
    delete[] mdl_colors;

    for (int i = 0; i < mdl->vertex_length; ++i)
        delete[] mdl->vertices[i];
    delete[] mdl->vertices;
    delete[] mdl->indices;
    for (int i = 0; i < mdl->normal_length; ++i)
        delete[] mdl->normals[i];
    delete[] mdl->normals;
    delete[] mdl->normal_indices;
    delete mdl;

    return 0;
}
