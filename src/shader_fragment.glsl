#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpola��o da posi��o global e a normal de cada v�rtice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posi��o do v�rtice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no c�digo C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto est� sendo desenhado no momento
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
uniform int object_id;

// Par�metros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Vari�veis para acesso das imagens de textura
uniform sampler2D TextureSun;
uniform sampler2D TextureMercury;
uniform sampler2D TextureVenus;
uniform sampler2D TextureEarth;
uniform sampler2D TextureMars;
uniform sampler2D TextureJupiter;
uniform sampler2D TextureSaturn;
uniform sampler2D TextureUranus;
uniform sampler2D TextureNeptune;
uniform sampler2D TextureSkybox;
uniform sampler2D TextureSpaceship;
uniform sampler2D TextureRock;

// O valor de sa�da ("out") de um Fragment Shader � a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posi��o da c�mera utilizando a inversa da matriz que define o
    // sistema de coordenadas da c�mera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual � coberto por um ponto que percente � superf�cie de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posi��o no
    // sistema de coordenadas global (World coordinates). Esta posi��o � obtida
    // atrav�s da interpola��o, feita pelo rasterizador, da posi��o de cada
    // v�rtice.
    vec4 p = position_world;

    // habemus lux:
    vec4 lightDir = normalize(origin - position_world);

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada v�rtice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em rela��o ao ponto atual.
    vec4 l = lightDir;

    // Vetor que define o sentido da c�mera em rela��o ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflex�o especular ideal.
    vec4 r = -l + 2*n*dot(n,l);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;
    vec3 Kd0 = vec3(0.2,0.2,0.2);

    // Par�metros que definem as propriedades espectrais da superf�cie
    vec3 Kd; // Reflet�ncia difusa
    vec3 Ks; // Reflet�ncia especular
    vec3 Ka; // Reflet�ncia ambiente
    float q; // Expoente especular para o modelo de ilumina��o de Phong

    U = texcoords.x;
    V = texcoords.y;

    vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

    if ( object_id == SOL || object_id == SKYBOX )
    {
        Kd = vec3(1.0,1.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(1.0,1.0,0.0);
        q = 1.0;

        if (object_id == SOL)
        {Kd0 = texture(TextureSun, vec2(U,V)).rgb * 1.2;}
        else if (object_id == SKYBOX)
        {Kd0 = texture(TextureSkybox, vec2(U,V)).rgb * 1.2;}

    }
    else {

        // Valores para todos os planetas:

        Kd = vec3(0.8,0.8,0.8);
        Ks = vec3(0.2,0.2,0.2);
        Ka = vec3(0.0,0.0,0.0);
        q = 32.0;

        if (object_id == MERCURIO)
            {Kd0 = texture(TextureMercury, vec2(U,V)).rgb;}
        else if (object_id == VENUS)
            {Kd0 = texture(TextureVenus, vec2(U,V)).rgb;}
        else if (object_id == TERRA)
            {Kd0 = texture(TextureEarth, vec2(U,V)).rgb;}
        else if (object_id == MARTE)
            {Kd0 = texture(TextureMars, vec2(U,V)).rgb;}
        else if (object_id == JUPITER)
            {Kd0 = texture(TextureJupiter, vec2(U,V)).rgb;}
        else if (object_id == SATURNO)
            {Kd0 = texture(TextureSaturn, vec2(U,V)).rgb;}
        else if (object_id == URANO)
            {Kd0 = texture(TextureUranus, vec2(U,V)).rgb;}
        else if (object_id == NETUNO)
            {Kd0 = texture(TextureNeptune, vec2(U,V)).rgb;}
        else if (object_id == SPACESHIP)
            {Kd0 = texture(TextureSpaceship, vec2(U,V)).rgb;}
        else if (object_id == ROCK)
            {Kd0 = texture(TextureRock, vec2(U,V)).rgb;}
        else // Objeto desconhecido = preto
        {
            Kd = vec3(0.0,0.0,0.0);
            Ks = vec3(0.0,0.0,0.0);
            Ka = vec3(0.0,0.0,0.0);
            q = 1.0;
        }
    }

    // Espectro da fonte de ilumina��o
    vec3 I = vec3(1.0,1.0,1.0); // PREENCH AQUI o espectro da fonte de luz

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.1,0.1,0.1); // PREENCHA AQUI o espectro da luz ambiente

    // Termo difuso utilizando a lei dos cossenos de Lambert
    float lambert_diffuse_term = max(0,dot(n,l));

    // Termo ambiente
    vec3 ambient_term = vec3(0.1,0.1,0.1); // PREENCHA AQUI o termo ambiente

    // Termo especular utilizando o modelo de ilumina��o de Phong
    float phong_specular_term  = pow(max(0,dot(r,v)),q); // PREENCH AQUI o termo especular de Phong

    // NOTE: Se voc� quiser fazer o rendering de objetos transparentes, �
    // necess�rio:
    // 1) Habilitar a opera��o de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no c�digo C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *ap�s* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas dist�ncias para a c�mera (desenhando primeiro objetos
    //    transparentes que est�o mais longe da c�mera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final do fragmento calculada com uma combina��o dos termos difuso,
    // especular, e ambiente. Veja slide 129 do documento Aula_17_e_18_Modelos_de_Iluminacao.pdf.

    if ( object_id == SOL || object_id == SKYBOX ) // Sol e skybox n�o possuem ilumina��o externa por si proprio emitir luz, ent�o precisamos compensar por isso
        color.rgb = Kd0;
    else
        color.rgb = Kd0 * (lambert_diffuse_term * I * Kd) + (ambient_term * Ia * Ka) + (phong_specular_term * I * Ks);

    // Cor final com corre��o gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

