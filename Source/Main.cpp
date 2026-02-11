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

// 2D Project State
struct Station {
    float x, y;
    int number;
};
Station stations[10];
bool busStopped = false;
float busJogY = 0.0f;
float busJogX = 0.0f;
int numberOfPassengers = 0;
bool isControlInside = false;
int numberOfTickets = 0;
int currentStation = 0;
int nextStation = 1;
float distanceTraveled = 0.0f;
double stopStartTime = 0.0;

struct PassengerModel {
    int modelIndex; // 0-14 for person1-15
    float scale;
    float rotationOffset;
    bool isActive;
    bool isWalkingIn;
    bool isWalkingOut;
    float walkProgress; // 0.0 to 1.0
};

struct ModelConfig {
    float baseScale;
    float rotationAdjustment; // Degrees
    float verticalOffset;     // Translation up/down
};

ModelConfig personConfigs[15] = {
    {0.15f, 0.0f, 0.0f},   // 1. Perfection
    {0.2f, 0.0f, 0.0f},    // 2. Smaller 2.5x (0.5 / 2.5 = 0.2)
    {6.0f, 0.0f, 0.0f},    // 3. 6x higher (1.0 * 6 = 6.0)
    {1.2f, 180.0f, 0.0f},  // 4. 20% higher (1.2), rotated 180 to face front (it was back)
    {2.0f, 0.0f, 0.0f},    // 5. 2x bigger (1.0 * 2 = 2.0)
    {1.0f, 0.0f, 0.33f},   // 6. Good size, translate up 1/3 of door height (approx 0.33f if door is 1.0f or similar)
    {1.2f, 0.0f, 0.0f},    // 7. 20% higher
    {0.7f, 0.0f, 0.0f},    // 8. 30% smaller
    {1.0f, 0.0f, 0.0f},    // 9. Looks good
    {0.1f, 0.0f, 0.0f},    // 10. 10x smaller
    {1.3f, 0.0f, 0.5f},    // 11. 30% larger, translate up 50%
    {0.05f, 0.0f, 0.0f},   // 12. 200x smaller (User said 20x, but 20x smaller of 1.0 is 0.05) - Wait, 20x smaller is 1/20 = 0.05
    {1.3f, 90.0f, 0.0f},   // 13. 30% larger, rotate left (90 degrees) to face driver
    {0.6f, 0.0f, 0.0f},    // 14. 40% smaller
    {2.5f, 0.0f, 0.0f}     // 15. 2.5x larger
};

ModelConfig controlConfig = {1.0f, 0.0f, 0.0f};
std::vector<PassengerModel> activePassengers;
std::vector<Model*> personModels;
Model* controlModel;
bool isPassengerWalking = false;
int pendingPassengersChange = 0; // >0 for entering, <0 for leaving
bool pendingControlChange = false;

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

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 2D Rendering globals (FBO)
unsigned int fbo, fboTex;
const unsigned int FBO_WIDTH = 1024;
const unsigned int FBO_HEIGHT = 1024;

void setupFBO() {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &fboTex);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Global textures for 2D
unsigned bus2DTex;
unsigned doorsOpenTex;
unsigned doorsClosedTex;
unsigned control2DTex;

void renderText(unsigned int shader, std::string text, float x, float y, float scale, float r, float g, float b, float screenWidth, float screenHeight);
void drawSignature(unsigned int shader, unsigned int vao);
extern unsigned int textShader;

// 2D draw functions (modified from Projekat2D)
void draw2DStations(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    GLint loc = glGetUniformLocation(shader, "uOffset");
    for (int i = 0; i < 10; i++) {
        glUniform2f(loc, stations[i].x, stations[i].y);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42); // NUM_SLICES + 2
    }

    for (int i = 0; i < 10; i++) {
        char numStr[2];
        snprintf(numStr, sizeof(numStr), "%d", i);
        renderText(textShader, numStr, stations[i].x - 0.02f, stations[i].y - 0.02f, 0.8f, 0.9f, 0.9f, 0.9f, FBO_WIDTH, FBO_HEIGHT);
    }
}

float bus2DX = -0.4f, bus2DY = 0.6f;

void draw2DBus(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bus2DTex);
    GLint loc = glGetUniformLocation(shader, "uOffset");
    glUniform2f(loc, bus2DX, bus2DY);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw2DPaths(unsigned int shader) {
    glUseProgram(shader);
    const int segments = 50;
    for (int i = 0; i < 10; i++) {
        int next = (i + 1) % 10;
        float cx = (stations[i].x + stations[next].x) / 2.0f;
        float cy = (stations[i].y + stations[next].y) / 2.0f;
        float dx = stations[next].x - stations[i].x;
        float dy = stations[next].y - stations[i].y;
        float offset = 0.35f;
        float length_dir = sqrt(dx*dx + dy*dy);
        if (length_dir > 0.0f) {
            cx += -dy / length_dir * offset;
            cy += dx / length_dir * offset;
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

void draw2DDoors(unsigned int shader, unsigned int vao) {
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, busStopped ? doorsOpenTex : doorsClosedTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw2DControl(unsigned int shader, unsigned int vao) {
    if (!isControlInside) return;
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, control2DTex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void draw2DText() {
    char passengerText[64];
    snprintf(passengerText, sizeof(passengerText), "Passengers: %d", numberOfPassengers);
    char ticketsText[64];
    snprintf(ticketsText, sizeof(ticketsText), "Tickets: %d", numberOfTickets);

    renderText(textShader, passengerText, 0.4f, 0.9f, 0.8f, 0.9f, 0.9f, 0.9f, FBO_WIDTH, FBO_HEIGHT);
    renderText(textShader, ticketsText, 0.4f, 0.8f, 0.8f, 0.9f, 0.9f, 0.9f, FBO_WIDTH, FBO_HEIGHT);
}

void updateBusLogic() {
    const float speed = 0.15f;
    const int segments = 100;
    const double stopDuration = 10.0;

    if (distanceTraveled == 0.0f) {
        if (stopStartTime == 0.0) {
            if (isControlInside) {
                int fined = (numberOfPassengers > 1) ? (rand() % (numberOfPassengers - 1)) : 0;
                numberOfTickets += fined;
                pendingControlChange = true;
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

    if (!busStopped) {
        float dx = stations[nextStation].x - stations[currentStation].x;
        float dy = stations[nextStation].y - stations[currentStation].y;
        float cx = (stations[currentStation].x + stations[nextStation].x) / 2.0f;
        float cy = (stations[currentStation].y + stations[nextStation].y) / 2.0f;
        float length_dir = sqrt(dx*dx + dy*dy);
        if (length_dir > 0.0f) {
            cx += -dy / length_dir * 0.35f;
            cy += dx / length_dir * 0.35f;
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

        distanceTraveled += speed * deltaTime;

        if (distanceTraveled >= totalLength) {
            if (isControlInside) {
                int fined = (numberOfPassengers > 1) ? (rand() % (numberOfPassengers - 1)) : 0;
                numberOfTickets += fined;
                pendingControlChange = true;
            }

            distanceTraveled = 0.0f;
            currentStation = nextStation;
            nextStation = (nextStation + 1) % 10;
            busStopped = true;
            stopStartTime = glfwGetTime();
        }

        float traveled = 0.0f;
        bus2DX = stations[currentStation].x;
        bus2DY = stations[currentStation].y;
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
                float ratio = (d > 0) ? (remain / d) : 0;
                bus2DX = lastX + (x-lastX) * ratio;
                bus2DY = lastY + (y-lastY) * ratio;
                break;
            }
            traveled += d;
            lastX = x;
            lastY = y;
        }
    } else {
        bus2DX = stations[currentStation].x;
        bus2DY = stations[currentStation].y;
    }
}

void passengersInputCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!busStopped) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        pendingPassengersChange++;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        pendingPassengersChange--;
    }
}

void controlInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (busStopped && key == GLFW_KEY_K && action == GLFW_PRESS) {
        if (!isControlInside && !pendingControlChange) {
            pendingControlChange = true;
        }
    }
}

void renderControlPanelToFBO(unsigned int busShader, unsigned int stationShader, unsigned int pathShader, unsigned int simpleShader, 
                              unsigned int busVAO, unsigned int stationVAO, unsigned int doorVAO, unsigned int controlVAO, unsigned int signatureVAO) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    drawSignature(simpleShader, signatureVAO);
    draw2DStations(stationShader, stationVAO);
    draw2DPaths(pathShader);
    draw2DBus(busShader, busVAO);
    draw2DDoors(simpleShader, doorVAO);
    draw2DText();
    draw2DControl(simpleShader, controlVAO);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float globalControlWalkProgress = 0.0f;
float doorProgress = 0.0f;

void processPassengersLogic() {
    if (isPassengerWalking) {
        bool found = false;
        
        // Handle control walking
        if (pendingControlChange) {
            globalControlWalkProgress += deltaTime / 1.5f;
            if (globalControlWalkProgress >= 1.0f) {
                globalControlWalkProgress = 0.0f;
                if (!isControlInside) {
                    isControlInside = true;
                    numberOfPassengers++;
                } else {
                    isControlInside = false;
                    numberOfPassengers--;
                }
                pendingControlChange = false;
                isPassengerWalking = false;
            }
            found = true;
        }
        
        if (!found) {
            for (auto it = activePassengers.begin(); it != activePassengers.end(); ++it) {
                if (it->isWalkingIn || it->isWalkingOut) {
                    it->walkProgress += deltaTime / 1.5f;
                    if (it->walkProgress >= 1.0f) {
                        it->walkProgress = 0.0f;
                        if (it->isWalkingIn) {
                            it->isWalkingIn = false;
                            it->isActive = true;
                            numberOfPassengers++;
                        } else {
                            it->isActive = false;
                            numberOfPassengers--;
                            activePassengers.erase(it);
                        }
                        isPassengerWalking = false;
                        pendingPassengersChange = (pendingPassengersChange > 0) ? (pendingPassengersChange - 1) : (pendingPassengersChange + 1);
                    }
                    found = true;
                    break;
                }
            }
        }
    } else {
        if (!busStopped) {
            pendingPassengersChange = 0;
            pendingControlChange = false;
            return;
        }

        if (pendingControlChange) {
            isPassengerWalking = true;
            globalControlWalkProgress = 0.0f;
        } else if (pendingPassengersChange != 0) {
            if (isControlInside) {
                pendingPassengersChange = 0;
                return;
            }
            
            if (pendingPassengersChange > 0) {
                if (numberOfPassengers >= 50) {
                    pendingPassengersChange = 0;
                    return;
                }
                PassengerModel p;
                p.modelIndex = rand() % 15;
                p.scale = personConfigs[p.modelIndex].baseScale;
                p.rotationOffset = personConfigs[p.modelIndex].rotationAdjustment;
                p.isActive = false;
                p.isWalkingIn = true;
                p.isWalkingOut = false;
                p.walkProgress = 0.0f;
                activePassengers.push_back(p);
                isPassengerWalking = true;
            } else if (pendingPassengersChange < 0) {
                if (activePassengers.empty()) {
                    pendingPassengersChange = 0;
                    return;
                }
                int idx = rand() % activePassengers.size();
                activePassengers[idx].isWalkingOut = true;
                activePassengers[idx].isActive = false;
                activePassengers[idx].walkProgress = 0.0f;
                isPassengerWalking = true;
            } else {
                pendingPassengersChange = 0;
            }
        }
    }
}

extern float busJogY;
extern float busJogX;

void draw3DPassengers(Shader& shader) {
    for (auto& p : activePassengers) {
        if (p.isActive || p.isWalkingIn || p.isWalkingOut) {
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec3 pos;
            float angle = -90.0f;

            glm::vec3 outside(3.5f, -1.0f, -4.0f);
            glm::vec3 door(2.0f, -1.0f, -4.0f);
            glm::vec3 inside(-1.0f, -1.0f, 2.0f);

            bool isInside = p.isActive || (p.isWalkingIn && p.walkProgress >= 0.5f) || (p.isWalkingOut && p.walkProgress < 0.5f);

            if (p.isActive) {
                pos = inside;
            } else if (p.isWalkingIn) {
                if (p.walkProgress < 0.5f) {
                    pos = glm::mix(outside, door, p.walkProgress * 2.0f);
                    angle = -90.0f;
                } else {
                    pos = glm::mix(door, inside, (p.walkProgress - 0.5f) * 2.0f);
                    angle = 180.0f;
                }
            } else if (p.isWalkingOut) {
                if (p.walkProgress < 0.5f) {
                    pos = glm::mix(inside, door, p.walkProgress * 2.0f);
                    angle = 0.0f;
                } else {
                    pos = glm::mix(door, outside, (p.walkProgress - 0.5f) * 2.0f);
                    angle = 90.0f;
                }
            }

            if (isInside) {
                pos.x += busJogX;
                pos.y += busJogY;
            }
            pos.y += personConfigs[p.modelIndex].verticalOffset;

            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(angle + p.rotationOffset), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(p.scale));
            shader.setMat4("uM", model);
            personModels[p.modelIndex]->Draw(shader);
        }
    }

    // Draw Control if inside or walking
    if (isControlInside || (pendingControlChange && isPassengerWalking)) {
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos;
        float angle = -90.0f;

        glm::vec3 outside(3.5f, -1.0f, -4.0f);
        glm::vec3 door(2.0f, -1.0f, -4.0f);
        glm::vec3 inside(-0.5f, -1.0f, -4.0f); // Control stands next to driver

        bool isInside = (isControlInside && !pendingControlChange) || 
                        (pendingControlChange && ((!isControlInside && globalControlWalkProgress >= 0.5f) || (isControlInside && globalControlWalkProgress < 0.5f)));

        if (isControlInside && !pendingControlChange) {
            pos = inside;
            angle = 90.0f;
        } else if (pendingControlChange) {
            bool walkingIn = !isControlInside;
            if (walkingIn) {
                if (globalControlWalkProgress < 0.5f) {
                    pos = glm::mix(outside, door, globalControlWalkProgress * 2.0f);
                    angle = -90.0f;
                } else {
                    pos = glm::mix(door, inside, (globalControlWalkProgress - 0.5f) * 2.0f);
                    angle = 180.0f;
                }
            } else {
                if (globalControlWalkProgress < 0.5f) {
                    pos = glm::mix(inside, door, globalControlWalkProgress * 2.0f);
                    angle = 0.0f;
                } else {
                    pos = glm::mix(door, outside, (globalControlWalkProgress - 0.5f) * 2.0f);
                    angle = 90.0f;
                }
            }
        }

        if (isInside) {
            pos.x += busJogX;
            pos.y += busJogY;
        }
        pos.y += controlConfig.verticalOffset;

        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(angle + controlConfig.rotationAdjustment), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(controlConfig.baseScale));
        shader.setMat4("uM", model);
        controlModel->Draw(shader);
    }
}

// render settings
bool depthTestEnabled = true;
bool faceCullingEnabled = false;

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
    glGenVertexArrays(1, &vao);
    unsigned int vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
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
    // Back Face (Normal is 0,0,-1) - CCW from outside
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

    // Front Face (Normal 0,0,1) - CCW from outside
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    // Left Face (Normal -1,0,0) - CCW from outside
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    // Right Face (Normal 1,0,0) - CCW from outside
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    // Bottom Face (Normal 0,-1,0) - CCW from outside
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    // Top Face (Normal 0,1,0) - CCW from outside
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
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

// light settings
glm::vec3 lightPos(0.0f, 1.8f, -3.0f); // Inside the bus, near the roof
glm::vec3 lightColor(0.95f, 0.9f, 0.7f); // Warm yellow-ish light
float lightIntensity = 1.2f;

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
    glfwSetMouseButtonCallback(window, passengersInputCallback);
    glfwSetKeyCallback(window, controlInputCallback);
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
        1.0f, -0.7f, 1.0f, 1.0f, // gornje desno teme
        1.0f, -1.0f, 1.0f, 0.0f, // donje desno teme
        0.5f, -1.0f, 0.0f, 0.0f, // donje levo teme
    };

    unsigned int VAOsignature;
    formVAOTexture(verticesSignature, sizeof(verticesSignature), VAOsignature);

    unsigned int cubeVAO, cubeVBO;
    formVAO3D(cubeVertices, sizeof(cubeVertices), cubeVAO, cubeVBO);

    unsigned int rectVAO, rectVBO;
    float rectVertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f
    };
    formVAO3D(rectVertices, sizeof(rectVertices), rectVAO, rectVBO);

    unsigned int busColorTex = createColorTexture(0.3f, 0.3f, 0.3f); // Grey-ish bus
    unsigned int windshieldTex = createColorTexture(0.1f, 0.1f, 0.1f, 0.5f); // Dark transparent
    unsigned int controlPanelTex = createColorTexture(1.0f, 0.0f, 0.0f); // Red
    unsigned int wheelTex = createColorTexture(0.15f, 0.15f, 0.15f); // Dark gray
    unsigned int doorTex = createColorTexture(0.2f, 0.6f, 0.3f); // Dark doors
    unsigned int lightTex = createColorTexture(lightColor.r, lightColor.g, lightColor.b); // Light source color

    setupFBO();
    preprocessTexture(bus2DTex, "../Projekat2D/Resources/bus.png");
    preprocessTexture(doorsClosedTex, "../Projekat2D/Resources/doors_closed.png");
    preprocessTexture(doorsOpenTex, "../Projekat2D/Resources/doors_open.png");
    preprocessTexture(control2DTex, "../Projekat2D/Resources/bus_control.png");

    unsigned int bus2DShader = createShader("../Projekat2D/Shaders/bus.vert", "../Projekat2D/Shaders/bus.frag");
    unsigned int station2DShader = createShader("../Projekat2D/Shaders/station.vert", "../Projekat2D/Shaders/station.frag");
    unsigned int path2DShader = createShader("../Projekat2D/Shaders/path.vert", "../Projekat2D/Shaders/path.frag");

    initializeStations();

    float verticesBus2D[] = {
        -0.06f, 0.1f, 0.0f, 1.0f,
        0.06f, 0.1f, 1.0f, 1.0f,
        0.06f, -0.1f, 1.0f, 0.0f,
        -0.06f, -0.1f, 0.0f, 0.0f,
    };
    unsigned int VAOBus2D;
    formVAOTexture(verticesBus2D, sizeof(verticesBus2D), VAOBus2D);

    float verticesStation2D[42 * 2];
    float xc2D = 0.0f, yc2D = 0.0f, r2D = 0.1f;
    float aspect2D = (float)FBO_WIDTH / (float)FBO_HEIGHT;
    verticesStation2D[0] = xc2D;
    verticesStation2D[1] = yc2D;
    for (int i = 1; i < 42; ++i) {
        float angle = i * 2 * 3.14159f / 40.0f;
        verticesStation2D[i * 2 + 0] = cos(angle) * r2D / aspect2D + xc2D;
        verticesStation2D[i * 2 + 1] = sin(angle) * r2D + yc2D;
    }
    unsigned int VAOstations2D;
    formVAOPositionOnly(verticesStation2D, sizeof(verticesStation2D), VAOstations2D);

    float verticesDoors2D[] = {
        -1.0f, -0.5f, 0.0f, 1.0f,
        -0.65, -0.5f, 1.0f, 1.0f,
        -0.65, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
    };
    unsigned int VAOdoors2D;
    formVAOTexture(verticesDoors2D, sizeof(verticesDoors2D), VAOdoors2D);

    float verticesControl2D[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -0.75f, 1.0f, 1.0f, 1.0f,
        -0.75f, 0.65f, 1.0f, 0.0f,
        -1.0f, 0.65f, 0.0f, 0.0f,
    };
    unsigned int VAOcontrol2D;
    formVAOTexture(verticesControl2D, sizeof(verticesControl2D), VAOcontrol2D);

    // Load person models
    for (int i = 1; i <= 15; i++) {
        std::string path = "../Resources/person" + std::to_string(i) + "/model.obj";
        personModels.push_back(new Model(path));
    }
    controlModel = new Model("../Resources/control/control.obj");

    textShader = createShader("../Projekat2D/Shaders/text.vert", "../Projekat2D/Shaders/text.frag");
    initFreeType("../Projekat2D/Resources/font.ttf");

    Shader unifiedShader("../Shaders/basic.vert", "../Shaders/basic.frag");

    Model tree("../Resources/tree/Tree.obj");
    Model wheel("../Resources/wheel/merc steering.obj");
    Model cigarette("../Resources/cigarette/CHAHIN_CIGARETTE_BUTT.obj");

    camera.Position = glm::vec3(-1.0f, 0.5f, -4.0f);

    glClearColor(0.3f, 0.4f, 0.8f, 1.0f); // kind of sky color
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        static bool key1Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            if (!key1Pressed) {
                depthTestEnabled = !depthTestEnabled;
                if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
                else glDisable(GL_DEPTH_TEST);
                key1Pressed = true;
            }
        } else {
            key1Pressed = false;
        }

        static bool key2Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            if (!key2Pressed) {
                faceCullingEnabled = !faceCullingEnabled;
                if (faceCullingEnabled) glEnable(GL_CULL_FACE);
                else glDisable(GL_CULL_FACE);
                key2Pressed = true;
            }
        } else {
            key2Pressed = false;
        }

        updateBusLogic();
        processPassengersLogic();

        // Calculate bus jogging
        if (!busStopped) {
            float time = (float)glfwGetTime();
            busJogY = sin(time * 10.0f) * 0.02f; // Up-down
            busJogX = cos(time * 7.0f) * 0.01f;  // Slight left-right
        } else {
            busJogY = 0.0f;
            busJogX = 0.0f;
        }

        // 1. Render Control Panel to FBO
        renderControlPanelToFBO(bus2DShader, station2DShader, path2DShader, simpleTextureShader, 
                                VAOBus2D, VAOstations2D, VAOdoors2D, VAOcontrol2D, VAOsignature);

        // Reset viewport to screen size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);

        glClearColor(0.3f, 0.4f, 0.8f, 1.0f); // Set sky color for main scene
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        unifiedShader.use();
        unifiedShader.setVec3("uLightPos", lightPos);
        unifiedShader.setVec3("uViewPos", camera.Position);
        unifiedShader.setVec3("uLightColor", lightColor);
        unifiedShader.setFloat("uLightIntensity", lightIntensity);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);
        unifiedShader.setMat4("uP", projection);

        glm::vec3 originalCamPos = camera.Position;
        camera.Position += glm::vec3(busJogX, busJogY, 0.0f);
        glm::mat4 view = camera.GetViewMatrix();
        unifiedShader.setMat4("uV", view);
        camera.Position = originalCamPos; // Restore so movement logic isn't messed up

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
        model = glm::translate(model, glm::vec3(busJogX, -1.0f + busJogY, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 0.1f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Left wall
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f + busJogX, 0.5f + busJogY, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Right wall (partial, for door)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f + busJogX, 0.5f + busJogY, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 8.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Roof
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, 2.0f + busJogY, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 0.1f, 10.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Back wall
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, 0.5f + busJogY, 5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 3.0f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Front lower part (where control panel is)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, -0.25f + busJogY, -5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 1.5f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Windshield (Transparent) - Moved here to be drawn after opaque but before interior items
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE); // Don't write to depth buffer for transparent objects
        glBindTexture(GL_TEXTURE_2D, windshieldTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, 1.25f + busJogY, -5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 1.5f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE); // Re-enable depth writing

        // Door (Synchronized with busStopped)
        float doorSpeed = 2.0f; 
        if (busStopped) {
            doorProgress += deltaTime * doorSpeed;
            if (doorProgress > 1.0f) doorProgress = 1.0f;
        } else {
            doorProgress -= deltaTime * doorSpeed;
            if (doorProgress < 0.0f) doorProgress = 0.0f;
        }

        glBindTexture(GL_TEXTURE_2D, doorTex);
        model = glm::mat4(1.0f);
        float doorAngle = doorProgress * -90.0f; // Opens 90 degrees outwards
        model = glm::translate(model, glm::vec3(2.0f + busJogX, 0.5f + busJogY, -3.0f)); 
        model = glm::rotate(model, glm::radians(doorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f)); 
        model = glm::scale(model, glm::vec3(0.1f, 3.0f, 2.0f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Control Panel (using FBO texture for the screen, and red for the backing)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, busJogY, -4.8f)); 
        model = glm::scale(model, glm::vec3(1.0f, 0.6f, 0.1f));
        unifiedShader.setMat4("uM", model);

        // Draw the red base
        glBindTexture(GL_TEXTURE_2D, controlPanelTex);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw the functional screen on front face
        glBindTexture(GL_TEXTURE_2D, fboTex);
        glBindVertexArray(rectVAO);
        glm::mat4 screenModel = model;
        screenModel = glm::translate(screenModel, glm::vec3(0.0f, 0.0f, 0.501f)); // Slightly in front of the cube face
        unifiedShader.setMat4("uM", screenModel);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3D Passengers
        draw3DPassengers(unifiedShader);

        // Steering Wheel
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wheelTex);
        
        // Simulating wheel movement (slight left-right rotation)
        float wheelRotation = sin(glfwGetTime() * 1.5f) * 15.0f; // Oscillation between -15 and 15 degrees
        
        model = glm::mat4(1.0f);
        // Position the wheel in the bus (Y adjusted from 0.0f to 0.26f to compensate for centering translation)
        model = glm::translate(model, glm::vec3(-1.0f + busJogX, 0.26f + busJogY, -4.5f));
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f)); 
        model = glm::rotate(model, glm::radians(wheelRotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.11f));
        model = glm::translate(model, glm::vec3(0.0f, -2.39f, 0.0f)); // Center the wheel (Y center is ~2.39)
        
        unifiedShader.setMat4("uM", model);
        wheel.Draw(unifiedShader);

        // Cigarette
        model = glm::mat4(1.0f);
        // Base position near the right side of the wheel (slightly lower: 0.45f -> 0.42f)
        glm::vec3 cigaretteBasePos = glm::vec3(-0.7f + busJogX, 0.38f + busJogY, -4.6f);
        // Camera (driver's face) is roughly at (-1.0f, 0.5f, -4.0f)
        glm::vec3 cigaretteTargetPos = glm::vec3(-0.95f + busJogX, 0.38f + busJogY, -4.15f);

        float smokingCycle = 10.0f; // total cycle in seconds
        float currentTime = (float)glfwGetTime();
        float timeInCycle = fmod(currentTime, smokingCycle);
        
        float smokingDuration = 3.0f; // how long it takes to move to face and back
        glm::vec3 currentCigarettePos = cigaretteBasePos;

        if (timeInCycle < smokingDuration) {
            // Normalize time in smoking duration to [0, 1]
            float t = timeInCycle / smokingDuration;
            // Use a smooth movement (sinusoidal) for back and forth
            // sin(0) = 0, sin(pi/2) = 1 (at face), sin(pi) = 0 (back)
            float moveFactor = sin(t * 3.14159f); 
            currentCigarettePos = glm::mix(cigaretteBasePos, cigaretteTargetPos, moveFactor);
        }

        model = glm::translate(model, currentCigarettePos);
        model = glm::rotate(model, glm::radians(50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.3f));
        unifiedShader.setMat4("uM", model);
        cigarette.Draw(unifiedShader);


        // Windshield (Transparent) - Draw after opaque interior and passengers
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBindTexture(GL_TEXTURE_2D, windshieldTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(busJogX, 1.25f + busJogY, -5.0f));
        model = glm::scale(model, glm::vec3(4.0f, 1.5f, 0.1f));
        unifiedShader.setMat4("uM", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);

        // Light Source (Visual Representation)
        unifiedShader.use();
        unifiedShader.setFloat("uLightIntensity", 5.0f); // Make it bright
        glBindTexture(GL_TEXTURE_2D, lightTex);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); 
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