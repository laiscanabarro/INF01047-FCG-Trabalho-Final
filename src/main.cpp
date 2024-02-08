// Arquivos "headers" padrï¿½es de C podem ser incluï¿½dos em um
// programa C++, sendo necessï¿½rio somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo sï¿½o especï¿½ficos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criaï¿½ï¿½o de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criaï¿½ï¿½o de janelas do sistema operacional

// Headers da biblioteca GLM: criaï¿½ï¿½o de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <unistd.h> //sleep

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Estrutura que representa um modelo geomï¿½trico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lï¿½ o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, entï¿½o setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretï¿½rio dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaraï¿½ï¿½o de funï¿½ï¿½es utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaraï¿½ï¿½o de vï¿½rias funï¿½ï¿½es utilizadas em main().  Essas estï¿½o definidas
// logo apï¿½s a definiï¿½ï¿½o de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrï¿½i representaï¿½ï¿½o de um ObjModel como malha de triï¿½ngulos para renderizaï¿½ï¿½o
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso nï¿½o existam.
void LoadShadersFromFiles(); // Carrega os shaders de vï¿½rtice e fragmento, criando um programa de GPU
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Funï¿½ï¿½o utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Funï¿½ï¿½o para debugging

// Declaraï¿½ï¿½o de funï¿½ï¿½es auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funï¿½ï¿½es estï¿½o definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
void LoadTextureImage(const char* filename); // Funï¿½ï¿½o que carrega imagens de textura
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funï¿½ï¿½es abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informaï¿½ï¿½es do programa. Definidas apï¿½s main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funï¿½ï¿½es callback para comunicaï¿½ï¿½o com o sistema operacional e interaï¿½ï¿½o do
// usuï¿½rio. Veja mais comentï¿½rios nas definiï¿½ï¿½es das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Collided();

glm::vec3 Bezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

// Definimos uma estrutura que armazenarï¿½ dados necessï¿½rios para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // ï¿½ndice do primeiro vï¿½rtice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Nï¿½mero de ï¿½ndices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterizaï¿½ï¿½o (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estï¿½o armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variï¿½veis globais utilizadas em vï¿½rias funï¿½ï¿½es do cï¿½digo.

// A cena virtual ï¿½ uma lista de objetos nomeados, guardados em um dicionï¿½rio
// (map).  Veja dentro da funï¿½ï¿½o BuildTrianglesAndAddToVirtualScene() como que sï¿½o incluï¿½dos
// objetos dentro da variï¿½vel g_VirtualScene, e veja na funï¿½ï¿½o main() como
// estes sï¿½o acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardarï¿½ as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razï¿½o de proporï¿½ï¿½o da janela (largura/altura). Veja funï¿½ï¿½o FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// ï¿½ngulos de Euler que controlam a rotaï¿½ï¿½o de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuï¿½rio estï¿½ com o botï¿½o esquerdo do mouse
// pressionado no momento atual. Veja funï¿½ï¿½o MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Anï¿½logo para botï¿½o direito do mouse
bool g_MiddleMouseButtonPressed = false; // Anï¿½logo para botï¿½o do meio do mouse

// Variï¿½veis que definem o movimento da camera
bool g_WPressed = false;
bool g_APressed = false;
bool g_SPressed = false;
bool g_DPressed = false;

// Variaveis de UP or DOWN da camera
bool g_SpacePressed = false;
bool g_ShiftPressed = false;

// Variavel de colisão
bool g_Collision = false;

// Variï¿½veis que definem a cï¿½mera em coordenadas esfï¿½ricas, controladas pelo
// usuï¿½rio atravï¿½s do mouse (veja funï¿½ï¿½o CursorPosCallback()). A posiï¿½ï¿½o
// efetiva da cï¿½mera ï¿½ calculada dentro da funï¿½ï¿½o main(), dentro do loop de
// renderizaï¿½ï¿½o.
float g_CameraTheta = 0.0f; // ï¿½ngulo no plano ZX em relaï¿½ï¿½o ao eixo Z
float g_CameraPhi = 0.0f;   // ï¿½ngulo em relaï¿½ï¿½o ao eixo Y
float g_CameraDistance = 1000.0f; // Distï¿½ncia da cï¿½mera para a origem

// Variï¿½veis que controlam rotaï¿½ï¿½o do antebraï¿½o
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variï¿½veis que controlam translaï¿½ï¿½o do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variï¿½vel que controla o tipo de projeï¿½ï¿½o utilizada: perspectiva ou ortogrï¿½fica.
bool g_UsePerspectiveProjection = true;

// Variï¿½vel que controla se o texto informativo serï¿½ mostrado na tela.
bool g_ShowInfoText = true;

// Variï¿½veis que definem um programa de GPU (shaders). Veja funï¿½ï¿½o LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Nï¿½mero de texturas carregadas pela funï¿½ï¿½o LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Coordenadas fixas da camera
float f_r = g_CameraDistance;
float f_y = f_r*sin(g_CameraPhi);
float f_z = f_r*cos(g_CameraPhi)*cos(g_CameraTheta);
float f_x = f_r*cos(g_CameraPhi)*sin(g_CameraTheta);
glm::vec4 camera_movement = glm::vec4(0.0f,0.0f,0.0f,0.0f);

float angle=0.0;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressï¿½o de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versï¿½o 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto ï¿½, utilizaremos somente as
    // funï¿½ï¿½es modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com tï¿½tulo "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Trabalho Final FCG", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a funï¿½ï¿½o de callback que serï¿½ chamada sempre que o usuï¿½rio
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botï¿½es do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverï¿½o renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funï¿½ï¿½es definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a funï¿½ï¿½o de callback que serï¿½ chamada sempre que a janela for
    // redimensionada, por consequï¿½ncia alterando o tamanho do "framebuffer"
    // (regiï¿½o de memï¿½ria onde sï¿½o armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forï¿½amos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informaï¿½ï¿½es sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vï¿½rtices e de fragmentos que serï¿½o utilizados
    // para renderizaï¿½ï¿½o. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Texturas:
    LoadTextureImage("../../data/textures/sun.jpg");               // TextureSun
    LoadTextureImage("../../data/textures/mercury.jpg");           // TextureMercury
    LoadTextureImage("../../data/textures/venus.jpg");             // TextureVenus
    LoadTextureImage("../../data/textures/earth.jpg");             // TextureEarth
    LoadTextureImage("../../data/textures/mars.jpg");              // TextureMars
    LoadTextureImage("../../data/textures/jupiter.jpg");           // TextureJupiter
    LoadTextureImage("../../data/textures/saturn.jpg");            // TextureSaturn
    LoadTextureImage("../../data/textures/uranus.jpg");            // TextureUranus
    LoadTextureImage("../../data/textures/neptune.jpg");           // TextureNeptune
    LoadTextureImage("../../data/textures/skybox.png");            // TextureSkybox
    LoadTextureImage("../../data/textures/silver_metal.jpg");      // TextureSpaceship
    LoadTextureImage("../../data/textures/rock.jpg");              // TextureRock
    LoadTextureImage("../../data/textures/asteroid.jpg");          // TextureAsteroid



    // Construï¿½mos a representaï¿½ï¿½o de objetos geomï¿½tricos atravï¿½s de malhas de triï¿½ngulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel spaceshipmodel("../../data/spaceship.obj");
    ComputeNormals(&spaceshipmodel);
    BuildTrianglesAndAddToVirtualScene(&spaceshipmodel);

    ObjModel rockmodel("../../data/rock.obj");
    ComputeNormals(&rockmodel);
    BuildTrianglesAndAddToVirtualScene(&rockmodel);

    ObjModel asteroidmodel("../../data/asteroid.obj");
    ComputeNormals(&asteroidmodel);
    BuildTrianglesAndAddToVirtualScene(&asteroidmodel);


    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o cï¿½digo para renderizaï¿½ï¿½o de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Propriedades da camera
    float speed = 200.0f; // Velocidade da cï¿½mera
    float prev_time = (float)glfwGetTime();

    // Ficamos em um loop infinito, renderizando, atï¿½ que o usuï¿½rio feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operaï¿½ï¿½es de renderizaï¿½ï¿½o

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor ï¿½
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto ï¿½:
        // Vermelho, Verde, Azul, Alpha (valor de transparï¿½ncia).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminaï¿½ï¿½o.
        //
        //           R     G     B     A
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e tambï¿½m resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vï¿½rtice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posiï¿½ï¿½o da cï¿½mera utilizando coordenadas esfï¿½ricas.  As
        // variï¿½veis g_CameraDistance, g_CameraPhi, e g_CameraTheta sï¿½o
        // controladas pelo mouse do usuï¿½rio. Veja as funï¿½ï¿½es CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varï¿½veis que efetivamente definem a cï¿½mera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f) + camera_movement; // Ponto "c", centro da cï¿½mera
        glm::vec4 camera_view_vector = glm::vec4(-x,-y,-z,0.0f);
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "cï¿½u" (eito Y global)

        // Especificando a cï¿½mera
        glm::vec4 vetor_w = -camera_view_vector / norm(camera_view_vector);
        glm::vec4 up_w_cross = crossproduct(camera_up_vector, vetor_w);
        glm::vec4 vetor_u = up_w_cross / norm(up_w_cross);

        // Computamos a matriz "View" utilizando os parï¿½metros da cï¿½mera para
        // definir o sistema de coordenadas da cï¿½mera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeï¿½ï¿½o.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da cï¿½mera, os planos near e far
        // estï¿½o no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posiï¿½ï¿½o do "near plane"
        float farplane  = -100000.0f; // Posiï¿½ï¿½o do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeï¿½ï¿½o Perspectiva.
            // Para definiï¿½ï¿½o do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeï¿½ï¿½o Ortogrï¿½fica.
            // Para definiï¿½ï¿½o dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEï¿½ï¿½O ORTOGRï¿½FICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortogrï¿½fico, computamos o valor de "t"
            // utilizando a variï¿½vel g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformaï¿½ï¿½o identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vï¿½deo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas sï¿½o
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        #define SOL         0
        #define MERCURIO    1
        #define VENUS       2
        #define TERRA       3
        #define MARTE       4
        #define JUPITER     5
        #define SATURNO     6
        #define URANO       7
        #define NETUNO      8
        #define SKYBOX      9
        #define SPACESHIP   10
        #define ROCK        11
        #define ASTEROID    12

        // Conversï¿½o de tamanhos: 1.0f = 1.000 KM
        float tamanhoSol = 140.0f; // Diï¿½metro: 1.400.000 KM (O sol terï¿½ que ser diminuido 90% de tamanho para caber na projeï¿½ï¿½o)
        float tamanhoMercurio = 4.8f; // Diï¿½metro: 4.879 KM
        float tamanhoVenus = 12.104f; // Diï¿½metro: 12.104 KM
        float tamanhoTerra = 12.742f; // Diï¿½metro: 12.742 KM
        float tamanhoMarte = 6.779f; // Diï¿½metro: 6.779 KM
        float tamanhoJupiter = 139.820f; // Diï¿½metro: 139.820 KM
        float tamanhoSaturno = 116.460f; // Diï¿½metro: 116.460 KM
        float tamanhoUrano = 50.724f;   // Diï¿½metro: 50.724 KM
        float tamanhoNetuno = 49.244f;  // Diï¿½mtro: 49.244 KM
        //float tamanhoLua = 3.474f; // Diï¿½metro: 3.474 km

        // Conversï¿½o de distancias: 1.0f = 1.000.000 KM
        float distanciaMercurioX = 58.0f + tamanhoSol + tamanhoMercurio/2; // Distancia do Sol: 58.000.000 + 1/2 diametro do Sol + 1/2 diametro de Mercurio
        float distanciaVenusX = distanciaMercurioX + 50.0f + tamanhoMercurio/2 + tamanhoVenus/2; // Distancia de Mercurio: 50.000.000 + 1/2 diametro de Mercurio + 1/2 diametro de Venus
        float distanciaTerraX = distanciaVenusX + 41.0f + tamanhoVenus/2 + tamanhoTerra/2; // Distancia da terra
        //float distanciaLuaX = distanciaTerraX + 0.384f + tamanhoTerra/2 + tamanhoLua/2; // Distancia de Lua
        float distanciaMarteX = distanciaTerraX + 78.0f + tamanhoTerra/2 + tamanhoMarte/2; // Distancia de Marte
        float distanciaJupiterX = distanciaMarteX + 550.0f + tamanhoMarte/2 + tamanhoJupiter/2; // Distancia de Jupiter
        float distanciaSaturnoX = distanciaJupiterX + 646.0f + tamanhoJupiter/2 + tamanhoSaturno/2; // Distancia de Saturno
        float distanciaUranoX = distanciaSaturnoX + 1448.0f + tamanhoSaturno/2 + tamanhoUrano/2; // Distancia de Urano
        float distanciaNetunoX = distanciaUranoX + 1627.0f +  tamanhoUrano/2 + tamanhoNetuno/2; // Distancia de Netuno

        float angularSpeed; // Velocidade angular para translaï¿½ï¿½o dos planetas
        float anglePlanet;  // ï¿½ngulo da posiï¿½ï¿½o dos planetas ao longo da suas ï¿½rbitas

        glDepthFunc(GL_ALWAYS); // Desativa Z-buffer para renderizar a skybox

        // Skybox:
        model = Matrix_Translate(1.0f,1.0f,1.0f) // Posiciona o objeto
                * Matrix_Scale(farplane/2,farplane/2,farplane/2); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SKYBOX);
        DrawVirtualObject("the_sphere");

        glDepthFunc(GL_LESS); // Ativa de novo o Z-buffer

        // Sol:
        // Centro da projeï¿½ï¿½o
        model = Matrix_Translate(-5.0f,0.0f,0.0f) // Posiciona o objeto
                * Matrix_Scale(tamanhoSol,tamanhoSol,tamanhoSol); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SOL);
        DrawVirtualObject("the_sphere");

        // Nave:
        r = 20.0f; // Distï¿½ncia da camera
        y = r*sin(g_CameraPhi);
        z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        x = r*cos(g_CameraPhi)*sin(g_CameraTheta);
        camera_view_vector = glm::vec4(-x,-y,-z,0.0f);
        glm::vec4 spaceship_position_relative = glm::vec4(0.0f, -5.0f, 0.25f, 1.0f); // 10 unidades ï¿½ frente da cï¿½mera

        // Posiï¿½ï¿½o absoluta da nave utilizando a posiï¿½ï¿½o da cï¿½mera.
        glm::vec4 spaceship_position = camera_position_c + camera_view_vector + spaceship_position_relative;


        glm::vec3 p0(spaceship_position.x, spaceship_position.y, spaceship_position.z); // Ponto inicial
        glm::vec3 p1(spaceship_position.x, spaceship_position.y + 0.15f, spaceship_position.z); // Primeiro ponto de controle
        glm::vec3 p2(spaceship_position.x, spaceship_position.y + 0.10f, spaceship_position.z); // Segundo ponto de controle
        glm::vec3 p3(spaceship_position.x, spaceship_position.y + 0.05f, spaceship_position.z); // Ponto final

        float t = fmod(glfwGetTime(), 1.5f); // Isso faz com que 't' varie de 0 a 1 repetidamente
        glm::vec3 position = Bezier(t, p0, p1, p2, p3);

        model = Matrix_Translate(position.x, position.y, position.z) // Posiciona o objeto
                * Matrix_Rotate_Z(g_CameraPhi)
                * Matrix_Rotate_X(-g_CameraPhi -0.1f)
                * Matrix_Rotate_Y(g_CameraTheta -0.45f)
                * Matrix_Scale(0.5f,0.5f,0.5f); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SPACESHIP);
        DrawVirtualObject("Cube");


        // Rochas:

        model = Matrix_Translate(350.0f, 0.0f, 0.0f) // Posiciona o objeto
                * Matrix_Scale(5,5,5); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROCK);
        DrawVirtualObject("rock");

        model = Matrix_Translate(250.0f, 100.0f, 0.0f) // Posiciona o objeto
                * Matrix_Scale(10,10,10); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROCK);
        DrawVirtualObject("rock");

        model = Matrix_Translate(375.0f, 0.0f, 200.0f) // Posiciona o objeto
                * Matrix_Scale(5,5,5); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROCK);
        DrawVirtualObject("rock");

        // Asteroide:

        glm::vec3 p4(350.0f, 100.0f, 100.0f); // Ponto inicial
        glm::vec3 p5(450.0f, 300.0f, 200.0f); // Primeiro ponto de controle
        glm::vec3 p6(550.0f, -100.0f, 300.0f); // Segundo ponto de controle
        glm::vec3 p7(650.0f, 100.0f, 400.0f); // Ponto final

        t = fmod(glfwGetTime(), 15.0f); // Isso faz com que 't' varie de 0 a 15 repetidamente
        position = Bezier(t, p4, p5, p6, p7);

        model = Matrix_Translate(position.x, position.y, position.z) // Posiciona o objeto
                * Matrix_Scale(5,5,5); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ASTEROID);
        DrawVirtualObject("Asteroid");


        // Mercurio:
        angularSpeed = 47.87f / distanciaMercurioX;    // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();    // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaMercurioX * cos(anglePlanet), 0.0f, distanciaMercurioX * sin(anglePlanet))
                * Matrix_Rotate_Z(0.6f)
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoMercurio,tamanhoMercurio,tamanhoMercurio); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, MERCURIO);
        DrawVirtualObject("the_sphere");

        // Venus:
        angularSpeed = 35.02f / distanciaVenusX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();    // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaVenusX * cos(anglePlanet), 0.0f, distanciaVenusX * sin(anglePlanet))
                * Matrix_Rotate_Z(0.6f)
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoVenus,tamanhoVenus,tamanhoVenus); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, VENUS);
        DrawVirtualObject("the_sphere");

        // Terra:
        angularSpeed = 29.78 / distanciaTerraX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaTerraX * cos(anglePlanet), 0.0f, distanciaTerraX * sin(anglePlanet))
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoTerra,tamanhoTerra,tamanhoTerra); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TERRA);
        DrawVirtualObject("the_sphere");

        /*
        // Lua:
        angularSpeed = 1.022f;       // Velocidade angular
        float moonDistanceFromEarth = 0.384f;
        float angleMoon = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita

        // Posiï¿½ï¿½o da Lua em relaï¿½ï¿½o ï¿½ Terra, antes de aplicar a translaï¿½ï¿½o da Terra
        glm::mat4 moonRelativePosition = glm::rotate(glm::mat4(1.0f), glm::radians(angleMoon), glm::vec3(0, 1, 0)) // Rotaï¿½ï¿½o ao redor da Terra
                                      * glm::translate(glm::mat4(1.0f), glm::vec3(moonDistanceFromEarth, 0, 0)); // Distï¿½ncia da Terra

        // Aplica a posiï¿½ï¿½o da Terra ï¿½ Lua para que a Lua siga a Terra
        glm::mat4 earthPosition = Matrix_Translate(distanciaTerraX * cos(anglePlanet), 0.0f, distanciaTerraX * sin(anglePlanet));
        glm::mat4 moonPosition = earthPosition * moonRelativePosition;

        // Posiciona o objeto em uma ï¿½rbita circular
        model = moonPosition * Matrix_Scale(tamanhoLua,tamanhoLua,tamanhoLua);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SOL);
        DrawVirtualObject("the_sphere");*/

        // Marte:
        angularSpeed = 24.07 / distanciaMarteX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaMarteX * cos(anglePlanet), 0.0f, distanciaMarteX * sin(anglePlanet))
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoMarte,tamanhoMarte,tamanhoMarte); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, MARTE);
        DrawVirtualObject("the_sphere");

        // Jupiter:
        angularSpeed = 13.07 / distanciaJupiterX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaJupiterX * cos(anglePlanet), 0.0f, distanciaJupiterX * sin(anglePlanet))
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoJupiter,tamanhoJupiter,tamanhoJupiter); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform,JUPITER);
        DrawVirtualObject("the_sphere");

        // Saturn:
        angularSpeed = 9.69 / distanciaSaturnoX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        float saturnX = distanciaSaturnoX * cos(anglePlanet);
        float saturnZ = distanciaSaturnoX * sin(anglePlanet);
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(saturnX, 0.0f, saturnZ)
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoSaturno,tamanhoSaturno,tamanhoSaturno); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform,SATURNO);
        DrawVirtualObject("the_sphere");


        // Urano:
        angularSpeed = 6.81 / distanciaUranoX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaUranoX * cos(anglePlanet), 0.0f, distanciaUranoX * sin(anglePlanet))
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoUrano,tamanhoUrano,tamanhoUrano); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform,URANO);
        DrawVirtualObject("the_sphere");

        // Netuno:
        angularSpeed = 5.43 / distanciaNetunoX;       // Velocidade angular
        anglePlanet = angularSpeed * glfwGetTime();   // ï¿½ngulo da posiï¿½ï¿½o do planeta ao longo da ï¿½rbita
        // Posiciona o objeto em uma ï¿½rbita circular
        model = Matrix_Translate(distanciaNetunoX * cos(anglePlanet), 0.0f, distanciaNetunoX * sin(anglePlanet))
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
                * Matrix_Scale(tamanhoNetuno,tamanhoNetuno,tamanhoNetuno); // Aumenta o objeto
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform,NETUNO);
        DrawVirtualObject("the_sphere");

        // Imprimimos na tela os ï¿½ngulos de Euler que controlam a rotaï¿½ï¿½o do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informaï¿½ï¿½o sobre a matriz de projeï¿½ï¿½o sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informaï¿½ï¿½o sobre o nï¿½mero de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // Atualiza delta de tempo
        float current_time = (float)glfwGetTime();
        float delta_t = current_time - prev_time;
        prev_time = current_time;

        // Movimentaï¿½ï¿½o de objetos a partir das teclas pressionadas
        if (g_WPressed)
        {
            camera_movement += -vetor_w * speed * delta_t;
        }

        if (g_APressed)
        {
            camera_movement += -vetor_u * speed * delta_t;
        }

        if (g_SPressed)
        {
            camera_movement += vetor_w * speed * delta_t;
        }

        if (g_DPressed)
        {
            camera_movement += vetor_u * speed * delta_t;
        }

        if (g_SpacePressed)
        {
            camera_movement += camera_up_vector * speed * delta_t;
        }

        if (g_ShiftPressed)
        {
            camera_movement += -camera_up_vector * speed * delta_t;
        }

        if (g_Collision)
        {
            camera_movement = glm::vec4(0.0f,0.0f,0.0f,0.0f);
        //    spaceship_position = glm::vec4(0.0f, -5.0f, 0.25f, 1.0f);

            g_Collision = false;
        }

        // O framebuffer onde OpenGL executa as operaï¿½ï¿½es de renderizaï¿½ï¿½o nï¿½o
        // ï¿½ o mesmo que estï¿½ sendo mostrado para o usuï¿½rio, caso contrï¿½rio
        // seria possï¿½vel ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuï¿½rio
        // tudo que foi renderizado pelas funï¿½ï¿½es acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interaï¿½ï¿½o do
        // usuï¿½rio (teclado, mouse, ...). Caso positivo, as funï¿½ï¿½es de callback
        // definidas anteriormente usando glfwSet*Callback() serï¿½o chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Funï¿½ï¿½o que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parï¿½metros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Funï¿½ï¿½o que desenha um objeto armazenado em g_VirtualScene. Veja definiï¿½ï¿½o
// dos objetos na funï¿½ï¿½o BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vï¿½rtices apontados pelo VAO criado pela funï¿½ï¿½o BuildTrianglesAndAddToVirtualScene(). Veja
    // comentï¿½rios detalhados dentro da definiï¿½ï¿½o de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variï¿½veis "bbox_min" e "bbox_max" do fragment shader
    // com os parï¿½metros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vï¿½rtices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definiï¿½ï¿½o de
    // g_VirtualScene[""] dentro da funï¿½ï¿½o BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentaï¿½ï¿½o da funï¿½ï¿½o glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operaï¿½ï¿½es posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Funï¿½ï¿½o que carrega os shaders de vï¿½rtices e de fragmentos que serï¿½o
// utilizados para renderizaï¿½ï¿½o. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estï¿½o fixados, sendo que assumimos a existï¿½ncia
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereï¿½o das variï¿½veis definidas dentro do Vertex Shader.
    // Utilizaremos estas variï¿½veis para enviar dados para a placa de vï¿½deo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variï¿½vel da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variï¿½vel da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variï¿½vel da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variï¿½vel "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variï¿½veis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureSun"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureMercury"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureVenus"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureEarth"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureMars"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureJupiter"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureSaturn"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureUranus"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureNeptune"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureSkybox"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureSpaceship"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureRock"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureAsteroid"), 12);
    glUseProgram(0);
}

// Funï¿½ï¿½o que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Funï¿½ï¿½o que remove a matriz atualmente no topo da pilha e armazena a mesma na variï¿½vel M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Funï¿½ï¿½o que computa as normais de um ObjModel, caso elas nï¿½o tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIï¿½NGULOS.
    // Segundo, computamos as normais dos Vï¿½RTICES atravï¿½s do mï¿½todo proposto
    // por Gouraud, onde a normal de cada vï¿½rtice vai ser a mï¿½dia das normais de
    // todas as faces que compartilham este vï¿½rtice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            // PREENCHA AQUI o cï¿½lculo da normal de um triï¿½ngulo cujos vï¿½rtices
            // estï¿½o nos pontos "a", "b", e "c", definidos no sentido anti-horï¿½rio.

            const glm::vec4 cp = crossproduct(b-a, c-a);

            const glm::vec4  n = cp;

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrï¿½i triï¿½ngulos para futura renderizaï¿½ï¿½o a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o cï¿½digo da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel ï¿½
                // comparando se o ï¿½ndice retornado ï¿½ -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro ï¿½ndice
        theobject.num_indices    = last_index - first_index + 1; // Nï¿½mero de indices
        theobject.rendering_mode = GL_TRIANGLES;       // ï¿½ndices correspondem ao tipo de rasterizaï¿½ï¿½o GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora ï¿½ GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operaï¿½ï¿½es posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definiï¿½ï¿½o de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // serï¿½ aplicado nos vï¿½rtices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definiï¿½ï¿½o de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // serï¿½ aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Funï¿½ï¿½o auxilar, utilizada pelas duas funï¿½ï¿½es acima. Carrega cï¿½digo de GPU de
// um arquivo GLSL e faz sua compilaï¿½ï¿½o.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variï¿½vel "filename"
    // e colocamos seu conteï¿½do em memï¿½ria, apontado pela variï¿½vel
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o cï¿½digo do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o cï¿½digo do shader GLSL (em tempo de execuï¿½ï¿½o)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilaï¿½ï¿½o
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memï¿½ria para guardar o log de compilaï¿½ï¿½o.
    // A chamada "new" em C++ ï¿½ equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilaï¿½ï¿½o
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ ï¿½ equivalente ao "free()" do C
    delete [] log;
}

// Esta funï¿½ï¿½o cria um programa de GPU, o qual contï¿½m obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definiï¿½ï¿½o dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memï¿½ria para guardar o log de compilaï¿½ï¿½o.
        // A chamada "new" em C++ ï¿½ equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ ï¿½ equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleï¿½ï¿½o apï¿½s serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definiï¿½ï¿½o da funï¿½ï¿½o que serï¿½ chamada sempre que a janela do sistema
// operacional for redimensionada, por consequï¿½ncia alterando o tamanho do
// "framebuffer" (regiï¿½o de memï¿½ria onde sï¿½o armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda regiï¿½o do framebuffer. A
    // funï¿½ï¿½o "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa ï¿½ a operaï¿½ï¿½o de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos tambï¿½m a razï¿½o que define a proporï¿½ï¿½o da janela (largura /
    // altura), a qual serï¿½ utilizada na definiï¿½ï¿½o das matrizes de projeï¿½ï¿½o,
    // tal que nï¿½o ocorra distorï¿½ï¿½es durante o processo de "Screen Mapping"
    // acima, quando NDC ï¿½ mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float ï¿½ necessï¿½rio pois nï¿½meros inteiros sï¿½o arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variï¿½veis globais que armazenam a ï¿½ltima posiï¿½ï¿½o do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Funï¿½ï¿½o callback chamada sempre que o usuï¿½rio aperta algum dos botï¿½es do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuï¿½rio pressionou o botï¿½o esquerdo do mouse, guardamos a
        // posiï¿½ï¿½o atual do cursor nas variï¿½veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tambï¿½m, setamos a variï¿½vel
        // g_LeftMouseButtonPressed como true, para saber que o usuï¿½rio estï¿½
        // com o botï¿½o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuï¿½rio soltar o botï¿½o esquerdo do mouse, atualizamos a
        // variï¿½vel abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuï¿½rio pressionou o botï¿½o esquerdo do mouse, guardamos a
        // posiï¿½ï¿½o atual do cursor nas variï¿½veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tambï¿½m, setamos a variï¿½vel
        // g_RightMouseButtonPressed como true, para saber que o usuï¿½rio estï¿½
        // com o botï¿½o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuï¿½rio soltar o botï¿½o esquerdo do mouse, atualizamos a
        // variï¿½vel abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuï¿½rio pressionou o botï¿½o esquerdo do mouse, guardamos a
        // posiï¿½ï¿½o atual do cursor nas variï¿½veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tambï¿½m, setamos a variï¿½vel
        // g_MiddleMouseButtonPressed como true, para saber que o usuï¿½rio estï¿½
        // com o botï¿½o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuï¿½rio soltar o botï¿½o esquerdo do mouse, atualizamos a
        // variï¿½vel abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Funï¿½ï¿½o callback chamada sempre que o usuï¿½rio movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botï¿½o esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o ï¿½ltimo
    // instante de tempo, e usamos esta movimentaï¿½ï¿½o para atualizar os
    // parï¿½metros que definem a posiï¿½ï¿½o da cï¿½mera dentro da cena virtual.
    // Assim, temos que o usuï¿½rio consegue controlar a cï¿½mera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parï¿½metros da cï¿½mera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esfï¿½ricas, o ï¿½ngulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variï¿½veis globais para armazenar a posiï¿½ï¿½o atual do
        // cursor como sendo a ï¿½ltima posiï¿½ï¿½o conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parï¿½metros da antebraï¿½o com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variï¿½veis globais para armazenar a posiï¿½ï¿½o atual do
        // cursor como sendo a ï¿½ltima posiï¿½ï¿½o conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parï¿½metros da antebraï¿½o com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variï¿½veis globais para armazenar a posiï¿½ï¿½o atual do
        // cursor como sendo a ï¿½ltima posiï¿½ï¿½o conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Funï¿½ï¿½o callback chamada sempre que o usuï¿½rio movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distï¿½ncia da cï¿½mera para a origem utilizando a
    // movimentaï¿½ï¿½o da "rodinha", simulando um ZOOM.
 //   g_CameraDistance -= 8.0f*yoffset;

    // Uma cï¿½mera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela estï¿½ olhando, pois isto gera problemas de divisï¿½o por zero na
    // definiï¿½ï¿½o do sistema de coordenadas da cï¿½mera. Isto ï¿½, a variï¿½vel abaixo
    // nunca pode ser zero. Versï¿½es anteriores deste cï¿½digo possuï¿½am este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definiï¿½ï¿½o da funï¿½ï¿½o que serï¿½ chamada sempre que o usuï¿½rio pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ===================
    // Nï¿½o modifique este loop! Ele ï¿½ utilizando para correï¿½ï¿½o automatizada dos
    // laboratï¿½rios. Deve ser sempre o primeiro comando desta funï¿½ï¿½o KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ===================

    // Se o usuï¿½rio pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O cï¿½digo abaixo implementa a seguinte lï¿½gica:
    //   Se apertar tecla X       entï¿½o g_AngleX += delta;
    //   Se apertar tecla shift+X entï¿½o g_AngleX -= delta;
    //   Se apertar tecla Y       entï¿½o g_AngleY += delta;
    //   Se apertar tecla shift+Y entï¿½o g_AngleY -= delta;
    //   Se apertar tecla Z       entï¿½o g_AngleZ += delta;
    //   Se apertar tecla shift+Z entï¿½o g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuï¿½rio apertar a tecla L, resetamos os ï¿½ngulos de Euler para zero.
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuï¿½rio apertar a tecla P, utilizamos projeï¿½ï¿½o perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuï¿½rio apertar a tecla O, utilizamos projeï¿½ï¿½o ortogrï¿½fica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuï¿½rio apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuï¿½rio apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        g_Collision = true;
    }

    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla W, entï¿½o atualizamos o estado para pressionada
            g_WPressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla W, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_WPressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla W e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;
    }

    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla A, entï¿½o atualizamos o estado para pressionada
            g_APressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla A, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_APressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla A e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;
    }

    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla S, entï¿½o atualizamos o estado para pressionada
            g_SPressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla S, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_SPressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla S e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;
    }

    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla D, entï¿½o atualizamos o estado para pressionada
            g_DPressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla D, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_DPressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla D e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;

    }

    if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla de espaï¿½o, entï¿½o atualizamos o estado para pressionada
            g_SpacePressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla de espaï¿½o, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_SpacePressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla espaï¿½o e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;
    }

    if (key == GLFW_KEY_LEFT_SHIFT)
    {
        if (action == GLFW_PRESS)
            // Usuï¿½rio apertou a tecla Shift, entï¿½o atualizamos o estado para pressionada
            g_ShiftPressed = true;

        else if (action == GLFW_RELEASE)
            // Usuï¿½rio largou a tecla Shift, entï¿½o atualizamos o estado para Nï¿½O pressionada
            g_ShiftPressed = false;

        else if (action == GLFW_REPEAT)
            // Usuï¿½rio estï¿½ segurando a tecla Shift e o sistema operacional estï¿½
            // disparando eventos de repetiï¿½ï¿½o. Neste caso, nï¿½o precisamos
            // atualizar o estado da tecla, pois antes de um evento REPEAT
            // necessariamente deve ter ocorrido um evento PRESS.
            ;
    }
}

// Definimos o callback para impressï¿½o de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta funï¿½ï¿½o recebe um vï¿½rtice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformaï¿½ï¿½es.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ï¿½ngulos de Euler definidos nas variï¿½veis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeï¿½ï¿½o estï¿½ sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o nï¿½mero de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variï¿½veis estï¿½ticas (static) mantï¿½m seus valores entre chamadas
    // subsequentes da funï¿½ï¿½o!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o nï¿½mero de segundos que passou desde a execuï¿½ï¿½o do programa
    float seconds = (float)glfwGetTime();

    // Nï¿½mero de segundos desde o ï¿½ltimo cï¿½lculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Funï¿½ï¿½o para debugging: imprime no terminal todas informaï¿½ï¿½es de um modelo
// geomï¿½trico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

//FONTE: ChatGPT
glm::vec3 Bezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0; // Primeiro termo
    p += 3 * uu * t * p1; // Segundo termo
    p += 3 * u * tt * p2; // Terceiro termo
    p += ttt * p3; // Quarto termo

    return p;
}