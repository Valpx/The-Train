#pragma once

#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_set_of_points.hpp"
#include "glbasimac/glbi_convex_2D_shape.hpp"
#include "tools/basic_mesh.hpp"
#include "nlohmann/json.hpp"

using namespace glbasimac;

/* Camera parameters and functions */
static const float Z_NEAR = 0.1f;
static const float Z_FAR = 500.0f;
extern Vector3D camera_pos;
extern Vector3D camera_front;
extern Vector3D camera_up;

/* OpenGL Engine */
extern GLBI_Engine myEngine;

void initScene(const nlohmann::json &);

bool initGrassTexture();

void freeGrassTexture();

void renderScene(const nlohmann::json &);
