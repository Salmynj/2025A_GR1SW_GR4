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

unsigned int loadTexture(const char* path); //función para cargar las texturas 

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

// Texturas para las pantallas iniciales
unsigned int studioTexture, logoTexture;

// Sonidos iniciales
ALuint studioSoundBuffer;
ALuint logoSoundBuffer;
ALuint studioSoundSource;
ALuint logoSoundSource;

// Variables de control para las pantallas iniciales
bool showStudio = true;
bool showLogo = false;
float studioTimer = 0.0f;
float logoTimer = 0.0f;
float logoStartTime = 0.0f;
static bool studioSoundPlayed = false;

// Variables para modelos de pantallas iniciales
Model* studioModel = nullptr;
Model* logoModel = nullptr;


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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Herbie to the Rescue", NULL, NULL);
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

    Shader lightCubeShader("shaders/light_vs.vs", "shaders/light_fs.fs");
    Shader ourShader("shaders/loading_vs.vs", "shaders/loading_fs.fs");
    Shader ojitosShader("shaders/eye_vs.vs", "shaders/eye_fs.fs");
    Shader flashlightShader("shaders/shader_flashlight.vs", "shaders/shader_flashlight.fs");
    Shader flamaShader("shaders/shader_flama.vs", "shaders/shader_flama.fs");

    // Carga de modelos

    Model nebulaModel("models/nebula/nebula.obj");
    Model ourModel("models/FantastiCar+Herbie/FantastiCar+Herbie.obj");
    Model asteroidModel("models/asteroid/asteroid01.obj");
    Model galactusModel("models/Galactus/GalactusCuerpo.obj");
    Model galactusHeadModel("models/Galactus/GalactusCabeza.obj");
    Model galactusOjitosModel("models/ojitos/ojitos2.obj");
    Model flamesModel("models/flama/flamaAzul.obj");

    // Cargar modelos de pantallas iniciales
    try {
        studioModel = new Model("models/initialScreens/studio.obj");
        std::cout << "Modelo studio.obj cargado exitosamente" << std::endl;
    }
    catch (...) {
        std::cout << "Error cargando modelo studio.obj" << std::endl;
    }

    try {
        logoModel = new Model("models/initialScreens/logo.obj");
        std::cout << "Modelo logo.obj cargado exitosamente" << std::endl;
    }
    catch (...) {
        std::cout << "Error cargando modelo logo.obj" << std::endl;
    }


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

    // Cargar sonidos iniciales
    std::vector<char> studioSoundData, logoSoundData;
    ALenum studioFormat, logoFormat;
    ALsizei studioFreq, logoFreq;

    // Cargar sonido de studio (gato.wav)
    if (!LoadWavFile("audio/gato.wav", studioSoundData, studioFormat, studioFreq)) {
        std::cout << "❌ Error cargando sonido de studio (gato.wav)" << std::endl;
        studioSoundBuffer = 0;
    }
    else {
        alGenBuffers(1, &studioSoundBuffer);
        alBufferData(studioSoundBuffer, studioFormat, studioSoundData.data(),
            (ALsizei)studioSoundData.size(), studioFreq);

        // VERIFICAR ERRORES DE OPENAL
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cout << "❌ Error OpenAL al crear buffer: " << error << std::endl;
        }

        alGenSources(1, &studioSoundSource);
        alSourcef(studioSoundSource, AL_GAIN, 5.0f);

        error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cout << "❌ Error OpenAL al crear source: " << error << std::endl;
        }

        std::cout << "✅ Sonido studio (gato.wav) cargado exitosamente" << std::endl;
        std::cout << "   Formato: " << studioFormat << ", Frecuencia: " << studioFreq << std::endl;
        std::cout << "   Tamaño datos: " << studioSoundData.size() << " bytes" << std::endl;
    }

    // Cargar sonido de logo (intro.wav)
    if (!LoadWavFile("audio/intro.wav", logoSoundData, logoFormat, logoFreq)) {
        std::cout << "Error cargando sonido de logo (intro.wav)" << std::endl;
        logoSoundBuffer = 0;
    }
    else {
        alGenBuffers(1, &logoSoundBuffer);
        alBufferData(logoSoundBuffer, logoFormat, logoSoundData.data(),
            (ALsizei)logoSoundData.size(), logoFreq);
        alGenSources(1, &logoSoundSource);
        alSourcef(logoSoundSource, AL_GAIN, 0.2f); // Ajustar ganancia
        std::cout << "Sonido logo (intro.wav) cargado exitosamente" << std::endl;
    }


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

    // Inicializar el tiempo base
    float startTime = glfwGetTime();

    //Render loop ---------------------------------------------------------------------
    unsigned int azulTextureID = loadTexture("models/flama/Azul.png");
    unsigned int turquesaTextureID = loadTexture("models/flama/Turquesa.png");
    unsigned int celesteTextureID = loadTexture("models/flama/Celeste.png");



    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // 1. Pantalla Studio
        if (showStudio) {
            // Calcular timer relativo al inicio del programa
            float currentTime = glfwGetTime() - startTime;
            studioTimer = currentTime;
            std::cout << "Pantalla studio activa, timer: " << studioTimer << std::endl;

            // Configurar cámara para pantalla studio
            camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
            camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

            // Actualizar matrices de vista y proyección
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 200.0f);
            glm::mat4 view = camera.GetViewMatrix();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            // Reproducir sonido solo una vez al inicio
            if (studioTimer >= 0.1f && !studioSoundPlayed && studioSoundBuffer != 0) {
                std::cout << "🔊 Intentando reproducir sonido de studio..." << std::endl;

                alSourcei(studioSoundSource, AL_BUFFER, studioSoundBuffer);

                // Verificar estado antes de reproducir
                ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cout << "❌ Error antes de reproducir: " << error << std::endl;
                }

                alSourcePlay(studioSoundSource);

                error = alGetError();
                if (error != AL_NO_ERROR) {
                    std::cout << "❌ Error al reproducir: " << error << std::endl;
                }

                studioSoundPlayed = true;
                std::cout << "🔊 Comando de reproducción enviado" << std::endl;

                // Verificar si realmente se está reproduciendo
                ALint state;
                alGetSourcei(studioSoundSource, AL_SOURCE_STATE, &state);
                if (state == AL_PLAYING) {
                    std::cout << "✅ El sonido se está reproduciendo correctamente" << std::endl;
                }
                else {
                    std::cout << "❌ El sonido NO se está reproduciendo. Estado: " << state << std::endl;
                }
            }

            // Dibujar fondo nebula
            glm::mat4 nebulaM = glm::mat4(1.0f);
            nebulaM = glm::translate(nebulaM, glm::vec3(0.0f, 0.0f, 0.0f));
            nebulaM = glm::scale(nebulaM, glm::vec3(150.0f, 150.0f, 150.0f));
            ourShader.setMat4("model", nebulaM);
            nebulaModel.Draw(ourShader);

            // Dibujar pantalla studio
            if (studioModel != nullptr) {
                std::cout << "Dibujando modelo studio" << std::endl;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.5f));
                ourShader.setMat4("model", model);
                studioModel->Draw(ourShader);
            }
            else {
                std::cout << "studioModel es nullptr!" << std::endl;
            }

            // Pasar a pantalla logo después de 10 segundos
            if (studioTimer >= 10.0f) {
                showStudio = false;
                showLogo = true;
                logoStartTime = glfwGetTime(); // Guardar cuando empezó logo
                logoTimer = 0.0f;
                std::cout << "Cambiando a pantalla logo" << std::endl;
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue; // IMPORTANTE: Saltar el resto del bucle
        }

        // 2. Pantalla Logo
        if (showLogo) {
            // Calcular timer relativo al inicio de la pantalla logo
            logoTimer = glfwGetTime() - logoStartTime;
            std::cout << "Debug - showStudio=true, studioTimer=" << studioTimer
                << ", studioSoundBuffer=" << studioSoundBuffer
                << ", studioSoundPlayed=" << studioSoundPlayed << std::endl;
            // Configurar cámara para pantalla logo
            camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
            camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

            // Actualizar matrices de vista y proyección
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 200.0f);
            glm::mat4 view = camera.GetViewMatrix();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            // Reproducir sonido solo al inicio
            if (logoTimer >= 0.1f && logoTimer <= 0.2f && logoSoundBuffer != 0) {
                alSourcei(logoSoundSource, AL_BUFFER, logoSoundBuffer);
                alSourcePlay(logoSoundSource);
                std::cout << "Reproduciendo sonido de logo" << std::endl;
            }

            // Dibujar fondo nebula
            glm::mat4 nebulaM = glm::mat4(1.0f);
            nebulaM = glm::translate(nebulaM, glm::vec3(0.0f, 0.0f, 0.0f));
            nebulaM = glm::scale(nebulaM, glm::vec3(150.0f, 150.0f, 150.0f));
            ourShader.setMat4("model", nebulaM);
            nebulaModel.Draw(ourShader);

            // Dibujar pantalla logo
            if (logoModel != nullptr) {
                std::cout << "Dibujando modelo logo" << std::endl;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.5f));
                ourShader.setMat4("model", model);
                logoModel->Draw(ourShader);
            }
            else {
                std::cout << "logoModel es nullptr!" << std::endl;
            }

            // Pasar a StartScreen después de 25 segundos
            if (logoTimer >= 25.0f) {
                showLogo = false;
                logoTimer = 0.0f;
                std::cout << "Cambiando a StartScreen" << std::endl;
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue; // IMPORTANTE: Saltar el resto del bucle
        }



        ojitosShader.setFloat("time", glfwGetTime());

        // Ahora dibuja la nebula para el resto de estados del juego
        glm::mat4 nebulaM = glm::mat4(1.0f);
        nebulaM = glm::translate(nebulaM, glm::vec3(0.0f, 0.0f, 0.0f));
        nebulaM = glm::scale(nebulaM, glm::vec3(150.0f, 150.0f, 150.0f));
        ourShader.setMat4("model", nebulaM);
        nebulaModel.Draw(ourShader);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);



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

            // Mostrar pantalla después del retraso 
            if (gameOverTimer >= gameEndDelay) {
                camera.Position = glm::vec3(0.0f, 0.75f, 1.3f);
                camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));



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


                // Reiniciar animación de StartScreen
                currentStartScreenFrame = 0;
                startScreenAnimationTimer = 0.0f;
                // Reiniciar animación de GameOver
                currentGameOverFrame = 0;
                gameOverAnimationTimer = 0.0f;
                // Reiniciar animación de Win
                currentWinFrame = 0;
                winAnimationTimer = 0.0f;


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
            if (isAccelerating) {
                // TRANSFORMACIÓN DEL MODELO DE LLAMAS (al frente del carro)
                glm::vec3 flameOffset = glm::vec3(0.0f, 0.0f, 0.00f);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
                    glm::radians(carYaw + 180.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
                glm::vec3 flamePos = carPosition + glm::vec3(rotation * glm::vec4(flameOffset, 1.0f));

                glm::mat4 flameM = glm::mat4(1.0f);
                flameM = glm::translate(flameM, flamePos);
                flameM = glm::rotate(flameM, glm::radians(carYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                flameM = glm::scale(flameM, glm::vec3(0.1f));

                flamaShader.use();
                flamaShader.setMat4("projection", projection);
                flamaShader.setMat4("view", view);
                flamaShader.setMat4("model", flameM);
                flamaShader.setFloat("time", glfwGetTime());

                // DIBUJAR LLAMAS CON EL SHADER CORRECTO


                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, azulTextureID); // Azul.png
                flamaShader.setInt("glowTexture", 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, turquesaTextureID); // Turquesa.png
                flamaShader.setInt("highlightTexture", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, celesteTextureID); // Celeste.png
                flamaShader.setInt("baseTexture", 2);

                ourShader.use();
                ourShader.setMat4("model", flameM);
                flamesModel.Draw(ourShader);

                // Restaurar shader principal
                ourShader.use();
            }

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
            flashlightShader.setFloat("light.cutOff", glm::cos(glm::radians(5.0f))); // Se puede regular el tamaño de la bolita

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

    // Limpiar recursos de audio
    if (studioSoundBuffer != 0) {
        alDeleteBuffers(1, &studioSoundBuffer);
        alDeleteSources(1, &studioSoundSource);
    }
    if (logoSoundBuffer != 0) {
        alDeleteBuffers(1, &logoSoundBuffer);
        alDeleteSources(1, &logoSoundSource);
    }

    // Limpiar modelos de pantallas iniciales
    if (studioModel != nullptr) {
        delete studioModel;
    }
    if (logoModel != nullptr) {
        delete logoModel;
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
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
            format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // o GL_CLAMP_TO_EDGE si prefieres
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "❌ Error al cargar textura en la ruta: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}