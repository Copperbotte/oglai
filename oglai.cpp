// oglai.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#define GLFW_INCLUDE_GLU
//#define GLFW_DLL
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

using namespace std;

namespace
{
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
}

const char VertexShader[] =
"#version 430 \n"
"in vec4 gl_Vertex; \n"
"in vec4 gl_Normal; \n"
"in vec4 gl_Color; \n"
"\n"
"uniform mat4 world;\n"
"uniform mat4 viewproj;\n"
"uniform vec4 ambient;\n"
"uniform vec3 campos;\n"
"\n"
"varying vec4 color;\n"
"varying vec3 normal;\n"
"varying vec4 wpos;\n"
"\n"
"void main() \n"
"{ \n"
"   wpos = world * gl_Vertex;\n"
"   gl_Position = viewproj * wpos;\n"
"   normal = normalize((world * vec4(gl_Normal.xyz, 0.0)).xyz);\n"
"   color = gl_Color; \n"
"} \n"
"";

const char PixelShader[] =
"#version 330 \n"
"out vec4 gl_FragColor;\n"
"in vec4 color; \n"
"in vec3 normal; \n"
"in vec4 wpos;\n"
"uniform vec4 ambient;\n"
"uniform vec3 campos;\n"
"\n"
"float s2l(float s)\n"
"{\n"
"    if (s <= 0.04045)\n"
"        return s / 12.92;\n"
"    return pow((s + 0.055) / 1.055, 2.4);\n"
"}\n"
"\n"
"float l2s(float l)\n"
"{\n"
"    if (l <= 0.0031308)\n"
"        return l * 12.92;\n"
"    return pow(l, 1.0 / 2.4) * 1.055 - 0.055;\n"
"}\n"
"\n"
"vec3 s2lvec(vec3 s)\n"
"{\n"
"    return vec3(s2l(s.x), s2l(s.y), s2l(s.z));\n"
"}\n"
"\n"
"vec3 l2svec(vec3 l)\n"
"{\n"
"    return vec3(l2s(l.x), l2s(l.y), l2s(l.z));\n"
"}\n"
"void main()\n"
"{\n"
"   vec3 albedo = s2lvec(color.xyz);\n"
"   vec3 light = vec3(0,1,0);\n"
"   vec3 view = normalize(campos - wpos.xyz);\n"
"   float power = 300;\n"
"   float reflectivity = 0.2;\n"
"   \n"
"   vec3 refl = reflect(light, normal);\n"
"   float diffuse = clamp(dot(normal, light), 0.0, 1.0);\n"
"   float specular = clamp(dot(view, refl), 0.0, 1.0);\n"
"   specular = pow(specular, power) * (power + 2.0) / 2.0;\n"
"   \n"
"   vec3 output = s2lvec(ambient.xyz) * ((1.0 - reflectivity) * albedo + reflectivity);\n"
"   output += (1.0 - reflectivity) * diffuse * albedo + reflectivity * specular;\n"
"   gl_FragColor = vec4(l2svec(output), 1.0);\n"
"}\n"
"";

int main()
{
    srand(25565);

    //initialize library
    if (!glfwInit())
    {
        cout << "GLFW Failed to init" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello world", NULL, NULL);
    if (!window)
    {
        cout << "GLFW Window failed to create" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    cout << "Opengl " << glGetString(GL_VERSION) << " GLSL" << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    //shaders
    GLint success;
    const GLchar* v[1], * p[1];
    v[0] = VertexShader;
    p[0] = PixelShader;

    int prog = glCreateProgram();

    int vshade = glCreateShader(GL_VERTEX_SHADER);
    int vshadesize = sizeof(VertexShader);
    glShaderSource(vshade, 1, v, &vshadesize);
    glCompileShader(vshade);

    glGetShaderiv(vshade, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(vshade, sizeof(InfoLog), NULL, InfoLog);
        cout << InfoLog;
        glfwTerminate();
        return -1;
    }

    int pshade = glCreateShader(GL_FRAGMENT_SHADER);
    int pshadesize = sizeof(PixelShader);
    glShaderSource(pshade, 1, p, &pshadesize);
    glCompileShader(pshade);

    glGetShaderiv(pshade, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(pshade, sizeof(InfoLog), NULL, InfoLog);
        cout << InfoLog;
        glfwTerminate();
        return -1;
    }

    glAttachShader(prog, vshade);
    glAttachShader(prog, pshade);
    glLinkProgram(prog);

    glUseProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar InfoLog[1024];
        glGetProgramInfoLog(prog, sizeof(InfoLog), NULL, InfoLog);
        cout << InfoLog;
        glfwTerminate();
        return -1;
    }



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

namespace
{
    unsigned long long GetTimeHns() // Time in 100 nanoseconds
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        ULARGE_INTEGER li;
        li.LowPart = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        return li.QuadPart;
    }

    Model* LoadOBJ(const char* path)
    {
        unsigned int VertexCount = 0;
        unsigned int IndexCount = 0;
        unsigned int NormalCount = 0;

        MODEL_RENDER_MODE RM = RENDER_FLAT;

        ifstream in(path);

        string line, word;
        string F = "";
        while (getline(in, line))
        {
            F += line + '\n';

            //get the number of faces and indices
            stringstream streamline(line);
            getline(streamline, word, ' ');

            if (!word.compare("#"))
                continue;

            if (!word.compare("v"))
            {
                ++VertexCount;
                continue;
            }

            if (!word.compare("vn"))
            {
                ++NormalCount;
                continue;
            }

            if (!word.compare("f"))
            {
                IndexCount += 3;
                continue;
            }

            if (!word.compare("s"))
            {
                //get 1 or off
                getline(streamline, word, ' ');
                if (!word.compare("1"))
                    RM = RENDER_SMOOTH;
                if (!word.compare("off"))
                    RM = RENDER_FLAT;
            }
        }

        in.close();

        Model* mdl = new Model;
        mdl->render_mode = RM;
        mdl->index_length = IndexCount;
        mdl->vertex_length = VertexCount;
        mdl->normal_length = NormalCount;
        mdl->vertices = new float* [VertexCount];
        mdl->indices = new unsigned int[IndexCount];
        mdl->normals = new float* [NormalCount];
        if (mdl->render_mode == RENDER_FLAT)
            mdl->normal_indices = new unsigned int[IndexCount / 3];
        else
            mdl->normal_indices = new unsigned int[IndexCount];

        unsigned int LoadedVertices = 0;
        unsigned int LoadedIndices = 0;
        unsigned int LoadedNormals = 0;

        stringstream data(F);
        while (getline(data, line))
        {

            //load faces and indices
            stringstream streamline(line);
            getline(streamline, word, ' ');

            if (!word.compare("v"))
            {
                mdl->vertices[LoadedVertices] = new float[3];

                for (int i = 0; i < 3; ++i)
                {
                    getline(streamline, word, ' ');
                    mdl->vertices[LoadedVertices][i] = stof(word);
                }

                ++LoadedVertices;
                continue;
            }

            if (!word.compare("vn"))
            {
                mdl->normals[LoadedNormals] = new float[3];

                for (int i = 0; i < 3; ++i)
                {
                    getline(streamline, word, ' ');
                    mdl->normals[LoadedNormals][i] = stof(word);
                }

                ++LoadedNormals;
                continue;
            }

            if (!word.compare("f"))
            {
                //get an index triplet
                for (int i = 0; i < 3; ++i)
                {
                    //get a face's indices
                    getline(streamline, word, ' ');
                    stringstream facestream(word);
                    string face;

                    //get vertex index
                    getline(facestream, face, '/');
                    mdl->indices[LoadedIndices + i] = stoi(face) - 1;

                    //get texture index
                    getline(facestream, face, '/');

                    //get normal index
                    getline(facestream, face, '/');
                    float normalindex = stoi(face) - 1;
                    normalindex = normalindex;
                    if (mdl->render_mode == RENDER_FLAT)
                        mdl->normal_indices[LoadedIndices / 3] = stoi(face) - 1;
                    else
                        mdl->normal_indices[LoadedIndices + i] = stoi(face) - 1;
                }

                LoadedIndices += 3;
                continue;
            }
        }

        return mdl;
    }

    Matrix* Matrix::Identity(Matrix* M)
    {
        //clear result
        for (int i = 0; i < 4; ++i)
            for (int n = 0; n < 4; ++n)
                M->data[i][n] = 0.0;

        //set trace
        for (int i = 0; i < 4; ++i)
            M->data[i][i] = 1.0;

        return M;
    }

    Matrix* Matrix::Translation(Matrix* M, float x, float y, float z)
    {
        Identity(M);

        M->data[0][3] = x;
        M->data[1][3] = y;
        M->data[2][3] = z;

        return M;
    }

    Matrix* Matrix::Scale(Matrix* M, float sx, float sy, float sz)
    {
        Identity(M);

        M->data[0][0] = sx;
        M->data[1][1] = sy;
        M->data[2][2] = sz;

        return M;
    }

    Matrix* Matrix::Projection(Matrix* M, float Width, float Height, float Near, float Far)
    {
        Identity(M);

        M->data[0][0] = 2 * Near / Width;
        M->data[1][1] = 2 * Near / Height;
        M->data[2][2] = (Far + Near) / (Far - Near);
        M->data[3][2] = 1.0;
        M->data[2][3] = -2.0 * Far * Near / (Far - Near);

        return M;
    }

    Matrix* Matrix::RotationX(Matrix* M, float theta)
    {
        Identity(M);

        float C = cos(theta);
        float S = sin(theta);

        M->data[1][1] = C;
        M->data[1][2] = S;
        M->data[2][1] = -S;
        M->data[2][2] = C;

        return M;
    }

    Matrix* Matrix::RotationY(Matrix* M, float theta)
    {
        Identity(M);

        float C = cos(theta);
        float S = sin(theta);

        M->data[0][0] = C;
        M->data[0][2] = S;
        M->data[2][0] = -S;
        M->data[2][2] = C;

        return M;
    }

    Matrix* Matrix::RotationZ(Matrix* M, float theta)
    {
        Identity(M);

        float C = cos(theta);
        float S = sin(theta);

        M->data[0][0] = C;
        M->data[0][1] = S;
        M->data[1][0] = -S;
        M->data[1][1] = C;

        return M;
    }

    float* Matrix::mul(float* V2, Matrix* M, float* V1)
    {
        //clear result
        for (int i = 0; i < 4; ++i)
            V2[i] = 0.0;

        //matrix mul
        for (int i = 0; i < 4; ++i)
            for (int n = 0; n < 4; ++n)
                V2[i] += M->data[i][n] * V1[n];

        return V2;
    }

    Matrix* Matrix::mul(Matrix* MO, Matrix* ML, Matrix* MR)
    {
        //clear result
        for (int i = 0; i < 4; ++i)
            for (int n = 0; n < 4; ++n)
                MO->data[i][n] = 0.0;

        //matrix mul
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                for (int i = 0; i < 4; ++i)
                    MO->data[r][c] += ML->data[r][i] * MR->data[i][c];

        return MO;
    }
}