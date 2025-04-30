#define GLFW_INCLUDE_NONE

#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_texture.hpp"
#include "nlohmann/json.hpp"
#include "draw_scene.hpp"
#include "vector.hpp"

#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace glbasimac;
using namespace STP3D;

/* Window properties */
static const unsigned int WINDOW_WIDTH = 1000;
static const unsigned int WINDOW_HEIGHT = 800;
static const char WINDOW_TITLE[] = "The Train";
static float aspectRatio = 1.0f;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1.0 / 30.0;

bool key_down = false;

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
        key_down = false;

    if (action == GLFW_PRESS || key_down)
    {
        key_down = true;
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

bool validSizeGridFormat(const nlohmann::json &data)
{
    return data.contains("size_grid") && data["size_grid"].is_number_integer();
}

bool validOriginFormat(const nlohmann::json &data)
{
    return data.contains("origin") && data["origin"].is_array() && data["origin"].size() == 2 && data["origin"][0].is_number_integer() && data["origin"][1].is_number_integer();
}

bool validPathFormat(const nlohmann::json &data)
{
    if (!data.contains("path") || !data["path"].is_array())
        return false;
    for (const auto &e : data["path"])
    {
        if (!e.is_array() || e.size() != 2 || !e[0].is_number_integer() || !e[1].is_number_integer())
            return false;
    }
    return true;
}

bool checkDataFormat(const nlohmann::json &data)
{
    if (!validSizeGridFormat(data) || !validOriginFormat(data) || !validPathFormat(data))
    {
        std::cerr << "ERROR: Invalid json file" << std::endl
                  << "size_grid: int" << std::endl
                  << "origin: [int, int]" << std::endl
                  << "path: [[int, int], ...]" << std::endl;
        return false;
    }
    return true;
}

bool checkGridSize(const nlohmann::json &data)
{
    if (data["size_grid"].get<int>() < 10)
    {
        std::cerr << "ERROR: Grid size must be at least 10" << std::endl;
        return false;
    }
    return true;
}

int manhattanDistance(const Vector2D &a, const Vector2D &b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool checkPath(const nlohmann::json &data)
{
    std::unordered_set<Vector2D, Vector2DHash> set;
    Vector2D prev;

    for (size_t i = 0; i < data["path"].size(); ++i)
    {
        const auto &e = data["path"][i];
        Vector2D current{e[0].get<int>(), e[1].get<int>()};

        if (set.count(current))
        {
            std::cerr << "ERROR: The path contains a duplicate (" << current.x << ", " << current.y << ")" << std::endl;
            return false;
        }
        set.insert(current);

        if (i > 0 && manhattanDistance(current, prev) != 1)
        {
            std::cerr << "ERROR: The current rail must be adjacent to the previous rail" << std::endl;
            return false;
        }
        prev = current;
    }
    return true;
}

bool checkData(const nlohmann::json &data)
{
    return checkDataFormat(data) && checkGridSize(data) && checkPath(data);
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
