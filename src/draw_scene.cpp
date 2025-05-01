#include <cmath>

#include "draw_scene.hpp"
#include "vector2d.hpp"

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
GLBI_Convex_2D_Shape iternalCurvedRail{3};
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

void initInternalCurvedRail()
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

    iternalCurvedRail.initShape(in_coord);
    iternalCurvedRail.changeNature(GL_TRIANGLES);
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
    initInternalCurvedRail();
    initExternalCurvedRail();
}

void drawGround()
{
    myEngine.setFlatColor(0.2f, 0.0f, 0.0f);
    ground.drawShape();
}

void drawStraightTrack()
{
    /* Rails */
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

    /* Balasts */
    myEngine.setFlatColor(0.4f, 0.2f, 0.0f);

    const float SX = (CELL_SIZE - (RR * 2.0f) * STRAIGHT_TRACK_BALLAST_COUNT) / 10.0f;
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, BALLAST_X_START, RR});
    myEngine.updateMvMatrix();
    for (auto i = 0; i < STRAIGHT_TRACK_BALLAST_COUNT; i++)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{-(SX + RR) * (i == 0 ? 1.0f : 2.0f), 0.0f, 0.0f});
        myEngine.updateMvMatrix();
        ballast->draw();
    }
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void drawCurvedTrack()
{
    /* Rails */
    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, 0.0f, RR * 2.0f});
    myEngine.updateMvMatrix();
    iternalCurvedRail.drawShape();
    externalCurvedRail.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Balasts */
    myEngine.setFlatColor(0.4f, 0.2f, 0.0f);

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{BALLAST_X_START * std::cos(5.0f * M_PI / 12.0f), BALLAST_X_START * std::sin(5.0f * M_PI / 12.0f), RR});
    myEngine.mvMatrixStack.addRotation(M_PI / 12.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    ballast->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{BALLAST_X_START * std::cos(3.0f * M_PI / 12.0f), BALLAST_X_START * std::sin(3.0f * M_PI / 12.0f), RR});
    myEngine.mvMatrixStack.addRotation(3.0f * M_PI / 12.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    ballast->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{BALLAST_X_START * std::cos(M_PI / 12.0f), BALLAST_X_START * std::sin(M_PI / 12.0f), RR});
    myEngine.mvMatrixStack.addRotation(5.0f * M_PI / 12.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    ballast->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

bool isCorner(const Vector2D &prev, const Vector2D &current, const Vector2D &next)
{
    auto A = current - prev;
    auto B = next - current;
    return (A.x * B.y - A.y * B.x) != 0;
}

void rotateStraightTrack(const Vector2D &current, const Vector2D &other)
{
    if (other.x != current.x)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
}

void rotateCurvedTrack(const Vector2D &prev, const Vector2D &current, const Vector2D &next)
{
    /*
     |
    -+
    */
    if ((prev.x == current.x - 1 || next.x == current.x - 1) && (prev.y == current.y + 1 || next.y == current.y + 1))
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    /*
    +-
    |
    */
    else if ((prev.y == current.y - 1 || next.y == current.y - 1) && (prev.x == current.x + 1 || next.x == current.x + 1))
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE, 0.0f, 0.0f});
        myEngine.mvMatrixStack.addRotation(3.0f * M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    else if ((prev.y == current.y + 1 || next.y == current.y + 1) && (prev.x == current.x + 1 || next.x == current.x + 1))
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI, Vector3D{0.0f, 0.0f, 1.0f});
        myEngine.updateMvMatrix();
    }
}

void drawTracks(const nlohmann::json &data)
{
    auto path = data["path"].get<std::vector<std::vector<int>>>();
    for (auto i = 0; i < path.size(); i++)
    {
        auto current = Vector2D{path[i]};
        myEngine.mvMatrixStack.pushMatrix();
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE * current.x, CELL_SIZE * current.y, 0.0f});
        myEngine.updateMvMatrix();

        if (path.size() == 1)
            drawStraightTrack();
        else if (path.size() == 2)
        {
            auto other = Vector2D{path[i == 0 ? 1 : 0]};
            rotateStraightTrack(current, other);
            drawStraightTrack();
        }
        else
        {
            auto prev = Vector2D{path[i == 0 ? path.size() - 1 : i - 1]};
            auto next = Vector2D{path[i == path.size() - 1 ? 0 : i + 1]};
            if (current.isNeighbor(prev) && current.isNeighbor(next) && isCorner(prev, current, next))
            {
                rotateCurvedTrack(prev, current, next);
                drawCurvedTrack();
            }
            else
            {
                rotateStraightTrack(current, current.isNeighbor(prev) ? prev : next);
                drawStraightTrack();
            }
        }

        myEngine.mvMatrixStack.popMatrix();
        myEngine.updateMvMatrix();
    }
}

void renderScene(const nlohmann::json &data)
{
    drawGround();
    drawTracks(data);
}
