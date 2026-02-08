#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "model.hpp"
#include "camera.hpp"
#include "../Header/Util.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

struct Character {
    unsigned int TextureID;
    int SizeX, SizeY;
    int BearingX, BearingY;
    unsigned int Advance;
};

std::map<char, Character> Characters;

// Main fajl funkcija sa osnovnim komponentama OpenGL programa

// Projekat je dozvoljeno pisati počevši od ovog kostura
// Toplo se preporučuje razdvajanje koda po fajlovima (i eventualno potfolderima) !!!
// Srećan rad!

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

unsigned int VBO;

void formVAOTexture(float* vertices, size_t size, unsigned int& vao) {
    // formiranje VAO-ova je izdvojeno u posebnu funkciju radi čitljivijeg koda u main funkciji

    // Podsetnik za atribute:
    /*
        Jedan VAO se vezuje za jedan deo celokupnog skupa verteksa na sceni.
        Na primer, dobra praksa je da se jedan VAO vezuje za jedan VBO koji se vezuje za jedan objekat, odnosno niz temena koja opisuju objekat.

        VAO je pomoćna struktura koja opisuje kako se podaci u nizu objekta interpretiraju.
        U render-petlji, za crtanje određenog objekta, naredbom glBindVertexArray(nekiVAO) se određuje koji se objekat crta.

        Potrebno je definisati koje atribute svako teme u nizu sadrži, npr. pozicija na lokaciji 0 i boja na lokaciji 1.

        Ova konfiguracija je specifična za naš primer na vežbama i može se menjati za različite potrebe u projektu.


        Atribut se opisuje metodom glVertexAttribPointer(). Argumenti su redom:
            index - identifikacioni kod atributa, u verteks šejderu je povezan preko svojstva location (location = 0 u šejderu -> indeks tog atributa treba staviti isto 0 u ovoj naredbi)
            size - broj vrednosti unutar atributa (npr. za poziciju su x i y, odnosno 2 vrednosti; za boju r, g i b, odnosno 3 vrednosti)
            type - tip vrednosti
            normalized - da li je potrebno mapirati na odgovarajući opseg (mi poziciju već inicijalizujemo na opseg (-1, 1), a boju (0, 1), tako da nam nije potrebno)
            stride - koliko elemenata u nizu treba preskočiti da bi se došlo od datog atributa u jednom verteksu do istog tog atributa u sledećem verteksu
            pointer - koliko elemenata u nizu treba preskočiti od početka niza da bi se došlo do prvog pojavljivanja datog atributa
    */
    // Četvorougao

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atribut 0 (pozicija):
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (tekstura):
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void formVAOPositionOnly(float* vertices, size_t size, unsigned int& vao) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atribut 0 (pozicija):
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void formVAO3D(float* vertices, size_t size, unsigned int& vao, unsigned int& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

float cubeVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

unsigned signatureTex;

void drawSignature(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, signatureTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

unsigned int textVAO, textVBO;
unsigned int textShader;

void initFreeType(const char* fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "ERROR::FREETYPE: Could not init FreeType Library\n");
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        fprintf(stderr, "ERROR::FREETYPE: Failed to load font\n");
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            fprintf(stderr, "ERROR::FREETYPE: Failed to load Glyph\n");
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            (int)face->glyph->bitmap.width,
            (int)face->glyph->bitmap.rows,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,
            (unsigned int)face->glyph->advance.x
        };
        Characters[c] = character;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderText(unsigned int shader, std::string text, float x, float y, float scale, float r, float g, float b, float screenWidth, float screenHeight) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), r, g, b);
    glUniform1i(glGetUniformLocation(shader, "text"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    float startX = x;
    for (char c : text) {
        Character ch = Characters[c];

        float xpos = startX + ch.BearingX * scale / screenWidth * 2.0f;
        float ypos = y - (ch.SizeY - ch.BearingY) * scale / screenHeight * 2.0f;

        float w = ch.SizeX * scale / screenWidth * 2.0f;
        float h = ch.SizeY * scale / screenHeight * 2.0f;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        startX += (ch.Advance >> 6) * scale / screenWidth * 2.0f;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int createColorTexture(float r, float g, float b, float a = 1.0f) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char data[] = {
        static_cast<unsigned char>(r * 255),
        static_cast<unsigned char>(g * 255),
        static_cast<unsigned char>(b * 255),
        static_cast<unsigned char>(a * 255)
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureID;
}

int main()
{
    srand(time(NULL));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Bus 3D Simulator", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    preprocessTexture(signatureTex, "../Resources/signature.png");

    unsigned int simpleTextureShader = createShader("../Shaders/simple_texture.vert", "../Shaders/simple_texture.frag");
    glUseProgram(simpleTextureShader);
    glUniform1i(glGetUniformLocation(simpleTextureShader, "signatureTex"), 0);

    float verticesSignature[] = {
        0.5f, -0.7f, 0.0f, 1.0f, // gornje levo teme
        0.5f, -1.0f, 0.0f, 0.0f, // donje levo teme
        1.0f, -1.0f, 1.0f, 0.0f, // donje desno teme
        1.0f, -0.7f, 1.0f, 1.0f, // gornje desno teme
   };

    unsigned int VAOsignature;
    formVAOTexture(verticesSignature, sizeof(verticesSignature), VAOsignature);

    unsigned int cubeVAO, cubeVBO;
    formVAO3D(cubeVertices, sizeof(cubeVertices), cubeVAO, cubeVBO);

    unsigned int busColorTex = createColorTexture(0.3f, 0.3f, 0.3f); // Grey-ish bus
    unsigned int windshieldTex = createColorTexture(0.1f, 0.1f, 0.1f, 0.5f); // Dark transparent
    unsigned int controlPanelTex = createColorTexture(1.0f, 0.0f, 0.0f); // Red
    unsigned int doorTex = createColorTexture(0.2f, 0.6f, 0.3f); // Dark doors

    Shader unifiedShader("../Shaders/basic.vert", "../Shaders/basic.frag");

    Model tree("../Resources/tree/Tree.obj");

    camera.Position = glm::vec3(-1.0f, 0.5f, -4.0f);

    glClearColor(0.3f, 0.4f, 0.8f, 1.0f); // kind of sky color
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        unifiedShader.use();
        unifiedShader.setVec3("uLightPos", 0, 5, 5);
        unifiedShader.setVec3("uViewPos", camera.Position);
        unifiedShader.setVec3("uLightColor", 1, 1, 1);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        unifiedShader.setMat4("uP", projection);
        glm::mat4 view = camera.GetViewMatrix();
        unifiedShader.setMat4("uV", view);

        // Render Tree (outside)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, -15.0f));
        unifiedShader.setMat4("uM", model);
        tree.Draw(unifiedShader);

        // Render Bus Body (main shell)
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, busColorTex);

        // Floor
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 0.1f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Left wall
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Right wall (partial, for door)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.5f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 8.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Roof
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 0.1f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Back wall
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 3.0f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Front lower part (where control panel is)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.25f, -5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 1.5f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Door
        glBindTexture(GL_TEXTURE_2D, doorTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.5f, -4.0f));
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 2.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Control Panel (Red rectangle)
        glBindTexture(GL_TEXTURE_2D, controlPanelTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.8f)); // Slightly closer to driver
        model = glm::scale(model, glm::vec3(1.0f, 0.6f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Windshield (Transparent)
        glEnable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, windshieldTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.25f, -5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 1.5f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        drawSignature(simpleTextureShader, VAOsignature);

        glfwSwapBuffers(window);
        glfwPollEvents();

        while (glfwGetTime() - currentFrame < 1 / 75.0) {}
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}