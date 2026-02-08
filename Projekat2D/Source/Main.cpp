#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>

#include "../Header/Util.h"

#define NUM_SLICES 40

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

unsigned signatureTex;
unsigned busTex;
unsigned doorsOpenTex;
unsigned doorsClosedTex;
unsigned controlTex;

unsigned int VBO;

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

void drawSignature(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, signatureTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

struct Station {
    float x, y;
    int number;
};

Station stations[10];

void initializeStations() {
    stations[0].x = -0.4f; stations[0].y = 0.6f; stations[0].number = 0;
    stations[1].x = 0.15f; stations[1].y = 0.55f; stations[1].number = 1;
    stations[2].x = 0.5f; stations[2].y = 0.65f; stations[2].number = 2;
    stations[3].x = 0.55f; stations[3].y = 0.3f; stations[3].number = 3;
    stations[4].x = 0.65f; stations[4].y = -0.35f; stations[4].number = 4;
    stations[5].x = 0.1f; stations[5].y = -0.5f; stations[5].number = 5;
    stations[6].x = -0.15f; stations[6].y = -0.65f; stations[6].number = 6;
    stations[7].x = -0.4f; stations[7].y = -0.1f; stations[7].number = 7;
    stations[8].x = -0.75f; stations[8].y = 0.15f; stations[8].number = 8;
    stations[9].x = -0.45f; stations[9].y = 0.25f; stations[9].number = 9;
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

void drawStations(unsigned int shader, unsigned int vao, const GLFWvidmode* mode) {
    glUseProgram(shader);

    GLint loc = glGetUniformLocation(shader, "uOffset");

    for (int i = 0; i < 10; i++) {
        glUniform2f(loc, stations[i].x, stations[i].y);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_SLICES + 2);
    }

    for (int i = 0; i < 10; i++) {
        char numStr[2];
        snprintf(numStr, sizeof(numStr), "%d", i);
        Character ch = Characters[numStr[0]];
        float scale = 1.5f;
        float textWidth = ch.SizeX * scale / mode->width * 2.0f;
        float textHeight = ch.SizeY * scale / mode->height * 2.0f;
        float offsetX = textWidth * 0.5f + ch.BearingX * scale / mode->width * 2.0f;
        float offsetY = textHeight * 0.5f - (ch.SizeY - ch.BearingY) * scale / mode->height * 2.0f;
        renderText(textShader, numStr, stations[i].x - offsetX, stations[i].y - offsetY, scale, 0.9f, 0.9f, 0.9f, mode->width, mode->height);
    }
}

bool busStopped = false;
int numberOfPassengers = 0;
bool isControlInside = false;
int numberOfTickets = 0;

void drawBus(unsigned int shader, unsigned int vao) {
    static int currentStation = 0;
    static int nextStation = 1;
    static float distanceTraveled = 0.0f;
    static double stopStartTime = 0.0;

    const float speed = 0.15f;
    const int segments = 100;
    const double stopDuration = 10.0;

    if (distanceTraveled == 0.0f) {
        if (stopStartTime == 0.0) {
            if (isControlInside) {
                numberOfTickets += rand() % numberOfPassengers;
                numberOfPassengers--;
                isControlInside = false;
            }

            stopStartTime = glfwGetTime();
        }
        double elapsed = glfwGetTime() - stopStartTime;
        if (elapsed < stopDuration) {
            busStopped = true;
        } else {
            stopStartTime = 0.0;
            busStopped = false;
        }
    } else {
        busStopped = false;
    }

    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, busTex);

    float cx = (stations[currentStation].x + stations[nextStation].x) / 2.0f;
    float cy = (stations[currentStation].y + stations[nextStation].y) / 2.0f;

    float dx = stations[nextStation].x - stations[currentStation].x;
    float dy = stations[nextStation].y - stations[currentStation].y;
    float offset = 0.35f;
    float length = sqrt(dx*dx + dy*dy);
    if (length > 0.0f) {
        cx += -dy / length * offset;
        cy += dx / length * offset;
    }

    float totalLength = 0.0f;
    float lastX = stations[currentStation].x;
    float lastY = stations[currentStation].y;
    for (int i = 1; i <= segments; i++) {
        float tSeg = (float)i / segments;
        float u = 1.0f - tSeg;
        float x = u*u*stations[currentStation].x + 2*u*tSeg*cx + tSeg*tSeg*stations[nextStation].x;
        float y = u*u*stations[currentStation].y + 2*u*tSeg*cy + tSeg*tSeg*stations[nextStation].y;
        totalLength += sqrt((x-lastX)*(x-lastX) + (y-lastY)*(y-lastY));
        lastX = x;
        lastY = y;
    }

    if (!busStopped) {
        distanceTraveled += speed / 75.0f;
    }

    if (distanceTraveled >= totalLength) {
        if (isControlInside) {
            numberOfTickets += rand() % numberOfPassengers;
            numberOfPassengers--;
            isControlInside = false;
        }

        distanceTraveled = 0.0f;
        currentStation = nextStation;
        nextStation = (nextStation + 1) % 10;
        busStopped = true;
        stopStartTime = glfwGetTime();
    }

    float traveled = 0.0f;
    float busX = stations[currentStation].x;
    float busY = stations[currentStation].y;
    lastX = stations[currentStation].x;
    lastY = stations[currentStation].y;
    for (int i = 1; i <= segments; i++) {
        float tSeg = (float)i / segments;
        float u = 1.0f - tSeg;
        float x = u*u*stations[currentStation].x + 2*u*tSeg*cx + tSeg*tSeg*stations[nextStation].x;
        float y = u*u*stations[currentStation].y + 2*u*tSeg*cy + tSeg*tSeg*stations[nextStation].y;
        float d = sqrt((x-lastX)*(x-lastX) + (y-lastY)*(y-lastY));
        if (traveled + d >= distanceTraveled) {
            float remain = distanceTraveled - traveled;
            float ratio = remain / d;
            busX = lastX + (x-lastX) * ratio;
            busY = lastY + (y-lastY) * ratio;
            break;
        }
        traveled += d;
        lastX = x;
        lastY = y;
    }

    GLint loc = glGetUniformLocation(shader, "uOffset");
    glUniform2f(loc, busX, busY);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawBusPaths(unsigned int shader) {
    glUseProgram(shader);

    const int segments = 50;
    for (int i = 0; i < 10; i++) {
        int next = (i + 1) % 10;

        float cx = (stations[i].x + stations[next].x) / 2.0f;
        float cy = (stations[i].y + stations[next].y) / 2.0f;

        float dx = stations[next].x - stations[i].x;
        float dy = stations[next].y - stations[i].y;
        float offset = 0.35f;
        float length = sqrt(dx*dx + dy*dy);
        if (length > 0.0f) {
            cx += -dy / length * offset;
            cy += dx / length * offset;
        }

        std::vector<float> vertices;
        for (int j = 0; j <= segments; j++) {
            float t = (float)j / segments;
            float u = 1.0f - t;
            float x = u*u*stations[i].x + 2*u*t*cx + t*t*stations[next].x;
            float y = u*u*stations[i].y + 2*u*t*cy + t*t*stations[next].y;
            vertices.push_back(x);
            vertices.push_back(y);
        }

        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glLineWidth(3.0f);
        glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 2);

        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
}

void drawDoors(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, busStopped ? doorsOpenTex : doorsClosedTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawTextInTopRight(const GLFWvidmode* mode) {
    char passengerText[64];
    snprintf(passengerText, sizeof(passengerText), "Number of passengers inside: %d", numberOfPassengers);
    char ticketsText[64];
    snprintf(ticketsText, sizeof(ticketsText), "Number of tickets: %d", numberOfTickets);

    float scale = 1.0f;

    float textWidth = 0.0f;
    for (char c : std::string(passengerText)) {
        Character ch = Characters[c];
        textWidth += (ch.Advance >> 6) * scale / mode->width * 2.0f;
    }

    float x = 1.0f - 20.0f / mode->width * 2.0f - textWidth;
    float y = 1.0f - 40.0f / mode->height * 2.0f;

    renderText(textShader, passengerText, x, y, scale, 0.9f, 0.9f, 0.9f, mode->width, mode->height);

    textWidth = 0.0f;
    for (char c : std::string(ticketsText)) {
        Character ch = Characters[c];
        textWidth += (ch.Advance >> 6) * scale / mode->width * 2.0f;
    }

    x = 1.0f - 20.0f / mode->width * 2.0f - textWidth;
    y = 1.0f - 90.0f / mode->height * 2.0f;

    renderText(textShader, ticketsText, x, y, scale, 0.9f, 0.9f, 0.9f, mode->width, mode->height);
}

void passengersEnterOrLeave(GLFWwindow* window, int button, int action, int mods) {
    if (!busStopped) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && numberOfPassengers < 50) {
        numberOfPassengers++;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && numberOfPassengers > 0) {
        numberOfPassengers--;
    }
}

void controlEntered(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!isControlInside && busStopped && key == GLFW_KEY_K && action == GLFW_PRESS && numberOfPassengers < 50) {
        isControlInside = true;
        numberOfPassengers++;
    }
}

void drawControl(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, controlTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Bus Route Tracker", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, passengersEnterOrLeave);
    glfwSetKeyCallback(window, controlEntered);

    glfwSetCursor(window, loadImageToCursor("../Resources/bus_stop.png"));

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    preprocessTexture(signatureTex, "../Resources/signature.png");

    unsigned int simpleTextureShader = createShader("../Shaders/simple_texture.vert", "../Shaders/simple_texture.frag");
    glUseProgram(simpleTextureShader);
    glUniform1i(glGetUniformLocation(simpleTextureShader, "signatureTex"), 0);

    preprocessTexture(busTex, "../Resources/bus.png");

    unsigned int busShader = createShader("../Shaders/bus.vert", "../Shaders/bus.frag");
    glUseProgram(busShader);
    glUniform1i(glGetUniformLocation(busShader, "busTex"), 0);

    unsigned int stationShader = createShader("../Shaders/station.vert", "../Shaders/station.frag");
    glUseProgram(stationShader);

    textShader = createShader("../Shaders/text.vert", "../Shaders/text.frag");
    initFreeType("../Resources/font.ttf");

    unsigned int pathShader = createShader("../Shaders/path.vert", "../Shaders/path.frag");
    glUseProgram(pathShader);

    preprocessTexture(doorsClosedTex, "../Resources/doors_closed.png");
    preprocessTexture(doorsOpenTex, "../Resources/doors_open.png");
    preprocessTexture(controlTex, "../Resources/bus_control.png");

    float verticesSignature[] = {
        0.5f, -0.7f, 0.0f, 1.0f, // gornje levo teme
        0.5f, -1.0f, 0.0f, 0.0f, // donje levo teme
        1.0f, -1.0f, 1.0f, 0.0f, // donje desno teme
        1.0f, -0.7f, 1.0f, 1.0f, // gornje desno teme
   };

    unsigned int VAOsignature;
    formVAOTexture(verticesSignature, sizeof(verticesSignature), VAOsignature);

    float verticesBus[] = {
        -0.06f, 0.1f, 0.0f, 1.0f, // gornje levo teme
        -0.06f, -0.1f, 0.0f, 0.0f, // donje levo teme
        0.06f, -0.1f, 1.0f, 0.0f, // donje desno teme
        0.06f, 0.1f, 1.0f, 1.0f, // gornje desno teme
   };

    unsigned int VAOBus;
    formVAOTexture(verticesBus, sizeof(verticesBus), VAOBus);

    initializeStations();
    float verticesStation[(NUM_SLICES + 2) * 2];

    float xc = 0.0f, yc = 0.0f, r = 0.1f;
    float aspect = (float)mode->width / (float)mode->height;

    verticesStation[0] = xc;
    verticesStation[1] = yc;

    for (int i = 1; i < NUM_SLICES + 2; ++i) {
        float angle = i * 2 * M_PI / NUM_SLICES;
        float x = cos(angle) * r / aspect + xc; // divide x by aspect
        float y = sin(angle) * r + yc;
        verticesStation[i * 2 + 0] = x;
        verticesStation[i * 2 + 1] = y;
    }

    unsigned int VAOstations;
    formVAOPositionOnly(verticesStation, sizeof(verticesStation), VAOstations);

    float verticesDoors[] = {
        -1.0f, -0.5f, 0.0f, 1.0f, // gornje levo teme
        -1.0f, -1.0f, 0.0f, 0.0f, // donje levo teme
        -0.65, -1.0f, 1.0f, 0.0f, // donje desno teme
        -0.65f, -0.5f, 1.0f, 1.0f, // gornje desno teme
   };

    unsigned int VAOdoors;
    formVAOTexture(verticesDoors, sizeof(verticesDoors), VAOdoors);

    float verticesControl[] = {
        -1.0f, 1.0f, 0.0f, 1.0f, // gornje levo teme
        -1.0f, 0.65f, 0.0f, 0.0f, // donje levo teme
        -0.75f, 0.65f, 1.0f, 0.0f, // donje desno teme
        -0.75f, 1.0f, 1.0f, 1.0f, // gornje desno teme
   };

    unsigned int VAOControl;
    formVAOTexture(verticesControl, sizeof(verticesControl), VAOControl);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT);

        drawSignature(simpleTextureShader, VAOsignature);
        drawStations(stationShader, VAOstations, mode);
        drawBusPaths(pathShader);
        drawBus(busShader, VAOBus);
        drawDoors(simpleTextureShader, VAOdoors);
        drawTextInTopRight(mode);

        if (isControlInside) {
            drawControl(simpleTextureShader, VAOControl);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}