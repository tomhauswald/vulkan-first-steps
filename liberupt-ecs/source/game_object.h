#pragma once

#include <renderer.h>

template <typename EngineType> class GameObject {
private:
  bool m_alive = true;
  float m_lifetime = 0;

protected:
  EngineType &m_engine;

public:
  inline GameObject(EngineType &e) : m_engine(e) {}

  inline virtual ~GameObject() = default;

  inline virtual void update(float dt) { m_lifetime += dt; }

  inline virtual void draw(typename EngineType::renderer_type &r) const {}

  inline void kill() noexcept { m_alive = false; }

  GETTER(alive, m_alive)
  GETTER(lifetime, m_lifetime)
};
