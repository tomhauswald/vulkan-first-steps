#pragma once

#include <engine.h>
#include <glm/gtx/rotate_vector.hpp>

class CameraController : public GameObject3d {
private:
  Camera3d &m_cam;
  glm::vec3 m_eye;
  glm::vec3 m_target;

protected:
  Keyboard const &m_kb;
  Mouse &m_mouse;
  bool m_captureMouse;

public:
  CameraController(Engine3d &e)
      : GameObject3d(e), m_cam(e.renderer().camera3d()), m_eye(0, 0, 0),
        m_target(0, 0, 1), m_kb(e.renderer().keyboard()),
        m_mouse(e.renderer().mouse()), m_captureMouse(true) {

    m_engine.renderer().mouse().setCursorMode(CursorMode::Centered);
  }

  virtual void update(float dt) override {

    if (m_kb.pressed(GLFW_KEY_ESCAPE)) {
      m_captureMouse = !m_captureMouse;
      m_mouse.setCursorMode(m_captureMouse ? CursorMode::Centered
                                           : CursorMode::Normal);
    }

    m_cam.setPosition(m_eye);
    m_cam.lookAt(m_target);

    GameObject3d::update(dt);
  }

  virtual ~CameraController() = default;

  GETTER(eye, m_eye)
  GETTER(target, m_target)

  SETTER(setEye, m_eye)
  SETTER(setTarget, m_target)
};

class FirstPersonController : public CameraController {
private:
  static constexpr float pitchRadius = 89.9f;

  float m_pitch;
  float m_yaw;
  bool m_moving;
  bool m_sprinting;
  float m_moveSpeed;
  float m_sprintSpeed;
  float m_rotateSpeed;

public:
  FirstPersonController(Engine3d &e, float moveSpeed, float sprintSpeed,
                        float rotateSpeed)
      : CameraController(e), m_pitch(0), m_yaw(0), m_moving(false),
        m_sprinting(false), m_moveSpeed(moveSpeed), m_sprintSpeed(sprintSpeed),
        m_rotateSpeed(rotateSpeed) {}

  void update(float dt) override {

    // Control camera with keyboard and mouse.
    if (m_captureMouse) {

      m_pitch = glm::clamp(m_pitch + m_mouse.movement().y * m_rotateSpeed * dt,
                           -pitchRadius, pitchRadius);

      m_yaw += m_mouse.movement().x * m_rotateSpeed * dt;
      while (m_yaw >= 180.0f)
        m_yaw -= 360.0f;
      while (m_yaw < -180.0f)
        m_yaw += 360.0f;

      auto forward = glm::rotateY(glm::vec3{0, 0, 1}, glm::radians(m_yaw));
      auto right = glm::cross(glm::vec3{0, 1, 0}, forward);
      forward = glm::rotate(forward, glm::radians(m_pitch), right);

      auto dir = glm::vec3{};

      if (m_kb.down(GLFW_KEY_W))
        dir += forward;
      if (m_kb.down(GLFW_KEY_A))
        dir -= right;
      if (m_kb.down(GLFW_KEY_S))
        dir -= forward;
      if (m_kb.down(GLFW_KEY_D))
        dir += right;

      m_moving = (dir.x != 0.0f || dir.z != 0.0f);
      m_sprinting = m_moving && m_kb.down(GLFW_KEY_LEFT_SHIFT);

      if (m_moving) {
        setEye(eye() + glm::normalize(dir) *
                           (m_sprinting ? m_sprintSpeed : m_moveSpeed) * dt);
      }

      setTarget(eye() + forward);
    }

    if (m_kb.pressed(GLFW_KEY_PERIOD)) {
      printf("X: %0.1f Y: %0.1f Z: %0.1f Pitch: %0.1f Yaw: %0.1f\n", eye().x,
             eye().y, eye().z, m_pitch, m_yaw);
    }

    CameraController::update(dt);
  }

  virtual ~FirstPersonController() = default;
};

class ThirdPersonController : public CameraController {
private:
  Model &m_targetModel;
  float m_yOffset;
  float m_distance;
  float m_pitch;
  float m_yaw;
  bool m_moving;
  bool m_sprinting;
  float m_moveSpeed;
  float m_sprintSpeed;
  float m_rotateSpeed;

public:
  ThirdPersonController(Engine3d &e, Model &targetModel, float yOffset,
                        float distance, float moveSpeed, float sprintSpeed,
                        float rotateSpeed)
      : CameraController(e), m_targetModel(targetModel), m_yOffset(yOffset),
        m_distance(distance), m_pitch(0.0f), m_yaw(0.0f), m_moving(false),
        m_sprinting(false), m_moveSpeed(moveSpeed), m_sprintSpeed(sprintSpeed),
        m_rotateSpeed(rotateSpeed) {}

  void update(float dt) override {

    // Move model and camera with keyboard and mouse.
    if (m_captureMouse) {

      m_pitch = glm::clamp(m_pitch + m_mouse.movement().y * m_rotateSpeed * dt,
                           -45.0f, 45.0f);

      m_yaw += m_mouse.movement().x * m_rotateSpeed * dt;
      while (m_yaw >= 180.0f)
        m_yaw -= 360.0f;
      while (m_yaw < -180.0f)
        m_yaw += 360.0f;

      auto forward = glm::rotateY(glm::vec3{0, 0, 1}, glm::radians(m_yaw));
      auto right = glm::cross(glm::vec3{0, 1, 0}, forward);
      forward = glm::rotate(forward, glm::radians(m_pitch), right);

      auto dir = glm::vec3{};

      if (m_kb.down(GLFW_KEY_W))
        dir += forward;
      if (m_kb.down(GLFW_KEY_A))
        dir -= right;
      if (m_kb.down(GLFW_KEY_S))
        dir -= forward;
      if (m_kb.down(GLFW_KEY_D))
        dir += right;

      m_moving = (dir.x != 0.0f || dir.z != 0.0f);
      m_sprinting = m_moving && m_kb.down(GLFW_KEY_LEFT_SHIFT);

      // Update model orientation.
      if (m_moving) {
        m_targetModel.setPosition(
            m_targetModel.position() +
            glm::normalize(glm::vec3{dir.x, 0.0f, dir.z}) *
                (m_sprinting ? m_sprintSpeed : m_moveSpeed) * dt);
      }
      m_targetModel.setEuler({0.0f, glm::radians(m_yaw), 0.0f});

      // Update camera orientation.
      setTarget(m_targetModel.position() + glm::vec3{0.0f, m_yOffset, 0.0f});
      setEye(target() - m_distance * forward);
    }

    if (m_kb.pressed(GLFW_KEY_PERIOD)) {
      printf("X: %0.1f Y: %0.1f Z: %0.1f Pitch: %0.1f Yaw: %0.1f\n", eye().x,
             eye().y, eye().z, m_pitch, m_yaw);
    }

    CameraController::update(dt);
  }

  virtual ~ThirdPersonController() = default;
};
