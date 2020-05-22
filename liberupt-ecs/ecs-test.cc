#include "ecs.hh"

#include <memory>

struct CTransform {
  float position[2] = {0.0f, 0.0f};
};

struct CRigidbody {
  float velocity[2] = {0.0f, 0.0f};
  float acceleration[2] = {0.0f, 0.0f};
};

struct SMovement : public System<CTransform, CRigidbody> {
  float dt = 1 / 60.f;
  void setup() override {
    std::cout << "Setting up movement system." << std::endl;
  }
  void teardown() override {
    std::cout << "Tearing down movement system." << std::endl;
  }
  void apply(size_t id, CTransform& t, CRigidbody& r) override {
    r.velocity[0] += r.acceleration[0] * dt;
    r.velocity[1] += r.acceleration[1] * dt;
    t.position[0] += r.velocity[0] * dt;
    t.position[1] += r.velocity[1] * dt;
    printf("%lu: p=(%0.2f,%0.2f), v=(%0.2f,%0.2f), a=(%0.2f,%0.2f)\n", id,
           t.position[0], t.position[1], r.velocity[0], r.velocity[1],
           r.acceleration[0], r.acceleration[1]);
  }
};

int main() {
  srand(time(NULL));
  auto ecs = std::make_unique<EntityComponentSystem<
      10000, ComponentList<CTransform, CRigidbody>, SystemList<SMovement>>>();

  static constexpr size_t numEntities = 5;
  for (size_t i = 0; i < numEntities; ++i) {
    auto e = ecs->createEntity();

    auto& transform = ecs->addComponent<CTransform>(e);
    transform.position[0] = rand() / (float)RAND_MAX;
    transform.position[1] = rand() / (float)RAND_MAX;

    auto& rb = ecs->addComponent<CRigidbody>(e);
    rb.acceleration[0] = rand() / (float)RAND_MAX;
    rb.acceleration[1] = rand() / (float)RAND_MAX;
  }

  static constexpr size_t numTicks = 3;
  for (size_t i = 0; i < numTicks; ++i) ecs->tick();

  return 0;
}
