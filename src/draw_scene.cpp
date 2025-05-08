#include <cmath>

#include <functional>
#include "draw_scene.hpp"
#include "vector2d.hpp"
#include "glbasimac/glbi_texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "tools/stb_image.h"
#include <utility>
#include <algorithm>
#include <random>

/* Camera */
Vector3D camera_pos;
Vector3D camera_front;
Vector3D camera_up{0.0f, 0.0f, 1.0f};

/* Ground */
static const float CELL_SIZE = 10.0f;
StandardMesh *ground = NULL;

/* Rail settings */
static const float SR = 0.5f;
static const float POS_X_RAIL1 = 3.0f;
static const float POS_X_RAIL2 = 7.0f;

/* Straight rail */
static const int STRAIGHT_TRACK_BALLAST_COUNT = 5;
GLBI_Convex_2D_Shape straightRail{3};

/* Curved rail */
static const int CURVED_TRACK_BALLAST_COUNT = 3;
GLBI_Convex_2D_Shape iternalCurvedRail{3};
GLBI_Convex_2D_Shape externalCurvedRail{3};

/* Ballast */
static const float RR = 0.25f;
static const float BALLAST_X_START = 2.0f;
static const float BALLAST_X_END = 8.0f;
IndexedMesh *ballast = NULL;
StandardMesh *ballast_side = NULL;

/* Station */
static const float STATION_GROUND_HEIGHT_1 = CELL_SIZE / 3.0f;
GLBI_Convex_2D_Shape station_ground_1{3};
static const float STATION_GROUND_HEIGHT_2 = 1.0f;
GLBI_Convex_2D_Shape station_ground_2{3};
static const float BENCH_WIDTH = CELL_SIZE / 2.0f - 0.5f;
static const float BENCH_HEIGHT = 1.0f;
static const float BENCH_LENGTH = 2.0f;
GLBI_Convex_2D_Shape bench{3};
static const float STRIP_WIDTH = CELL_SIZE - 0.5f;
static const float STRIP_HEIGHT = 0.1f;
static const float STRIP_LENGTH = 1.5f;
GLBI_Convex_2D_Shape strip{3};

/* Train */
static const float TRAIN_X_START = 2.0f;
static const float TRAIN_X_END = 8.0f;
GLBI_Convex_2D_Shape train{3};
static const float TRAIN_WHEEL_RADIUS = 1.0f;
IndexedMesh *train_wheel = NULL;
StandardMesh *train_wheel_side = NULL;
static const float TRAIN_CHIMNEY_HEIGHT = 2.5f;
static const float TRAIN_CHIMNEY_RADIUS = 0.5f;
IndexedMesh *train_chimney = NULL;
StandardMesh *train_chimney_hat = NULL;

/* Tree */
static const float TRUNK_WIDTH = 2.0f;
static const float TRUNK_HEIGHT = 10.0f;
GLBI_Convex_2D_Shape trunk{3};
static const float LEAF_WIDTH = 7.0f;
static const float LEAF_HEIGHT = 9.0f;
GLBI_Convex_2D_Shape leaf{3};
std::vector<std::pair<int, int>> tree_pos{};

/* Building */
static const int BUILDING_SIZE = 10;
static const float BUILDING_HEIGHT = 1.5f;
static const float BLACK_BUILDING_WIDTH = 6.0f;
GLBI_Convex_2D_Shape black_building{3};
static const float GRAY_BUILDING_WIDTH = 10.0f;
GLBI_Convex_2D_Shape gray_building{3};
std::vector<std::pair<int, int>> building_pos{};

stbi_uc *img;
GLBI_Texture grass_texture;

GLBI_Engine myEngine;

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

void add_rectangle_triangles(std::vector<float> &in_coord, const Vector3D &origin, float width, float height, float length)
{
    /* Back face */
    add_triangle(in_coord, {origin.x, origin.y, origin.z}, {origin.x + width, origin.y, origin.z}, {origin.x, origin.y, origin.z + height});
    add_triangle(in_coord, {origin.x + width, origin.y, origin.z}, {origin.x + width, origin.y, origin.z + height}, {origin.x, origin.y, origin.z + height});

    /* Front face */
    add_triangle(in_coord, {origin.x, origin.y + length, origin.z}, {origin.x + width, origin.y + length, origin.z}, {origin.x, origin.y + length, origin.z + height});
    add_triangle(in_coord, {origin.x + width, origin.y + length, origin.z}, {origin.x + width, origin.y + length, origin.z + height}, {origin.x, origin.y + length, origin.z + height});

    /* Left face */
    add_triangle(in_coord, {origin.x, origin.y, origin.z}, {origin.x, origin.y, origin.z + height}, {origin.x, origin.y + length, origin.z});
    add_triangle(in_coord, {origin.x, origin.y + length, origin.z}, {origin.x, origin.y + length, origin.z + height}, {origin.x, origin.y, origin.z + height});

    /* Right face */
    add_triangle(in_coord, {origin.x + width, origin.y, origin.z}, {origin.x + width, origin.y, origin.z + height}, {origin.x + width, origin.y + length, origin.z});
    add_triangle(in_coord, {origin.x + width, origin.y + length, origin.z}, {origin.x + width, origin.y + length, origin.z + height}, {origin.x + width, origin.y, origin.z + height});

    /* Bottom face */
    add_triangle(in_coord, {origin.x, origin.y, origin.z}, {origin.x + width, origin.y, origin.z}, {origin.x, origin.y + length, origin.z});
    add_triangle(in_coord, {origin.x, origin.y + length, origin.z}, {origin.x + width, origin.y + length, origin.z}, {origin.x + width, origin.y, origin.z});

    /* Top face */
    add_triangle(in_coord, {origin.x, origin.y, origin.z + height}, {origin.x + width, origin.y, origin.z + height}, {origin.x, origin.y + length, origin.z + height});
    add_triangle(in_coord, {origin.x, origin.y + length, origin.z + height}, {origin.x + width, origin.y + length, origin.z + height}, {origin.x + width, origin.y, origin.z + height});
}

/* ---INITIALIZATION--- */

void init_set_positions(const nlohmann::json &data)
{
    const auto sizeGrid = data["size_grid"].get<int>();
    std::vector<std::pair<int, int>> set_pos{};
    for (auto y = 0; y < sizeGrid; y++)
        for (auto x = 0; x < sizeGrid; x++)
            set_pos.emplace_back(x, y);

    auto origin = data["origin"].get<std::pair<int, int>>();
    const auto path = data["path"].get<std::vector<std::pair<int, int>>>();
    set_pos.erase(
        std::remove_if(set_pos.begin(), set_pos.end(), [&origin, &path](const std::pair<int, int> &p)
                       { return p == origin || std::find(path.begin(), path.end(), p) != path.end(); }),
        set_pos.end());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(2, 5);
    std::shuffle(set_pos.begin(), set_pos.end(), gen);

    int tree_count = dist(gen);
    for (int i = 0; i < tree_count && !set_pos.empty(); i++)
    {
        tree_pos.push_back(set_pos.back());
        set_pos.pop_back();
    }

    int building_count = dist(gen);
    for (int i = 0; i < building_count && !set_pos.empty(); i++)
    {
        building_pos.push_back(set_pos.back());
        set_pos.pop_back();
    }
}

void initCamera(const nlohmann::json &data)
{
    float sizeGrid = data["size_grid"].get<int>() * CELL_SIZE;
    camera_pos = Vector3D{sizeGrid / 2.0f, sizeGrid / 2.0f, 25.0f};
}

void initGround(const nlohmann::json &data)
{
    float sizeGrid = data["size_grid"].get<int>() * CELL_SIZE;
    ground = basicRect(sizeGrid, sizeGrid);
    ground->createVAO();
}

void initStraightRail()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, SR, SR, CELL_SIZE);
    straightRail.initShape(in_coord);
    straightRail.changeNature(GL_TRIANGLES);
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

void initBallastSide()
{
    ballast_side = basicCone(0.0f, RR);
    ballast_side->createVAO();
}

void initStationGround1()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, CELL_SIZE, STATION_GROUND_HEIGHT_1, CELL_SIZE);
    station_ground_1.initShape(in_coord);
    station_ground_1.changeNature(GL_TRIANGLES);
}

void initStationGround2()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, CELL_SIZE, STATION_GROUND_HEIGHT_2, CELL_SIZE);
    station_ground_2.initShape(in_coord);
    station_ground_2.changeNature(GL_TRIANGLES);
}

void initBench()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, 0.25, BENCH_HEIGHT - 0.25, BENCH_LENGTH);               // Left foot
    add_rectangle_triangles(in_coord, Vector3D{BENCH_WIDTH - 0.25, 0.0f, 0.0f}, 0.25, BENCH_HEIGHT - 0.25, BENCH_LENGTH); // Right foot
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, BENCH_HEIGHT - 0.25}, BENCH_WIDTH, 0.25, BENCH_LENGTH);        // Top plank
    bench.initShape(in_coord);
    bench.changeNature(GL_TRIANGLES);
}

void initStrip()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, STRIP_WIDTH, STRIP_HEIGHT, STRIP_LENGTH);
    strip.initShape(in_coord);
    strip.changeNature(GL_TRIANGLES);
}

void initTrain()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{0.0f, 0.0f, 0.0f}, TRAIN_X_END - TRAIN_X_START, 4.0f, CELL_SIZE);
    add_rectangle_triangles(in_coord, Vector3D{((TRAIN_X_END - TRAIN_X_START) - (TRAIN_X_END - TRAIN_X_START - 2.0f)) / 2.0f, 0.0f, 4.0f}, TRAIN_X_END - TRAIN_X_START - 2.0f, 2.0f, 2.0f * CELL_SIZE / 3.0f);
    train.initShape(in_coord);
    train.changeNature(GL_TRIANGLES);
}

void initTrainWheel()
{
    train_wheel = basicCylinder(SR, TRAIN_WHEEL_RADIUS);
    train_wheel->createVAO();
}

void initTrainWheelSide()
{
    train_wheel_side = basicCone(0.0f, TRAIN_WHEEL_RADIUS);
    train_wheel_side->createVAO();
}

void initTrainChimney()
{
    train_chimney = basicCylinder(TRAIN_CHIMNEY_HEIGHT, TRAIN_CHIMNEY_RADIUS);
    train_chimney->createVAO();
}

void initTrainChimneyHat()
{
    train_chimney_hat = basicCone(1.0f, TRAIN_CHIMNEY_RADIUS * 2.0f);
    train_chimney_hat->createVAO();
}

void initTrunk()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{CELL_SIZE / 2.0f - TRUNK_WIDTH / 2.0f, CELL_SIZE / 2.0f - TRUNK_WIDTH / 2.0f, 0.0f}, TRUNK_WIDTH, TRUNK_HEIGHT, TRUNK_WIDTH);
    trunk.initShape(in_coord);
    trunk.changeNature(GL_TRIANGLES);
}

void initLeaf()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{CELL_SIZE / 2.0f - LEAF_WIDTH / 2.0f, CELL_SIZE / 2.0f - LEAF_WIDTH / 2.0f, TRUNK_HEIGHT}, LEAF_WIDTH, LEAF_HEIGHT, LEAF_WIDTH);
    leaf.initShape(in_coord);
    leaf.changeNature(GL_TRIANGLES);
}

void initBlackBuilding()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{CELL_SIZE / 2.0f - BLACK_BUILDING_WIDTH / 2.0f, CELL_SIZE / 2.0f - BLACK_BUILDING_WIDTH / 2.0f, 0.0f}, BLACK_BUILDING_WIDTH, BUILDING_HEIGHT, BLACK_BUILDING_WIDTH);
    black_building.initShape(in_coord);
    black_building.changeNature(GL_TRIANGLES);
}

void initGrayBuilding()
{
    std::vector<float> in_coord{};
    add_rectangle_triangles(in_coord, Vector3D{CELL_SIZE / 2.0f - GRAY_BUILDING_WIDTH / 2.0f, CELL_SIZE / 2.0f - GRAY_BUILDING_WIDTH / 2.0f, 0.0f}, GRAY_BUILDING_WIDTH, BUILDING_HEIGHT, GRAY_BUILDING_WIDTH);
    gray_building.initShape(in_coord);
    gray_building.changeNature(GL_TRIANGLES);
}

void initScene(const nlohmann::json &data)
{
    /* Camera */
    initCamera(data);

    init_set_positions(data);

    /* Ground */
    initGround(data);

    /* Rails */
    initStraightRail();
    initInternalCurvedRail();
    initExternalCurvedRail();

    /* Ballast */
    initBallast();
    initBallastSide();

    /* Station */
    initStationGround1();
    initStationGround2();
    initBench();
    initStrip();

    /* Train */
    initTrain();
    initTrainWheel();
    initTrainWheelSide();
    initTrainChimney();
    initTrainChimneyHat();

    /* Tree */
    initTrunk();
    initLeaf();

    /* Building */
    initBlackBuilding();
    initGrayBuilding();
}

bool initGrassTexture()
{
    int x, y, comp;
    img = stbi_load("../assets/textures/grass.jpg", &x, &y, &comp, 0);
    if (img == NULL)
    {
        std::cerr << "ERROR: Can't load ../assets/textures/grass.jpg" << std::endl;
        return false;
    }
    grass_texture.createTexture();
    grass_texture.attachTexture();
    grass_texture.setParameters(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    grass_texture.loadImage(x, y, comp, img);
    grass_texture.detachTexture();
    return true;
}

void freeGrassTexture()
{
    stbi_image_free(img);
}

/* ---GROUND--- */

void drawGround()
{
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{-1.0f, 0.0f, 0.0f});
    myEngine.updateMvMatrix();

    myEngine.activateTexturing(true);
    grass_texture.attachTexture();
    ground->draw();
    grass_texture.detachTexture();
    myEngine.activateTexturing(false);

    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

/* ---TRACKS--- */

void drawBallast()
{
    ballast_side->draw();
    ballast->draw();
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.updateMvMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, BALLAST_X_END - BALLAST_X_START, 0.0f});
    myEngine.updateMvMatrix();
    ballast_side->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void drawStraightTrack()
{
    /* Rails */
    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL1 - (SR / 2.0f), 0.0f, RR * 2.0f});
    myEngine.updateMvMatrix();
    straightRail.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL2 - (SR / 2.0f), 0.0f, RR * 2.0f});
    myEngine.updateMvMatrix();
    straightRail.drawShape();
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
        drawBallast();
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
    drawBallast();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{BALLAST_X_START * std::cos(3.0f * M_PI / 12.0f), BALLAST_X_START * std::sin(3.0f * M_PI / 12.0f), RR});
    myEngine.mvMatrixStack.addRotation(3.0f * M_PI / 12.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawBallast();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{BALLAST_X_START * std::cos(M_PI / 12.0f), BALLAST_X_START * std::sin(M_PI / 12.0f), RR});
    myEngine.mvMatrixStack.addRotation(5.0f * M_PI / 12.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawBallast();
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
    /*
    |
    +-
    */
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

/* ---STATION--- */

void rotateStation(const Vector2D &origin, const Vector2D &track)
{
    if (track.x == origin.x + 1)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    else if (track.x == origin.x - 1)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE, 0.0f, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, 1.0f});
        myEngine.updateMvMatrix();
    }
    else if (track.y == origin.y - 1)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
}

void drawStation(const nlohmann::json &data)
{
    auto origin = Vector2D{data["origin"].get<std::vector<int>>()};
    auto path = data["path"].get<std::vector<std::vector<int>>>();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE * origin.x, CELL_SIZE * origin.y, 0.0f});
    myEngine.updateMvMatrix();

    /* Turn the station towards the track */
    auto it = std::find_if(path.begin(), path.end(), [&origin](const std::vector<int> &pos)
                           { return origin.isNeighbor(pos); });
    if (it != path.end())
        rotateStation(origin, *it);

    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);
    station_ground_1.drawShape();

    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, 0.0f, STATION_GROUND_HEIGHT_1});
    myEngine.updateMvMatrix();
    myEngine.setFlatColor(0.25f, 0.25f, 0.25f);
    station_ground_2.drawShape();

    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, 0.0f, STATION_GROUND_HEIGHT_2});
    myEngine.updateMvMatrix();

    myEngine.setFlatColor(0.4f, 0.2f, 0.0f);

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.25f, 0.0f, 0.0f});
    myEngine.updateMvMatrix();
    bench.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE / 2.0f + 0.25f, 0.0f, 0.0f});
    myEngine.updateMvMatrix();
    bench.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.25f, CELL_SIZE - STRIP_LENGTH - 0.25f, 0.0f});
    myEngine.updateMvMatrix();
    myEngine.setFlatColor(0.6f, 0.5f, 0.0f);
    strip.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

/* ---TRAIN--- */

void drawTrainWheel()
{
    myEngine.setFlatColor(0.6f, 0.0f, 0.0f);

    train_wheel_side->draw();
    train_wheel->draw();

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, SR, 0.0f});
    myEngine.updateMvMatrix();
    train_wheel_side->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void rotateTrainOnStraightTrack(const Vector2D &current, const Vector2D &next)
{
    if (next.x == current.x + 1)
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    else if (next.x == current.x - 1)
    {
        myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, 1.0f});
        myEngine.updateMvMatrix();
    }
}

void rotateTrainOnCurvedTrack(const Vector2D &prev, const Vector2D &current, const Vector2D &next)
{
    /*
     |
    -+
    */
    if ((prev.x == current.x - 1 || next.x == current.x - 1) && (prev.y == current.y + 1 || next.y == current.y + 1))
    {
        float angle = M_PI / 4.0f;
        myEngine.mvMatrixStack.addTranslation(Vector3D{-CELL_SIZE / 2.0f, CELL_SIZE / 2.0f, 0.0f});
        myEngine.mvMatrixStack.addRotation(angle, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    /*
    +-
    |
    */
    else if ((prev.y == current.y - 1 || next.y == current.y - 1) && (prev.x == current.x + 1 || next.x == current.x + 1))
    {
        float angle = M_PI / 4.0f;
        myEngine.mvMatrixStack.addRotation(angle, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    /*
    |
    +-
    */
    else if ((prev.y == current.y + 1 || next.y == current.y + 1) && (prev.x == current.x + 1 || next.x == current.x + 1))
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE / 2.0f, CELL_SIZE + CELL_SIZE / 2.0f, 0.0f});
        myEngine.mvMatrixStack.addRotation(3.0f * M_PI / 4.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
    /*
    -+
     |
    */
    else
    {
        myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE / 2.0f, CELL_SIZE, 0.0f});
        myEngine.mvMatrixStack.addRotation(3.0f * M_PI / 4.0f, Vector3D{0.0f, 0.0f, -1.0f});
        myEngine.updateMvMatrix();
    }
}

void rotateTrain(const std::vector<std::vector<int>> &path, const Vector2D &current)
{
    if (path.size() == 2)
        rotateTrainOnStraightTrack(current, Vector2D{path[1]});
    else if (path.size() > 2)
    {
        auto prev = Vector2D{path[path.size() - 1]};
        auto next = Vector2D{path[1]};
        if (current.isNeighbor(prev) && current.isNeighbor(next) && isCorner(prev, current, next))
            rotateTrainOnCurvedTrack(prev, current, next);
        else
            rotateTrainOnStraightTrack(current, next);
    }
}

void drawTrain(const nlohmann::json &data)
{
    auto path = data["path"].get<std::vector<std::vector<int>>>();
    if (path.size() == 0)
        return;
    auto position = Vector2D{path[0]};

    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE * position.x, CELL_SIZE * position.y, RR * 2.0f + SR});
    myEngine.updateMvMatrix();

    rotateTrain(path, position);

    /* Bottom left wheel */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL1 - SR / 2.0f, TRAIN_WHEEL_RADIUS, TRAIN_WHEEL_RADIUS});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawTrainWheel();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Bottom right wheel */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL2 - SR / 2.0f, TRAIN_WHEEL_RADIUS, TRAIN_WHEEL_RADIUS});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawTrainWheel();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Top left wheel */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL1 - SR / 2.0f, CELL_SIZE - TRAIN_WHEEL_RADIUS, TRAIN_WHEEL_RADIUS});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawTrainWheel();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Top right wheel */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{POS_X_RAIL2 - SR / 2.0f, CELL_SIZE - TRAIN_WHEEL_RADIUS, TRAIN_WHEEL_RADIUS});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{0.0f, 0.0f, -1.0f});
    myEngine.updateMvMatrix();
    drawTrainWheel();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Train */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{TRAIN_X_START, 0.0f, TRAIN_WHEEL_RADIUS * 2.0f});
    myEngine.updateMvMatrix();
    myEngine.setFlatColor(0.1f, 0.1f, 0.1f);
    train.drawShape();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Train chimney */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE / 2.0f, TRAIN_CHIMNEY_RADIUS + 4.0f, 4.0f + TRAIN_X_END - TRAIN_X_START - 2.0f});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{1.0f, 0.0f, 0.0f});
    myEngine.updateMvMatrix();
    myEngine.setFlatColor(0.2f, 0.2f, 0.2f);
    train_chimney->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    /* Train chimney hat */
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.mvMatrixStack.addTranslation(Vector3D{CELL_SIZE / 2.0f, TRAIN_CHIMNEY_RADIUS + 4.0f, 4.0f + TRAIN_X_END - TRAIN_X_START - 2.0f + TRAIN_CHIMNEY_HEIGHT});
    myEngine.mvMatrixStack.addRotation(M_PI / 2.0f, Vector3D{1.0f, 0.0f, 0.0f});
    myEngine.updateMvMatrix();
    myEngine.setFlatColor(0.1f, 0.1f, 0.1f);
    train_chimney_hat->draw();
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();

    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void draw_tree()
{
    myEngine.setFlatColor(0.3f, 0.15f, 0.15f);
    trunk.drawShape();
    myEngine.setFlatColor(0.0f, 0.4f, 0.0f);
    leaf.drawShape();
}

void draw_building()
{
    myEngine.mvMatrixStack.pushMatrix();
    myEngine.updateMvMatrix();
    for (auto i = 0; i < BUILDING_SIZE; i++)
    {
        myEngine.setFlatColor(0.1f, 0.1f, 0.1f);
        black_building.drawShape();
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, 0.0f, BUILDING_HEIGHT});
        myEngine.updateMvMatrix();
        myEngine.setFlatColor(0.4f, 0.4f, 0.4f);
        gray_building.drawShape();
        myEngine.mvMatrixStack.addTranslation(Vector3D{0.0f, 0.0f, BUILDING_HEIGHT});
        myEngine.updateMvMatrix();
    }
    myEngine.mvMatrixStack.popMatrix();
    myEngine.updateMvMatrix();
}

void renderScene(const nlohmann::json &data)
{
    drawGround();
    drawTracks(data);
    drawStation(data);
    drawTrain(data);
}
