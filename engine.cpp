
#include "engine.h"

using namespace std;

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

string LoadFile(const char* src)
{
    ifstream in(src);
    string out;

    in.seekg(0, in.end);
    out.reserve(in.tellg());
    in.seekg(0, in.beg);

    out.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    return out;
}

int BuildShader(int *shader, const char* src, GLenum type)
{
    GLint success;
    string source = LoadFile(src);
    const GLchar* shader_cstr = source.c_str();
    int shader_size = source.size();

    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &shader_cstr, &shader_size);
    glCompileShader(*shader);

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(*shader, sizeof(InfoLog), NULL, InfoLog);
        cout << InfoLog;
        glfwTerminate();
        return -1;
    }

    return 0;
}

int LoadShader(int* prog, const char* vs, const char* ps)
{
    int shader = glCreateProgram();
    int vshade, pshade;

    int res;
    res = BuildShader(&vshade, vs, GL_VERTEX_SHADER);
    if (res) return res;

    glAttachShader(shader, vshade);

    res = BuildShader(&pshade, ps, GL_FRAGMENT_SHADER);
    if (res) return res;

    glAttachShader(shader, pshade);
    glLinkProgram(shader);

    glGetProgramiv(shader, GL_LINK_STATUS, &res);
    if (!res)
    {
        GLchar InfoLog[1024];
        glGetProgramInfoLog(shader, sizeof(InfoLog), NULL, InfoLog);
        cout << InfoLog;
        glfwTerminate();
        return -1;
    }

    glDeleteShader(vshade);
    glDeleteShader(pshade);

    *prog = shader;
    return 0;
}

int EngineInit(GLFWwindow **window)
{
    if (!glfwInit())
    {
        cout << "GLFW Failed to init" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* wnd = glfwCreateWindow(800, 600, "Hello world", NULL, NULL);
    if (!window)
    {
        cout << "GLFW Window failed to create" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(wnd);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    cout << "Opengl " << glGetString(GL_VERSION) << " GLSL" << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    *window = wnd;

    return 0;
}
