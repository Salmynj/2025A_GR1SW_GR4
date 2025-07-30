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
#include <string>
#include <sstream>
#include <iomanip>

#include <AL/al.h>
#include <AL/alc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/stb_image.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void spawnAsteroid();
bool checkCollision(const glm::vec3& a, const glm::vec3& b, float threshold = 0.8f);
bool LoadWavFile(const char* filename, std::vector<char>& buffer, ALenum& format, ALsizei& freq);
void playMusic(int musicIndex, bool loop);
void stopMusic();
void updateAudio();


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
//Estados del juego
bool gameOver = false;
bool win = false;
bool rPressed = false;
bool gameStarted = false;
float gameOverTimer = 0.0f;
// Variables para Galactus
float headRotationAngle = 0.0f;
float headRotationSpeed = 5.0f;
float headRotationLimit = 20.0f;

// PANTALLAS ANIMADAS COMENTADAS - Descomenta para usar las animaciones completas
/*
// Variables para animación de StartScreen en pantalla de inicio
std::vector<Model*> startScreenFrames;
int currentStartScreenFrame = 0;
float startScreenAnimationTimer = 0.0f;
float startScreenFrameRate = 24.0f; // 24 fps

// Variables para animación de GameOver
std::vector<Model*> gameOverFrames;
int currentGameOverFrame = 0;
float gameOverAnimationTimer = 0.0f;
float gameOverFrameRate = 24.0f; // 24 fps

// Variables para animación de Win
std::vector<Model*> winFrames;
int currentWinFrame = 0;
float winAnimationTimer = 0.0f;
float winFrameRate = 24.0f; // 24 fps
*/

// Variables para audio
ALCdevice* audioDevice = nullptr;
ALCcontext* audioContext = nullptr;

// Música
ALuint musicBuffers[4]; // 0:inicio, 1:juego, 2:gameover, 3:victoria
ALuint musicSource;
int currentMusic = -1;
bool musicLooping = false;

//Esquivación
ALuint dodgeSoundBuffer = 0;
std::vector<ALuint> dodgeSoundSources;
float lastDodgeTime = 0.0f;
float dodgeSoundCooldown = 0.2f;

//Aceleración
ALuint accelerationSoundBuffer = 0;
ALuint accelerationSoundSource = 0;
bool isAccelerating = false;

//Grito al perder y ganar
ALuint winSoundBuffer = 0;
ALuint loseSoundBuffer = 0;
float gameEndDelay = 1.5f; // 1.5 segundos de retraso
bool endSoundPlayed = false;

bool LoadWavFile(const char* filename, std::vector<char>& buffer, ALenum& format, ALsizei& freq) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) return false;

    file.seekg(22, std::ios::beg);
    short channels;
    file.read(reinterpret_cast<char*>(&channels), 2);

    file.read(reinterpret_cast<char*>(&freq), 4);

    file.seekg(34, std::ios::beg);
    short bitsPerSample;
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    char dataHeader[4];
    int dataSize = 0;
    while (file.read(dataHeader, 4)) {
        file.read(reinterpret_cast<char*>(&dataSize), 4);
        if (std::strncmp(dataHeader, "data", 4) == 0) break;
        file.seekg(dataSize, std::ios::cur);
    }
    if (dataSize == 0) return false;

    buffer.resize(dataSize);
    file.read(buffer.data(), dataSize);

    if (channels == 1) {
        format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    }
    else {
        format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    }
    return true;
}

// Control de música
void playMusic(int musicIndex, bool loop) {
    if (musicIndex < 0 || musicIndex >= 4) return;
    if (!musicBuffers[musicIndex]) return;

    alSourceStop(musicSource);
    alSourcei(musicSource, AL_BUFFER, musicBuffers[musicIndex]);
    alSourcei(musicSource, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcePlay(musicSource);
    currentMusic = musicIndex;
    musicLooping = loop;
}

void stopMusic() {
    alSourceStop(musicSource);
    currentMusic = -1;
}
struct Asteroid {
    glm::vec3 position;
    float speed;
};

// Función para reproducir sonido de esquivación
void playDodgeSound() {
    float currentTime = glfwGetTime();
    if (currentTime - lastDodgeTime < dodgeSoundCooldown) return;

    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, dodgeSoundBuffer);
    alSourcef(source, AL_GAIN, 0.6f);
    alSourcePlay(source);
    dodgeSoundSources.push_back(source);
    lastDodgeTime = currentTime;
}


// Función para manejar el sonido de aceleración
void handleAccelerationSound(bool accelerating) {
    if (accelerating && !isAccelerating) {
        // Comenzar a acelerar
        alSourcei(accelerationSoundSource, AL_BUFFER, accelerationSoundBuffer);
        alSourcei(accelerationSoundSource, AL_LOOPING, AL_TRUE);
        alSourcePlay(accelerationSoundSource);
        isAccelerating = true;
    }
    else if (!accelerating && isAccelerating) {
        // Dejar de acelerar
        alSourceStop(accelerationSoundSource);
        isAccelerating = false;
    }
}

// Función para reproducir sonidos de final
void playEndGameSound(bool won) {
    ALuint buffer = won ? winSoundBuffer : loseSoundBuffer;
    if (buffer == 0) return;

    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, 0.8f);
    alSourcePlay(source);
}

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
#ifdef _APPLE_
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
    glEnable(GL_PROGRAM_POINT_SIZE); // solo para probar si se están dibujando los puntos
    glDisable(GL_CULL_FACE);

    Shader lightCubeShader("shaders/shader_exercise15_lightcube.vs", "shaders/shader_exercise15_lightcube.fs");
    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");
    Shader ojitosShader("shaders/shader_exercise9t5.vs", "shaders/shader_exercise9t5.fs");
    Shader flashlightShader("shaders/shader_flashlight.vs", "shaders/shader_flashlight.fs");


    Model nebulaModel("models/nebula/nebula.obj");
    Model ourModel("models/FantastiCar+Herbie/FantastiCar+Herbie.obj");
    Model asteroidModel("models/asteroid/asteroid01.obj");
    Model galactusModel("models/Galactus/GalactusCuerpo.obj");
    Model galactusHeadModel("models/Galactus/GalactusCabeza.obj");
    Model galactusOjitosModel("models/ojitos/ojitos2.obj");

#define NR_POINT_LIGHTS 10000
    glm::vec3 pointLightPositions[NR_POINT_LIGHTS];

    for (int i = 0; i < NR_POINT_LIGHTS; i++) {
        float x = ((i % 10) - 5) * 20.0f; // 10 luces por fila
        float y = ((i / 10) - 2) * 10.0f; // 5 filas
        float z = sin(i * 1.0f) * 15.0f;
        pointLightPositions[i] = glm::vec3(x, y + 10.0f, z - 150.0f);
    }

    float cubeVertices[] = {
        // posiciones         
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,

        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,

         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,

        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);



    // CARGA DE PANTALLAS ANIMADAS COMENTADA - Descomenta para usar las animaciones completas
    /*
    // Cargar frames de animación de StartScreen para pantalla de inicio
    startScreenFrames.resize(20);
    for (int i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << "models/StartScreen/StartScreen" << std::setfill('0') << std::setw(4) << (i + 1) << ".obj";
        try {
            startScreenFrames[i] = new Model(ss.str());
        }
        catch (...) {
            std::cout << "Error cargando frame de StartScreen: " << ss.str() << std::endl;
            startScreenFrames[i] = nullptr;
        }
    }

    // Cargar frames de animación de GameOver
    gameOverFrames.resize(20);
    for (int i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << "models/GameOver/GameOver" << std::setfill('0') << std::setw(4) << (i + 1) << ".obj";
        try {
            gameOverFrames[i] = new Model(ss.str());
        }
        catch (...) {
            std::cout << "Error cargando frame de GameOver: " << ss.str() << std::endl;
            gameOverFrames[i] = nullptr;
        }
    }

    // Cargar frames de animación de Win
    winFrames.resize(20);
    for (int i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << "models/Win/Win" << std::setfill('0') << std::setw(4) << (i + 1) << ".obj";
        try {
            winFrames[i] = new Model(ss.str());
        }
        catch (...) {
            std::cout << "Error cargando frame de Win: " << ss.str() << std::endl;
            winFrames[i] = nullptr;
        }
    }
    */

    //Música
    // Inicialización OpenAL
    audioDevice = alcOpenDevice(nullptr);
    if (!audioDevice) {
        std::cerr << "Error al abrir dispositivo de audio" << std::endl;
        return -1;
    }

    audioContext = alcCreateContext(audioDevice, nullptr);
    if (!audioContext || !alcMakeContextCurrent(audioContext)) {
        std::cerr << "Error al crear contexto de audio" << std::endl;
        return -1;
    }

    // Cargar música
    const char* musicFiles[4] = {
        "audio/Lobby.wav",
        "audio/InGame.wav",
        "audio/Lose.wav",
        "audio/Win.wav"
    };

    for (int i = 0; i < 4; i++) {
        std::vector<char> bufferData;
        ALenum format;
        ALsizei freq;

        if (!LoadWavFile(musicFiles[i], bufferData, format, freq)) {
            std::cout << "Error cargando: " << musicFiles[i] << std::endl;
            musicBuffers[i] = 0;
        }
        else {
            alGenBuffers(1, &musicBuffers[i]);
            alBufferData(musicBuffers[i], format, bufferData.data(), (ALsizei)bufferData.size(), freq);
        }
    }

    alGenSources(1, &musicSource);
    alSourcef(musicSource, AL_GAIN, 0.7f);

    // Sonido de esquivación
    std::vector<char> dodgeData;
    ALenum dodgeFormat;
    ALsizei dodgeFreq;
    if (!LoadWavFile("audio/Esquiva.wav", dodgeData, dodgeFormat, dodgeFreq)) {
        std::cout << "Error cargando sonido de esquivación" << std::endl;
    }
    else {
        alGenBuffers(1, &dodgeSoundBuffer);
        alBufferData(dodgeSoundBuffer, dodgeFormat, dodgeData.data(), (ALsizei)dodgeData.size(), dodgeFreq);
    }

    // Cargar sonido de aceleración
    std::vector<char> accelerationData;
    ALenum accelerationFormat;
    ALsizei accelerationFreq;
    if (!LoadWavFile("audio/Speed.wav", accelerationData, accelerationFormat, accelerationFreq)) {
        std::cout << "Error cargando sonido de aceleración" << std::endl;
    }
    else {
        alGenBuffers(1, &accelerationSoundBuffer);
        alBufferData(accelerationSoundBuffer, accelerationFormat, accelerationData.data(),
            (ALsizei)accelerationData.size(), accelerationFreq);
        alGenSources(1, &accelerationSoundSource);
        alSourcef(accelerationSoundSource, AL_GAIN, 6.0f);
    }


    // Cargar sonidos de final
    std::vector<char> winSoundData, loseSoundData;
    ALenum winFormat, loseFormat;
    ALsizei winFreq, loseFreq;

    if (!LoadWavFile("audio/Yei.wav", winSoundData, winFormat, winFreq)) {
        std::cout << "Error cargando sonido de celebración" << std::endl;
    }
    else {
        alGenBuffers(1, &winSoundBuffer);
        alBufferData(winSoundBuffer, winFormat, winSoundData.data(),
            (ALsizei)winSoundData.size(), winFreq);
    }

    if (!LoadWavFile("audio/Scream.wav", loseSoundData, loseFormat, loseFreq)) {
        std::cout << "Error cargando sonido de game over" << std::endl;
    }
    else {
        alGenBuffers(1, &loseSoundBuffer);
        alBufferData(loseSoundBuffer, loseFormat, loseSoundData.data(),
            (ALsizei)loseSoundData.size(), loseFreq);
    }


    //Render loop ---------------------------------------------------------------------



    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        //ourShader.setFloat("time", glfwGetTime());
        ojitosShader.setFloat("time", glfwGetTime());

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 nebulaM = glm::mat4(1.0f);
        nebulaM = glm::translate(nebulaM, glm::vec3(0.0f, 0.0f, 0.0f));
        nebulaM = glm::scale(nebulaM, glm::vec3(150.0f, 150.0f, 150.0f));
        ourShader.setMat4("model", nebulaM);
        nebulaModel.Draw(ourShader);

        // Control de música basado en el estado del juego
        if (!gameStarted) {
            if (currentMusic != 0) playMusic(0, true); // Música de inicio en loop
        }
        else if (gameOver) {
            if (currentMusic != 2) playMusic(2, false); // Música de game over (sin loop)
        }
        else if (win) {
            if (currentMusic != 3) playMusic(3, false); // Música de victoria (sin loop)
        }
        else {
            if (currentMusic != 1) playMusic(1, true); // Música de juego en loop
        }

        // Actualizar sonido de aceleración
        bool accelerating = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        handleAccelerationSound(accelerating);


        // Limpiar fuentes de sonido de esquivación que ya terminaron
        for (auto it = dodgeSoundSources.begin(); it != dodgeSoundSources.end(); ) {
            ALint state;
            alGetSourcei(*it, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING) {
                alDeleteSources(1, &(*it));
                it = dodgeSoundSources.erase(it);
            }
            else {
                ++it;
            }
        }


        if (!gameStarted) {
            camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
            camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

            // PANTALLA DE INICIO SIMPLIFICADA
            // Texto simple en lugar de animación
            std::cout << "Presiona R para iniciar el juego" << std::endl;

            // ANIMACIÓN COMENTADA - Descomenta para usar la animación completa
            /*
            // Animación de StartScreen en pantalla de inicio
            startScreenAnimationTimer += deltaTime;
            float frameTime = 1.0f / startScreenFrameRate;
            if (startScreenAnimationTimer >= frameTime) {
                currentStartScreenFrame = (currentStartScreenFrame + 1) % 20;
                startScreenAnimationTimer = 0.0f;
            }

            // Dibujar frame actual de StartScreen
            if (startScreenFrames[currentStartScreenFrame] != nullptr) {
                glm::mat4 startScreenM = glm::mat4(1.0f);
                startScreenM = glm::translate(startScreenM, glm::vec3(0.0f, 0.0f, 0.0f));
                startScreenM = glm::scale(startScreenM, glm::vec3(0.5f));
                ourShader.setMat4("model", startScreenM);
                startScreenFrames[currentStartScreenFrame]->Draw(ourShader);
            }
            */

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
                gameStarted = true;
                rPressed = true;
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rPressed = false;

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }


        // PANTALLAS DE FINAL SIMPLIFICADAS
        if (gameOver || win) {
            gameOverTimer += deltaTime;

            // Reproducir sonido solo una vez al inicio
            if (gameOverTimer >= 0.1f && !endSoundPlayed) {
                playEndGameSound(win);
                endSoundPlayed = true;

                // Detener sonido de aceleración si está activo
                if (isAccelerating) {
                    alSourceStop(accelerationSoundSource);
                    isAccelerating = false;
                }
            }

            // Mostrar pantalla después del retraso (simplificada)
            if (gameOverTimer >= gameEndDelay) {
                camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
                camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

                // Mensajes simples en lugar de animaciones
                if (gameOver) {
                    std::cout << "GAME OVER - Presiona R para reiniciar" << std::endl;
                }
                else {
                    std::cout << "¡VICTORIA! - Presiona R para jugar de nuevo" << std::endl;
                }

                // ANIMACIONES COMENTADAS - Descomenta para usar las animaciones completas
                /*
                if (gameOver) {
                    // Animación de GameOver
                    gameOverAnimationTimer += deltaTime;
                    float frameTime = 1.0f / gameOverFrameRate;
                    if (gameOverAnimationTimer >= frameTime) {
                        currentGameOverFrame = (currentGameOverFrame + 1) % 20;
                        gameOverAnimationTimer = 0.0f;
                    }

                    // Dibujar frame actual de GameOver
                    if (gameOverFrames[currentGameOverFrame] != nullptr) {
                        glm::mat4 gameOverM = glm::mat4(1.0f);
                        gameOverM = glm::translate(gameOverM, glm::vec3(0.0f, 0.0f, 0.0f));
                        gameOverM = glm::scale(gameOverM, glm::vec3(0.5f));
                        ourShader.setMat4("model", gameOverM);
                        gameOverFrames[currentGameOverFrame]->Draw(ourShader);
                    }
                }
                else {
                    // Animación de Win
                    winAnimationTimer += deltaTime;
                    float frameTime = 1.0f / winFrameRate;
                    if (winAnimationTimer >= frameTime) {
                        currentWinFrame = (currentWinFrame + 1) % 20;
                        winAnimationTimer = 0.0f;
                    }

                    // Dibujar frame actual de Win
                    if (winFrames[currentWinFrame] != nullptr) {
                        glm::mat4 winM = glm::mat4(1.0f);
                        winM = glm::translate(winM, glm::vec3(0.0f, 0.0f, 0.0f));
                        winM = glm::scale(winM, glm::vec3(0.5f));
                        ourShader.setMat4("model", winM);
                        winFrames[currentWinFrame]->Draw(ourShader);
                    }
                }
                */
            }

            // Reiniciar juego
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed && gameOverTimer >= gameEndDelay) {
                gameOver = false;
                win = false;
                gameStarted = true;
                endSoundPlayed = false;
                carPosition = glm::vec3(0.0f);
                currentLane = 1;
                asteroids.clear();
                asteroidSpawnTimer = 0.0f;
                gameOverTimer = 0.0f;
                rPressed = true;

                // REINICIO DE ANIMACIONES COMENTADO - Descomenta si usas las animaciones
                /*
                // Reiniciar animación de StartScreen
                currentStartScreenFrame = 0;
                startScreenAnimationTimer = 0.0f;
                // Reiniciar animación de GameOver
                currentGameOverFrame = 0;
                gameOverAnimationTimer = 0.0f;
                // Reiniciar animación de Win
                currentWinFrame = 0;
                winAnimationTimer = 0.0f;
                */

                // Restaurar música de juego
                playMusic(1, true);
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
                rPressed = false;
            }

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

        glm::vec3 galactusPos = glm::vec3(0.0f, -96.0f, -90.0f); //coordenadas del galactus 

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

        // Dibujar ojitos de Galactus  
        glm::vec3 galactusEyesPos = galactusPos + glm::vec3(0.0f, 20.0f, 0.0f);

        glm::mat4 gEyes = glm::translate(gBody, glm::vec3(0.0f));
        headRotationAngle += headRotationSpeed * deltaTime;
        if (abs(headRotationAngle) >= headRotationLimit) {
            headRotationSpeed *= -1;
            headRotationAngle = glm::clamp(headRotationAngle, -headRotationLimit, headRotationLimit);
        }
        gEyes = glm::rotate(gEyes, glm::radians(headRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        ojitosShader.use();
        ojitosShader.setFloat("time", glfwGetTime());
        ojitosShader.setMat4("projection", projection);
        ojitosShader.setMat4("view", view);
        gEyes = glm::translate(gEyes, glm::vec3(0.001f, 0.0f, 0.0f));  // puedes ajustar el valor
        ojitosShader.setMat4("model", gEyes);
        galactusOjitosModel.Draw(ojitosShader);

        ourShader.use();

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

            flashlightShader.use();
            flashlightShader.setMat4("projection", projection);
            flashlightShader.setMat4("view", view);
            flashlightShader.setMat4("model", asteroidM);

            flashlightShader.setVec3("viewPos", camera.Position);
            flashlightShader.setVec3("light.position", carPosition + glm::vec3(0.0f, 0.2f, -0.5f)); // delante del carro
            flashlightShader.setVec3("light.direction", carDirection);
            flashlightShader.setFloat("light.cutOff", glm::cos(glm::radians(5.0f)));

            flashlightShader.setVec3("light.ambient", glm::vec3(0.1f));
            flashlightShader.setVec3("light.diffuse", glm::vec3(0.8f));
            flashlightShader.setVec3("light.specular", glm::vec3(1.0f));
            flashlightShader.setFloat("light.constant", 1.0f);
            flashlightShader.setFloat("light.linear", 0.09f);
            flashlightShader.setFloat("light.quadratic", 0.032f);

            asteroidModel.Draw(flashlightShader);
        }

        asteroids.erase(std::remove_if(asteroids.begin(), asteroids.end(),
            [](const Asteroid& a) { return a.position.z > 10.0f; }),
            asteroids.end());


        // Setup lights

        lightCubeShader.use();
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setMat4("projection", projection);

        glBindVertexArray(cubeVAO);
        for (int i = 0; i < NR_POINT_LIGHTS; i++) {
            glm::mat4 modelLight = glm::mat4(1.0f);
            modelLight = glm::translate(modelLight, pointLightPositions[i]);
            modelLight = glm::scale(modelLight, glm::vec3(2.5f)); // cubo más pequeño
            lightCubeShader.setMat4("model", modelLight);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // LIMPIEZA DE MEMORIA COMENTADA - Descomenta si usas las animaciones
    /*
    // Limpiar memoria de los frames de StartScreen
    for (Model* model : startScreenFrames) {
        if (model != nullptr) {
            delete model;
        }
    }
    startScreenFrames.clear();

    // Limpiar memoria de los frames de GameOver
    for (Model* model : gameOverFrames) {
        if (model != nullptr) {
            delete model;
        }
    }
    gameOverFrames.clear();

    // Limpiar memoria de los frames de Win
    for (Model* model : winFrames) {
        if (model != nullptr) {
            delete model;
        }
    }
    winFrames.clear();
    */

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
        playDodgeSound(); // Reproducir sonido al esquivar a la izquierda
        aPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) aPressed = false;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dPressed) {
        if (currentLane < 2) currentLane++;
        playDodgeSound(); // Reproducir sonido al esquivar a la derecha
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