#include "../include/render.hpp"

/* Camera settings */
float angle_theta{45.0}; // Angle between x axis and viewpoint
float angle_phy{30.0};   // Angle between z axis and viewpoint
float dist_zoom{300.0};  // Distance between origin and viewpoint

GLBI_Engine myEngine;
GLBI_Convex_2D_Shape ground{3};

void initGround()
{
    ground.initShape({-100.0, -100.0, 0.0,
                      100.0, -100.0, 0.0,
                      100.0, 100.0, 0.0,
                      -100.0, 100.0, 0.0});
    ground.changeNature(GL_TRIANGLE_FAN);
}

void initScene()
{
    initGround();
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
