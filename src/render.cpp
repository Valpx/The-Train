#include "../include/render.hpp"

/* Camera settings */
float angle_theta = 45.0f; // Angle between x axis and viewpoint
float angle_phy = 30.0f;   // Angle between z axis and viewpoint
float dist_zoom = 300.0f;  // Distance between origin and viewpoint

GLBI_Engine myEngine;
GLBI_Convex_2D_Shape ground{3};

void initGround(const nlohmann::json &data)
{
    float sizeGrid = (int)data["size_grid"] * CELL_SIZE;
    ground.initShape({-sizeGrid, -sizeGrid, 0.0f,
                      sizeGrid, -sizeGrid, 0.0f,
                      sizeGrid, sizeGrid, 0.0f,
                      -sizeGrid, sizeGrid, 0.0f});
    ground.changeNature(GL_TRIANGLE_FAN);
}

void initScene(const nlohmann::json &data)
{
    initGround(data);
}

void drawGround()
{
    myEngine.setFlatColor(0.2f, 0.0f, 0.0f);
    ground.drawShape();
}

void renderScene()
{
    drawGround();
}
