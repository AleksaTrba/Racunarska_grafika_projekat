#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const * path);
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool shadows=false;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirectionLight{
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

bool shadowsKeyPressed=false;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
       // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);



    // build and compile shaders
    // -------------------------

    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skybox_shader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");
    Shader shadow_point("resources/shaders/shadow.vs","resources/shaders/shadow.fs","resources/shaders/geometryshader.gs");
    Shader floor("resources/shaders/grass.vs","resources/shaders/grass.fs");


    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



    //plane

    float planeVertices[] = {
            // positions          // texture Coords
            5.0f, 0.001f,  5.0f,  2.0f, 0.0f,
            -5.0f, 0.001f,  5.0f,  0.0f, 0.0f,
            -5.0f, 0.001f, -5.0f,  0.0f, 2.0f,

            5.0f, 0.001f,  5.0f,  2.0f, 0.0f,
            -5.0f, 0.001f, -5.0f,  0.0f, 2.0f,
            5.0f, 0.001f, -5.0f,  2.0f, 2.0f
    };

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));



    //end plane

    //trava
    //TODO izracunaj normale
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  -0.5f,  0.0f,  0.0f,  0.0f,0.0,0.0,1.0,
            0.0f, 0.5f,  0.0f,  0.0f,  1.0f,0.0,0.0,1.0,
            1.0f, 0.5f,  0.0f,  1.0f,  1.0f,0.0,0.0,1.0,

            0.0f,  -0.5f,  0.0f,  0.0f,  0.0f,0.0,0.0,1.0,
            1.0f, 0.5f,  0.0f,  1.0f,  1.0f,0.0,0.0,1.0,
            1.0f,  -0.5f,  0.0f,  1.0f,  0.0f,0.0,0.0,1.0
    };
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8 * sizeof(float),(void*)(5*sizeof(float)));
    glBindVertexArray(0);

    //end trava



    // load models
    // -----------


    Model ourModel("resources/objects/camp_fire/Campfire OBJ.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model planina("resources/objects/mountain/mount.blend1.obj");
    planina.SetShaderTextureNamePrefix("material.");

    //ovde se priprema deapthmap


    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //ovde se zavrsava




    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 1.0);
    pointLight.ambient = glm::vec3(0.5,0.4,0.4);
    pointLight.diffuse = glm::vec3(0.5,0.4,0.4);
    pointLight.specular = glm::vec3(0.5,0.4,0.4);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirectionLight dirlight;
    dirlight.direction=glm::vec3(0.0,25.0,0.0);
    dirlight.ambient=glm::vec3 (0.05);
    dirlight.diffuse=glm::vec3 (0.05);
    dirlight.specular=glm::vec3(0.05);



    vector<glm::vec3> transacije{
        glm::vec3(10.0,0.5,0.0),
        glm::vec3(-9.0,0.5,0.0),
        glm::vec3(-11.0,0.5,10.0),
        glm::vec3(-5.0,0.5,-12.0),
        glm::vec3(0.0,0.5,14.0),
        glm::vec3(5.0,0.5,-7.0),
        glm::vec3(6.0,0.5,8.0),
        glm::vec3(4.0,0.5,13.5),
        glm::vec3(10.5,0.5,-14.5),
        glm::vec3(-10.5,0.5,-12.5),
        glm::vec3(-15.5,0.5,-16.5),
        glm::vec3(17.5,0.5,-12.5)
    };


    vector<std::string> faces
    {
        FileSystem::getPath("resources/textures/skybox/skybox_left.png"),
        FileSystem::getPath("resources/textures/skybox/skybox_right.png"),
        FileSystem::getPath("resources/textures/skybox/skybox_up.png"),
        FileSystem::getPath("resources/textures/skybox/skybox_down.png"),
        FileSystem::getPath("resources/textures/skybox/skybox_front.png"),
        FileSystem::getPath("resources/textures/skybox/skybox_back.png")
    };

    unsigned int skybox=loadCubemap(faces);

    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/objects/camp_fire/grass.jpg").c_str());

    unsigned int grassTexture= loadTexture("resources/textures/grass.png");

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    skybox_shader.use();
    skybox_shader.setInt("skybox",0);

    ourShader.use();
    ourShader.setInt("depthMap",15);

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        pointLight.position = glm::vec3(1.5,0.1,0.2);//glm::vec3(4.0 * cos(currentFrame), 4.0f,
                                                                 //  4.0 * sin(currentFrame));
        pointLight.ambient = glm::vec3(0.25*(cos(currentFrame)+1),0.2*(cos(currentFrame)+1),0.2*(cos(currentFrame)+1));
        pointLight.diffuse = glm::vec3(0.5,0.4,0.4);
        pointLight.specular = glm::vec3(0.5,0.4,0.4);

        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model,programState->backpackPosition);
        model = glm::scale(model, glm::vec3(0.1/*programState->backpackScale*/));
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();






        // deapth cubemap
        float near_plane = 0.1f;
        float far_plane = 100.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float) SHADOW_WIDTH / (float) SHADOW_HEIGHT,
                                                near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        glm::vec3 lightPos = pointLight.position;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f),
                                                            glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f),
                                                            glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f)));
//render to cubemap

        glm::mat4 pomocna_model_matrica = model;
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadow_point.use();
        for (unsigned int i = 0; i < 6; ++i)
            shadow_point.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        shadow_point.setFloat("far_plane", far_plane);
        shadow_point.setVec3("lightPos", lightPos);
        shadow_point.setMat4("model", model);
        //shadow_point.setVec3("lightPos", lightPos/glm::vec3(0.1));
        glDisable(GL_CULL_FACE);
        ourModel.Draw(shadow_point);


        model = glm::mat4(1.0f);//glm::scale(model, glm::vec3(10.0));
        shadow_point.use();
        shadow_point.setVec3("lightPos", lightPos/glm::vec3(0.1));
        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);

        shadow_point.use();
        model = glm::translate(model, glm::vec3(0.0, 0.0, 3.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);

        shadow_point.use();

        model = glm::translate(model, glm::vec3(-2.5, 0.0, 0.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);
        shadow_point.use();
        model = glm::translate(model, glm::vec3(-2.5, 0.0, 0.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);
        shadow_point.use();
        model = glm::translate(model, glm::vec3(0.0, 0.0, -3.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);
        shadow_point.use();
        model = glm::translate(model, glm::vec3(0.0, 0.0, -3.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);
        shadow_point.use();
        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);
        shadow_point.use();
        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        shadow_point.setMat4("model", model);
        planina.Draw(shadow_point);

        glBindVertexArray(planeVAO);
        shadow_point.use();
        shadow_point.setMat4("model", pomocna_model_matrica);
        shadow_point.setVec3("lightPos",lightPos);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        model = pomocna_model_matrica;

        // render
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("dirlight.direction", dirlight.direction);
        ourShader.setVec3("dirlight.ambient", dirlight.ambient);
        ourShader.setVec3("dirlight.diffuse", dirlight.diffuse);
        ourShader.setVec3("dirlight.specular", dirlight.specular);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("far_plane", far_plane);
        ourShader.setFloat("material.shininess", 8.0f);
        // view/projection transformations

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setInt("shadows", shadows);

        // render the loaded model

        ourShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE15);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        ourModel.Draw(ourShader);
        glActiveTexture(GL_TEXTURE15);




        //planine

        model =glm::mat4(1.0f); //glm::scale(model, glm::vec3(10.0));

        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(0.0, 0.0, 3.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(-2.5, 0.0, 0.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(-2.5, 0.0, 0.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(0.0, 0.0, -3.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(0.0, 0.0, -3.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);

        model = glm::translate(model, glm::vec3(2.5, 0.0, 0.0));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        planina.Draw(ourShader);


//pod
        glCullFace(GL_FRONT);
        glBindVertexArray(planeVAO);
        //model1=glm::mat4(1.0);
        // model=glm::translate(model,glm::vec3(0.0,10,0.0));
        ourShader.use();
        ourShader.setMat4("model", pomocna_model_matrica);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        ourShader.setInt("material.texture_diffuse1",0);
         /*

          floor.setVec3("dirlight.direction", dirlight.direction);
          floor.setVec3("dirlight.ambient", dirlight.ambient);
          floor.setVec3("dirlight.diffuse", dirlight.diffuse);
          floor.setVec3("dirlight.specular", dirlight.specular);*/
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        //model=glm::translate(model,glm::vec3(0.0,-10,0.0));
        //glBindTexture(GL_TEXTURE_2D,0);
        //glActiveTexture(GL_TEXTURE0);
        glCullFace(GL_BACK);

//kraj poda

//trava
        glDisable(GL_CULL_FACE);
        floor.use();
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        floor.setVec3("pointLight.position", pointLight.position);
        floor.setVec3("pointLight.ambient", pointLight.ambient);
        floor.setVec3("pointLight.diffuse", pointLight.diffuse);
        floor.setVec3("pointLight.specular", pointLight.specular);
        floor.setFloat("pointLight.constant", pointLight.constant);
        floor.setFloat("pointLight.linear", pointLight.linear);
        floor.setFloat("pointLight.quadratic", pointLight.quadratic);
        floor.setVec3("viewPosition", programState->camera.Position);
        glm::mat4 trava_model;
        for (unsigned int i = 0; i < transacije.size(); i++) {
            trava_model = glm::mat4(1.0);
            trava_model = glm::scale(trava_model, glm::vec3(0.1));
            trava_model = glm::translate(trava_model, transacije[i]);
            if(i%2==0){
                trava_model=glm::rotate(trava_model,glm::radians(15.0f*(i%12)),glm::vec3(0.0,1.0,0.0));
            }
            floor.setMat4("model", trava_model);
            floor.setMat4("view", view);
            floor.setMat4("projection", projection);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        glEnable(GL_CULL_FACE);
//kraj trave


        //skybox
        glDepthFunc(GL_LEQUAL);
        skybox_shader.use();
        skybox_shader.setMat4("view",glm::mat4(glm::mat3(view)));
        skybox_shader.setMat4("projection",projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    stbi_set_flip_vertically_on_load(true);
    return textureID;
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
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
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

