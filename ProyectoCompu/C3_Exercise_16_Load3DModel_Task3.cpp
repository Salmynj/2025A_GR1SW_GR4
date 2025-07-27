// Código final: al llegar al trigger invisible frente a Galactus se gana, se muestra Win.obj y se congela el carro

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void spawnAsteroid();
bool checkCollision(const glm::vec3& a, const glm::vec3& b, float threshold = 0.8f);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 2.0f, 5.0f));
glm::vec3 cameraTargetPos = camera.Position;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::vector<float> lanePositions = { -2.0f, 0.0f, 2.0f };
int currentLane = 1;
glm::vec3 carPosition = glm::vec3(0.0f);
glm::vec3 carDirection = glm::vec3(0.0f, 0.0f, -1.0f);
float carYaw = 0.0f;
float carSpeed = 5.0f;
bool gameOver = false;
bool win = false;
bool rPressed = false;
bool gameStarted = false;
float gameOverTimer = 0.0f;

float headRotationAngle = 0.0f;
float headRotationSpeed = 5.0f;
float headRotationLimit = 20.0f;

struct Asteroid {
    glm::vec3 position;
    float speed;
};

std::vector<Asteroid> asteroids;
float asteroidSpawnTimer = 0.0f;
float spawnInterval = 1.0f;

void spawnAsteroid() {
    std::vector<glm::vec3> spawnPositions = {
        glm::vec3(0.0f, 0.0f, -100.0f),
        glm::vec3(-2.0f, 0.0f, -100.0f),
        glm::vec3(2.0f, 0.0f, -100.0f)
    };
    int index = rand() % 3;
    Asteroid a;
    a.position = spawnPositions[index];
    a.speed = 20.0f;
    asteroids.push_back(a);
}

bool checkCollision(const glm::vec3& a, const glm::vec3& b, float threshold) {
    return glm::distance(a, b) < threshold;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "FantastiCar", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");

    Model ourModel("models/FantastiCar+Herbie/FantastiCar+Herbie.obj");
    Model asteroidModel("models/asteroid/asteroid01.obj");
    Model startModel("models/StartScreen/startScreen.obj");
    Model gameOverModel("models/GameOver/GameOver.obj");
    Model galactusModel("models/Galactus/GalactusCuerpo.obj");
    Model galactusHeadModel("models/Galactus/GalactusCabeza.obj");
    Model winModel("models/Win/Win.obj");

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        if (!gameStarted) {
            camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
            camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
            glm::mat4 startM = glm::mat4(1.0f);
            startM = glm::scale(startM, glm::vec3(0.5f));
            ourShader.setMat4("model", startM);
            startModel.Draw(ourShader);

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
                gameStarted = true;
                rPressed = true;
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rPressed = false;

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        if (gameOver || win) {
            gameOverTimer += deltaTime;
            if (gameOverTimer >= 0.1f) {
                camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
                camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
                glm::mat4 endM = glm::mat4(1.0f);
                endM = glm::scale(endM, glm::vec3(0.5f));
                ourShader.setMat4("model", endM);
                if (gameOver)
                    gameOverModel.Draw(ourShader);
                else
                    winModel.Draw(ourShader);
            }

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
                gameOver = false;
                win = false;
                gameStarted = true;
                carPosition = glm::vec3(0.0f);
                currentLane = 1;
                asteroids.clear();
                asteroidSpawnTimer = 0.0f;
                gameOverTimer = 0.0f;
                rPressed = true;
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rPressed = false;

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        float radians = glm::radians(carYaw);
        carDirection = glm::vec3(sin(radians), 0.0f, -cos(radians));
        glm::vec3 desiredCameraPos = carPosition + glm::normalize(-carDirection) * 1.5f + glm::vec3(0.0f, 0.4f, 0.0f);
        cameraTargetPos = glm::mix(cameraTargetPos, desiredCameraPos, 3.0f * deltaTime);
        camera.Position = cameraTargetPos;
        camera.Front = glm::normalize((carPosition + glm::vec3(0.0f, 0.3f, -1.2f)) - camera.Position);

        if (!win) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, carPosition);
            model = glm::rotate(model, glm::radians(carYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f));
            ourShader.setMat4("model", model);
            ourModel.Draw(ourShader);
        }

        glm::vec3 galactusPos = glm::vec3(0.0f, -96.0f, -90.0f);

        glm::mat4 gBody = glm::mat4(1.0f);
        gBody = glm::translate(gBody, galactusPos);
        gBody = glm::rotate(gBody, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        gBody = glm::scale(gBody, glm::vec3(40.0f));
        ourShader.setMat4("model", gBody);
        galactusModel.Draw(ourShader);

        glm::vec3 galactusHeadPos = galactusPos + glm::vec3(0.0f, 20.0f, 0.0f);

        glm::mat4 gHead = glm::translate(gBody, glm::vec3(0.0f));
        headRotationAngle += headRotationSpeed * deltaTime;
        if (abs(headRotationAngle) >= headRotationLimit) {
            headRotationSpeed *= -1;
            headRotationAngle = glm::clamp(headRotationAngle, -headRotationLimit, headRotationLimit);
        }
        gHead = glm::rotate(gHead, glm::radians(headRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", gHead);
        galactusHeadModel.Draw(ourShader);

        // Punto de colisión invisible delante de Galactus
        glm::vec3 winTriggerPos = glm::vec3(0.0f, 0.0f, -85);
        if (checkCollision(carPosition, winTriggerPos, 2.0f)) {
            win = true;
            gameOverTimer = 0.0f;
        }

        asteroidSpawnTimer += deltaTime;
        if (asteroidSpawnTimer >= spawnInterval) {
            spawnAsteroid();
            asteroidSpawnTimer = 0.0f;
        }

        for (const Asteroid& ast : asteroids) {
            if (checkCollision(carPosition, ast.position)) {
                gameOver = true;
                gameOverTimer = 0.0f;
                break;
            }
        }

        for (Asteroid& ast : asteroids) {
            ast.position.z += ast.speed * deltaTime;
            glm::mat4 asteroidM = glm::mat4(1.0f);
            asteroidM = glm::translate(asteroidM, ast.position);
            asteroidM = glm::scale(asteroidM, glm::vec3(0.007f));
            ourShader.setMat4("model", asteroidM);
            asteroidModel.Draw(ourShader);
        }

        asteroids.erase(std::remove_if(asteroids.begin(), asteroids.end(),
            [](const Asteroid& a) { return a.position.z > 10.0f; }),
            asteroids.end());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!gameStarted || gameOver || win) return;

    static bool aPressed = false;
    static bool dPressed = false;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !aPressed) {
        if (currentLane > 0) currentLane--;
        aPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) aPressed = false;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dPressed) {
        if (currentLane < 2) currentLane++;
        dPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) dPressed = false;

    carPosition.x = glm::mix(carPosition.x, lanePositions[currentLane], 10.0f * deltaTime);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        carPosition.z -= carSpeed * deltaTime;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
