#include "ecs.hh"

struct CTransform {
  float position[2] = {0.0f, 0.0f};
};

struct CRigidbody {
  float velocity[2] = {0.0f, 0.0f};
  float acceleration[2] = {0.0f, 0.0f};
};

struct SMovement : public System<CTransform, CRigidbody> {
  void update(float dt, CTransform& t, CRigidbody& r) override {
    r.velocity[0] += r.acceleration[0] * dt;
    r.velocity[1] += r.acceleration[1] * dt;
    t.position[0] += r.velocity[0] * dt;
    t.position[1] += r.velocity[1] * dt;
    printf("p=(%0.2f,%0.2f), v=(%0.2f,%0.2f), a=(%0.2f,%0.2f)\n", t.position[0],
           t.position[1], r.velocity[0], r.velocity[1], r.acceleration[0],
           r.acceleration[1]);
  }
};

using MyECS = EntityComponentSystem<1024, ComponentList<CTransform, CRigidbody>,
                                    SystemList<SMovement>>;

int main() {
  MyECS ecs;
  srand(time(NULL));

  for(int i=0; i<1000; ++i) {
    auto e = ecs.createEntity();
    ecs.addComponent<CTransform>(e).position[0] = rand() / (float)RAND_MAX;
    ecs.addComponent<CTransform>(e).position[1] = rand() / (float)RAND_MAX;
    ecs.addComponent<CRigidbody>(e).acceleration[0] = rand() / (float)RAND_MAX;
    ecs.addComponent<CRigidbody>(e).acceleration[1] = rand() / (float)RAND_MAX;
  }

  while (true) ecs.update(1e-6);
  return 0;
}
