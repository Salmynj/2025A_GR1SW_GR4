#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 2.0f, 5.0f));
glm::vec3 cameraTargetPos = camera.Position;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// vehículo
glm::vec3 carPosition = glm::vec3(0.0f);
glm::vec3 carDirection = glm::vec3(0.0f, 0.0f, -1.0f);
float carYaw = 0.0f;
float carSpeed = 5.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "FantastiCar", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");
    Model ourModel("C:/Users/ASUS/source/repos/ProyectoCompu/ProyectoCompu/models/FantastiCar+Herbie/FantastiCar+Herbie.obj");

    while (!glfwWindowShouldClose(window))
    {
        // tiempo por frame
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // dirección del carro
        float radians = glm::radians(carYaw);
        carDirection = glm::vec3(sin(radians), 0.0f, -cos(radians));

        // posición deseada de la cámara (detrás del carro)
        glm::vec3 desiredCameraPos = carPosition + glm::normalize(-carDirection) * 1.5f + glm::vec3(0.0f, 0.4f, 0.0f);
        cameraTargetPos = glm::mix(cameraTargetPos, desiredCameraPos, 6.0f * deltaTime);
        camera.Position = cameraTargetPos;

        // que mire al frente (ligeramente abajo y hacia adelante)
        camera.Front = glm::normalize((carPosition + glm::vec3(0.0f, 0.3f, -1.2f)) - camera.Position);

        // render
        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // modelo del carro
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, carPosition);
        model = glm::rotate(model, glm::radians(carYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // rotado para mirar bien
        model = glm::scale(model, glm::vec3(0.1f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        // piso plano simple
        glm::mat4 floorModel = glm::mat4(1.0f);
        floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.05f, 0.0f));
        floorModel = glm::scale(floorModel, glm::vec3(50.0f, 0.01f, 50.0f));
        ourShader.setMat4("model", floorModel);
        ourModel.Draw(ourShader); // reutiliza el modelo si no tienes un piso propio

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float radians = glm::radians(carYaw);
    glm::vec3 forward = glm::vec3(sin(radians), 0.0f, -cos(radians));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        carPosition += forward * carSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        carPosition -= forward * carSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        carPosition -= right * carSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        carPosition += right * carSpeed * deltaTime;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    lastX = xpos;
    lastY = ypos;
    // mouse desactivado
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
