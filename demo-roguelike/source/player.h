#pragma once

#include <engine.h>
#include <mesh_util.h>
#include <model.h>

class Player : public GameObject3d {
private:
  Model m_model;

public:
  Player(Engine3d &e)
      : GameObject3d(e), m_model(e.renderer().createMesh("player"),
                                 e.renderer().texture("nether_brick")) {
    m_model.mesh().setVertices(
        cubeVertices({0.0f, 0.75f, 0.0f}, {0.75f, 1.5f, 0.75f}));
  }

  virtual ~Player() {}
};
