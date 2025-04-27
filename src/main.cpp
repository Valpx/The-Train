#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_texture.hpp"
#include "../include/render.hpp"

#include <iostream>

using namespace glbasimac;
using namespace STP3D;

/* Window properties */
static const unsigned int WINDOW_WIDTH = 800;
static const unsigned int WINDOW_HEIGHT = 800;
static const char WINDOW_TITLE[] = "The Train";
static float aspectRatio = 1.0f;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1. / 30.;

bool pressed = false;

/* Error handling function */
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

void onMouseButton(GLFWwindow *window, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        std::cout << "Pressed in " << xpos << " " << ypos << std::endl;
    }
}

int main(int argc, char **argv)
{
    /* GLFW initialisation */
    GLFWwindow *window;
    if (!glfwInit())
        return -1;

    /* Callback to a function if an error is rised by GLFW */
    glfwSetErrorCallback(onError);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!window)
    {
        // If no context created : exit !
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    std::cout << "Loading GL extension" << std::endl;
    // Intialize glad (loads the OpenGL functions)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    glfwSetWindowSizeCallback(window, onWindowResized);
    glfwSetKeyCallback(window, onKey);
    glfwSetMouseButtonCallback(window, onMouseButton);

    std::cout << "Engine init" << std::endl;
    myEngine.mode2D = false; // Set engine to 3D mode
    myEngine.initGL();
    onWindowResized(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    CHECK_GL;

    initScene();

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
