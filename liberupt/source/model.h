#pragma once

#include "mesh.h"
#include "texture.h"

class Model {
 private:
  Mesh* m_pMesh;
  Texture* m_pTexture;
  glm::vec3 m_position = {0, 0, 0};
  glm::vec3 m_scale = {1, 1, 1};
  glm::vec3 m_euler = {0, 0, 0};

 public:
  inline Model(Mesh& mesh, Texture& texture)
      : m_pMesh(&mesh), m_pTexture(&texture) {}

  inline Mesh& mesh() noexcept { return *m_pMesh; }
  inline void setTexture(Texture& texture) noexcept { m_pTexture = &texture; }
  inline void setMesh(Mesh& mesh) noexcept { m_pMesh = &mesh; }

  GETTER(mesh, *m_pMesh)
  GETTER(texture, *m_pTexture)
  GETTER(position, m_position)
  GETTER(scale, m_scale)
  GETTER(euler, m_euler)

  SETTER(setPosition, m_position)
  SETTER(setScale, m_scale)
  SETTER(setEuler, m_euler)

  inline virtual ~Model() = default;
};
