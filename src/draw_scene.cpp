#include <cmath>

#include "../include/draw_scene.hpp"

/* Camera settings */
float angle_theta = -90.0f; // Angle between x axis and viewpoint
float angle_phy = 30.0f;    // Angle between z axis and viewpoint
float dist_zoom = 25.0f;    // Distance between origin and viewpoint

/* Ground */
static const float CELL_SIZE = 10.0f;
GLBI_Convex_2D_Shape ground{3};

/* Rail settings */
static const float SR = 0.5f;
static const float POS_X_RAIL1 = 3.0f;
static const float POS_X_RAIL2 = 7.0f;

/* Straight rail */
static const int STRAIGHT_TRACK_BALLAST_COUNT = 5;
IndexedMesh *straightRail = NULL;

/* Curved rail */
static const int CURVED_TRACK_BALLAST_COUNT = 3;
GLBI_Convex_2D_Shape innerCurvedRail{3};
GLBI_Convex_2D_Shape externalCurvedRail{3};

/* Ballast */
static const float RR = 0.25f;
static const float BALLAST_X_START = 2.0f;
static const float BALLAST_X_END = 8.0f;
IndexedMesh *ballast = NULL;

GLBI_Engine myEngine;

void initGround(const nlohmann::json &data)
{
    float sizeGrid = data["size_grid"].get<int>() * CELL_SIZE;
    ground.initShape({0.0f, 0.0f, 0.0f,
                      sizeGrid, 0.0f, 0.0f,
                      0.0f, sizeGrid, 0.0f,
                      sizeGrid, sizeGrid, 0.0f});
    ground.changeNature(GL_TRIANGLE_STRIP);
}

void initStraightRail()
{
    const unsigned int vertex_number = 8;
    std::vector<float> vertex_coord = {
        0.0f, 0.0f, 0.0f,      // v0
        SR, 0.0f, 0.0f,        // v1
        SR, CELL_SIZE, 0.0f,   // v2
        0.0f, CELL_SIZE, 0.0f, // v3
        0.0f, 0.0f, SR,        // v4
        0.0f, CELL_SIZE, SR,   // v5
        SR, CELL_SIZE, SR,     // v6
        SR, 0.0f, SR           // v7
    };

    const unsigned int triangle_number = 12;
    std::vector<unsigned int> triangle_index = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 5,
        0, 4, 5,
        2, 3, 6,
        3, 5, 6,
        0, 1, 7,
        0, 4, 7,
        1, 2, 7,
        2, 6, 7,
        4, 5, 7,
        5, 6, 7};

    straightRail = new IndexedMesh(triangle_number, vertex_number);
    straightRail->addOneBuffer(0, 3, vertex_coord.data(), "Coord", true);
    straightRail->addIndexBuffer(triangle_index.data(), true);
    straightRail->createVAO();
}

void add_triangle(std::vector<float> &in_coord, Vector3D a, Vector3D b, Vector3D c)
{
    in_coord.emplace_back(a.x);
    in_coord.emplace_back(a.y);
    in_coord.emplace_back(a.z);

    in_coord.emplace_back(b.x);
    in_coord.emplace_back(b.y);
    in_coord.emplace_back(b.z);

    in_coord.emplace_back(c.x);
    in_coord.emplace_back(c.y);
    in_coord.emplace_back(c.z);
}

void initInternCurvedRail()
{
    const int subdivision = 10;
    std::vector<float> in_coord{};

    for (auto i = 0; i < subdivision; i++)
    {
        float angle1 = (M_PI / 2.0f) * i / subdivision;
        float angle2 = (M_PI / 2.0f) * (i + 1) / subdivision;

        /* Left face */
        float x1 = (POS_X_RAIL1 - SR / 2.0f) * std::cos(angle1);
        float y1 = (POS_X_RAIL1 - SR / 2.0f) * std::sin(angle1);

        float x2 = (POS_X_RAIL1 - SR / 2.0f) * std::cos(angle2);
        float y2 = (POS_X_RAIL1 - SR / 2.0f) * std::sin(angle2);

        add_triangle(in_coord, {x1, y1, 0.0f}, {x1, y1, SR}, {x2, y2, 0.0f});
        add_triangle(in_coord, {x1, y1, SR}, {x2, y2, SR}, {x2, y2, 0.0f});

        /* Right face */
        float x3 = (POS_X_RAIL1 + SR / 2.0f) * std::cos(angle1);
        float y3 = (POS_X_RAIL1 + SR / 2.0f) * std::sin(angle1);

        float x4 = (POS_X_RAIL1 + SR / 2.0f) * std::cos(angle2);
        float y4 = (POS_X_RAIL1 + SR / 2.0f) * std::sin(angle2);

        add_triangle(in_coord, {x3, y3, 0.0f}, {x3, y3, SR}, {x4, y4, 0.0f});
        add_triangle(in_coord, {x3, y3, SR}, {x4, y4, SR}, {x4, y4, 0.0f});

        /* Bottom face */
        add_triangle(in_coord, {x1, y1, 0.0f}, {x3, y3, 0.0f}, {x2, y2, 0.0f});
        add_triangle(in_coord, {x3, y3, 0.0f}, {x4, y4, 0.0f}, {x2, y2, 0.0f});

        /* Top face */
        add_triangle(in_coord, {x1, y1, SR}, {x3, y3, SR}, {x2, y2, SR});
        add_triangle(in_coord, {x3, y3, SR}, {x4, y4, SR}, {x2, y2, SR});

        /* Front face */
        if (i == 0)
        {
            add_triangle(in_coord, {x1, y1, 0.0f}, {x1, y1, SR}, {x3, y3, 0.0f});
            add_triangle(in_coord, {x3, y3, 0.0f}, {x1, y1, SR}, {x3, y3, SR});
        }

        /* Back face */
        if (i == subdivision - 1)
        {
            add_triangle(in_coord, {x2, y2, 0.0f}, {x2, y2, SR}, {x4, y4, 0.0f});
            add_triangle(in_coord, {x4, y4, 0.0f}, {x2, y2, SR}, {x4, y4, SR});
        }
    }

    innerCurvedRail.initShape(in_coord);
    innerCurvedRail.changeNature(GL_TRIANGLES);
}

void initExternalCurvedRail()
{
    const int subdivision = 10;
    std::vector<float> in_coord{};

    for (auto i = 0; i < subdivision; i++)
    {
        float angle1 = (M_PI / 2.0f) * i / subdivision;
        float angle2 = (M_PI / 2.0f) * (i + 1) / subdivision;

        /* Left face */
        float x1 = (POS_X_RAIL2 - SR / 2.0f) * std::cos(angle1);
        float y1 = (POS_X_RAIL2 - SR / 2.0f) * std::sin(angle1);

        float x2 = (POS_X_RAIL2 - SR / 2.0f) * std::cos(angle2);
        float y2 = (POS_X_RAIL2 - SR / 2.0f) * std::sin(angle2);

        add_triangle(in_coord, {x1, y1, 0.0f}, {x1, y1, SR}, {x2, y2, 0.0f});
        add_triangle(in_coord, {x1, y1, SR}, {x2, y2, SR}, {x2, y2, 0.0f});

        /* Right face */
        float x3 = (POS_X_RAIL2 + SR / 2.0f) * std::cos(angle1);
        float y3 = (POS_X_RAIL2 + SR / 2.0f) * std::sin(angle1);

        float x4 = (POS_X_RAIL2 + SR / 2.0f) * std::cos(angle2);
        float y4 = (POS_X_RAIL2 + SR / 2.0f) * std::sin(angle2);

        add_triangle(in_coord, {x3, y3, 0.0f}, {x3, y3, SR}, {x4, y4, 0.0f});
        add_triangle(in_coord, {x3, y3, SR}, {x4, y4, SR}, {x4, y4, 0.0f});

        /* Bottom face */
        add_triangle(in_coord, {x1, y1, 0.0f}, {x3, y3, 0.0f}, {x2, y2, 0.0f});
        add_triangle(in_coord, {x3, y3, 0.0f}, {x4, y4, 0.0f}, {x2, y2, 0.0f});

        /* Top face */
        add_triangle(in_coord, {x1, y1, SR}, {x3, y3, SR}, {x2, y2, SR});
        add_triangle(in_coord, {x3, y3, SR}, {x4, y4, SR}, {x2, y2, SR});

        /* Front face */
        if (i == 0)
        {
            add_triangle(in_coord, {x1, y1, 0.0f}, {x1, y1, SR}, {x3, y3, 0.0f});
            add_triangle(in_coord, {x3, y3, 0.0f}, {x1, y1, SR}, {x3, y3, SR});
        }

        /* Back face */
        if (i == subdivision - 1)
        {
            add_triangle(in_coord, {x2, y2, 0.0f}, {x2, y2, SR}, {x4, y4, 0.0f});
            add_triangle(in_coord, {x4, y4, 0.0f}, {x2, y2, SR}, {x4, y4, SR});
        }
    }

    externalCurvedRail.initShape(in_coord);
    externalCurvedRail.changeNature(GL_TRIANGLES);
}
void initBallast()
{
    ballast = basicCylinder(BALLAST_X_END - BALLAST_X_START, RR);
    ballast->createVAO();
}

void initScene(const nlohmann::json &data)
{
    initGround(data);
    initStraightRail();
    initBallast();
    initInternCurvedRail();
    initExternalCurvedRail();
}

void drawGround()
{
    myEngine.setFlatColor(0.2f, 0.0f, 0.0f);
    ground.drawShape();
}

void drawStraightTrack()
{
    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL1 - (SR / 2.0f), 0.0f, RR * 2.0f});
    myEngine.updateMvMatrix();
    straightRail->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL2 - (SR / 2.0f), 0.0f, RR * 2.0f});
    myEngine.updateMvMatrix();
    straightRail->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.setFlatColor(0.4f, 0.2f, 0.0f);

    const float SX = (CELL_SIZE - (RR * 2.0f) * STRAIGHT_TRACK_BALLAST_COUNT) / 10.0f;
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.mvMatrixStack.addTranslation(Vector3D{-(SX + RR), BALLAST_X_START, RR});
    myEngine.updateMvMatrix();
    ballast->draw();
    for (auto i = 0; i < 4; i++)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{-(SX + RR) * 2.0f, 0.0f, 0.0f});
        myEngine.updateMvMatrix();
        ballast->draw();
    }
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void drawCurvedRail()
{
    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);

    innerCurvedRail.drawShape();
    externalCurvedRail.drawShape();
}

void renderScene()
{
    drawGround();
    // drawStraightTrack();
    drawCurvedRail();
}
