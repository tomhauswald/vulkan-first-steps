#pragma once

#include <engine.h>
#include <mesh_util.h>
#include <model.h>

enum Dir { North = 1, East = 2, South = 4, West = 8 };

class Room : public GameObject3d {
private:
  size_t m_index;
  glm::vec3 m_origin;
  glm::vec3 m_size;
  Model m_model;

public:
  static constexpr auto wallThickness = 0.1f;
  static constexpr auto doorWidth = 3.0f;
  static constexpr auto doorHeight = 3.0f;

  Room(Engine3d &e, size_t index, glm::vec3 origin, glm::vec3 size, Dir doors)
      : GameObject3d(e), m_index(index), m_origin(std::move(origin)),
        m_size(std::move(size)),
        m_model(e.renderer().createMesh("room" + std::to_string(index)),
                e.renderer().texture("stonebrick_mossy")) {

    auto vertices = std::vector<VPositionColorTexcoord>{};

    const auto addCubeAt = [&vertices](glm::vec3 const &pos,
                                       glm::vec3 const &size,
                                       glm::vec3 const &col = {1, 1, 1}) {
      if (size.x > 0 && size.y > 0 && size.z > 0) {
        for (auto const &v : cubeVertices(pos + size / 2.0f, size, col, size)) {
          vertices.push_back(v);
        }
      }
    };

    // floor
    addCubeAt({0, 0, 0}, {m_size.x, wallThickness, m_size.z}, {1, 0, 1});

    // ceiling
    addCubeAt({0, m_size.y - wallThickness, 0},
              {m_size.x, wallThickness, m_size.z}, {0, 1, 0});

    // north wall
    if (doors & Dir::North) {
      addCubeAt({0, wallThickness, m_size.z - wallThickness},
                {(m_size.x - doorWidth) / 2.0f, doorHeight, wallThickness},
                {0, 0, 1});
      addCubeAt({m_size.x - (m_size.x - doorWidth) / 2.0f, wallThickness,
                 m_size.z - wallThickness},
                {(m_size.x - doorWidth) / 2.0f, doorHeight, wallThickness},
                {0, 0, 1});
      addCubeAt(
          {0, wallThickness + doorHeight, m_size.z - wallThickness},
          {m_size.x, m_size.y - 2 * wallThickness - doorHeight, wallThickness},
          {0, 0, 1});
    } else {
      addCubeAt({0, wallThickness, m_size.z - wallThickness},
                {m_size.x, m_size.y - 2 * wallThickness, wallThickness},
                {0, 0, 1});
    }

    // east wall
    if (doors & Dir::East) {
      addCubeAt({m_size.x - wallThickness, wallThickness, wallThickness},
                {wallThickness, doorHeight,
                 (m_size.z - doorWidth) / 2.0f - wallThickness},
                {1, 0, 0});
      addCubeAt({m_size.x - wallThickness, wallThickness,
                 m_size.z - (m_size.z - doorWidth) / 2.0f},
                {wallThickness, doorHeight,
                 (m_size.z - doorWidth) / 2.0f - wallThickness},
                {1, 0, 0});
      addCubeAt(
          {m_size.x - wallThickness, wallThickness + doorHeight, wallThickness},
          {wallThickness, m_size.y - 2 * wallThickness - doorHeight,
           m_size.z - 2 * wallThickness},
          {1, 0, 0});
    } else {
      addCubeAt({m_size.x - wallThickness, wallThickness, wallThickness},
                {wallThickness, m_size.y - 2 * wallThickness,
                 m_size.z - 2 * wallThickness},
                {1, 0, 0});
    }

    // south wall
    if (doors & Dir::South) {
      addCubeAt({0, wallThickness, 0},
                {(m_size.x - doorWidth) / 2.0f, doorHeight, wallThickness},
                {1, 1, 0});
      addCubeAt({m_size.x - (m_size.x - doorWidth) / 2.0f, wallThickness, 0},
                {(m_size.x - doorWidth) / 2.0f, doorHeight, wallThickness},
                {1, 1, 0});
      addCubeAt(
          {0, wallThickness + doorHeight, 0},
          {m_size.x, m_size.y - 2 * wallThickness - doorHeight, wallThickness},
          {1, 1, 0});
    } else {
      addCubeAt({0, wallThickness, 0},
                {m_size.x, m_size.y - 2 * wallThickness, wallThickness},
                {1, 1, 0});
    }

    // west wall
    if (doors & Dir::West) {
      addCubeAt({0, wallThickness, wallThickness},
                {wallThickness, doorHeight,
                 (m_size.z - doorWidth) / 2.0f - wallThickness},
                {0, 1, 1});
      addCubeAt({0, wallThickness, m_size.z - (m_size.z - doorWidth) / 2.0f},
                {wallThickness, doorHeight,
                 (m_size.z - doorWidth) / 2.0f - wallThickness},
                {0, 1, 1});
      addCubeAt({0, wallThickness + doorHeight, wallThickness},
                {wallThickness, m_size.y - 2 * wallThickness - doorHeight,
                 m_size.z - 2 * wallThickness},
                {0, 1, 1});
    } else {
      addCubeAt({0, wallThickness, wallThickness},
                {wallThickness, m_size.y - 2 * wallThickness,
                 m_size.z - 2 * wallThickness},
                {0, 1, 1});
    }

    m_model.mesh().setVertices(vertices);
    m_model.setPosition(m_origin);
  }
};
