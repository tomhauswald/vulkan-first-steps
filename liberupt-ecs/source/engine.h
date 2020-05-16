#pragma once

#include <renderer.h>

#include <chrono>

#include "game_object.h"

struct EngineSettings {
  RendererSettings renderer;
};

template <typename RendererType>
class Engine {
 public:
  using renderer_type = RendererType;
  using game_object_type = GameObject<Engine<RendererType>>;

  inline Engine(EngineSettings const& settings)
      : m_settings{settings},
        m_renderer{settings.renderer},
        m_objects{},
        m_totalTime{} {
    srand(time(nullptr));
    m_renderer.materialize();
  }

  template <typename Obj, typename... Args>
  inline Obj& add(Args&&... args) {
    m_objects.push_back(
        std::make_unique<Obj>(*this, std::forward<Args>(args)...));
    return dynamic_cast<Obj&>(*m_objects.back());
  }

  void run() {
    auto prev = std::chrono::high_resolution_clock::now();

    while (m_renderer.isWindowOpen()) {
      auto now = std::chrono::high_resolution_clock::now();
      auto dt =
          std::chrono::duration_cast<std::chrono::duration<float>>(now - prev)
              .count();
      m_totalTime += dt;
      prev = now;

      std::remove_if(m_objects.begin(), m_objects.end(),
                     [](auto const& pObj) { return !pObj->alive(); });

      if (m_renderer.tryBeginFrame()) {
        for (auto& object : m_objects) {
          object->update(dt);
          object->draw(m_renderer);
        }
        m_renderer.endFrame();
      }
      m_renderer.handleWindowEvents();
    }
  }

  inline renderer_type& renderer() noexcept { return m_renderer; }

  GETTER(settings, m_settings)
  GETTER(objects, m_objects)
  GETTER(totalTime, m_totalTime)

 private:
  EngineSettings m_settings;
  renderer_type m_renderer;
  std::vector<std::unique_ptr<game_object_type>> m_objects;
  float m_totalTime;
};

using Engine2d = Engine<Renderer2d>;
using Engine3d = Engine<Renderer3d>;
using GameObject2d = GameObject<Engine<Renderer2d>>;
using GameObject3d = GameObject<Engine<Renderer3d>>;
