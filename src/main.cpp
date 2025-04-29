#define GLFW_INCLUDE_NONE

#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_texture.hpp"
#include "nlohmann/json.hpp"
#include "../include/render.hpp"

#include <fstream>
#include <iostream>

using namespace glbasimac;
using namespace STP3D;

/* Window properties */
static const unsigned int WINDOW_WIDTH = 1000;
static const unsigned int WINDOW_HEIGHT = 800;
static const char WINDOW_TITLE[] = "The Train";
static float aspectRatio = 1.0f;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1. / 30.;

bool pressed = false;

void onError(int error, const char *description)
{
    std::cout << "GLFW Error (" << error << ") : " << description << std::endl;
}

void onWindowResized(GLFWwindow * /*window*/, int width, int height)
{
    aspectRatio = width / (float)height;

    glViewport(0, 0, width, height);
    std::cerr << "Setting 3D projection" << std::endl;
    myEngine.set3DProjection(60.0, aspectRatio, Z_NEAR, Z_FAR);
}

void onKey(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_RELEASE)
        pressed = false;

    if (action == GLFW_PRESS || pressed)
    {
        pressed = true;
        switch (key)
        {
        case GLFW_KEY_A:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_UP:
            angle_phy += 4.0f;
            break;
        case GLFW_KEY_DOWN:
            angle_phy -= 4.0f;
            break;
        case GLFW_KEY_LEFT:
            angle_theta += 4.0f;
            break;
        case GLFW_KEY_RIGHT:
            angle_theta -= 4.0f;
            break;
        case GLFW_KEY_Q:
            dist_zoom *= 1.1f;
            break;
        case GLFW_KEY_W:
            dist_zoom *= 0.9f;
            break;
        }
    }
}

void usage()
{
    std::cerr << "Usage: " << "./the_train filename.json" << std::endl;
}

bool checkGridSize(const nlohmann::json &data)
{
    return (int)data["size_grid"] >= 10;
}

bool checkData(const nlohmann::json &data)
{
    if (!checkGridSize(data))
    {
        std::cerr << "ERROR: Grid size must be at least 10" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    /* Open the file in read mode */
    std::ifstream file(argv[1]);
    if (!file)
    {
        std::cerr << "ERROR: Cannot open the file" << std::endl;
        return 1;
    }

    /* Load the json file */
    nlohmann::json data;
    file >> data;

    if (!checkData(data))
        return 1;

    /* GLFW initialisation */
    GLFWwindow *window;
    if (!glfwInit())
        return 1;

    /* Callback to a function if an error is rised by GLFW */
    glfwSetErrorCallback(onError);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window)
    {
        // If no context created : exit !
        glfwTerminate();
        return 1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    std::cout << "Loading GL extension" << std::endl;
    // Intialize glad (loads the OpenGL functions)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return 1;

    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetKeyCallback(window, onKey);

    std::cout << "Engine init" << std::endl;
    myEngine.mode2D = false; // Set engine to 3D mode
    myEngine.initGL();
    onWindowResized(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    CHECK_GL;

    initScene(data);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Get time (in second) at loop beginning */
        double startTime = glfwGetTime();

        /* Render here */
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        /* Fix camera position */
        myEngine.mvMatrixStack.loadIdentity();
        Vector3D pos_camera =
            Vector3D(dist_zoom * cos(deg2rad(angle_theta)) * cos(deg2rad(angle_phy)),
                     dist_zoom * sin(deg2rad(angle_theta)) * cos(deg2rad(angle_phy)),
                     dist_zoom * sin(deg2rad(angle_phy)));
        Vector3D viewed_point = Vector3D(0.0, 0.0, 0.0);
        Vector3D up_vector = Vector3D(0.0, 0.0, 1.0);
        Matrix4D viewMatrix = Matrix4D::lookAt(pos_camera, viewed_point, up_vector);
        myEngine.setViewMatrix(viewMatrix);
        myEngine.updateMvMatrix();

        renderScene();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        /* Elapsed time computation from loop begining */
        double elapsedTime = glfwGetTime() - startTime;
        /* If to few time is spend vs our wanted FPS, we wait */
        if (elapsedTime < FRAMERATE_IN_SECONDS)
            glfwWaitEventsTimeout(FRAMERATE_IN_SECONDS - elapsedTime);
    }

    glfwTerminate();

    return 0;
}
