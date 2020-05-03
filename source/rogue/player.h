#pragma once

#include "../model.h"
#include "../mesh_util.h"

class Player : public Model {
private:

public:
    Player(Engine& e) :
        Model(e, e.renderer().createMesh("player"), e.renderer().texture("nether_brick")) {
        mesh().setVertices(cubeVertices({ 0.0f, 0.75f, 0.0f }, { 0.75f, 1.5f, 0.75f }));
    }

    virtual ~Player() {}
};
