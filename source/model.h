#include "game_object.h"
#include "mesh.h"
#include "texture.h"

#include <glm/gtx/euler_angles.hpp>

class Model : public GameObject {
private:
	Mesh& m_mesh;
	Texture const& m_texture;
	glm::vec3 m_position;
	glm::vec3 m_scale;
	glm::vec3 m_euler;

public:
	inline Model(Mesh& mesh, Texture const& texture) : 
		m_mesh{ mesh },
		m_texture{ texture },
		m_position{ 0, 0, 0 },
		m_scale{ 1, 1, 1 },
		m_euler{ 0, 0, 0 } {
	}
	
	inline void draw(Renderer& r) const override {
		r.bindTextureSlot(0, m_texture);
		r.setPushConstants(PCInstanceTransform{
			glm::translate(m_position) 
			* glm::scale(m_scale)
			* glm::eulerAngleYXZ(m_euler.y, m_euler.x, m_euler.z)
		});
		r.renderMesh(m_mesh);
	}

	GETTER(mesh, m_mesh)
	GETTER(position, m_position)
	GETTER(scale, m_scale)
	GETTER(euler, m_euler)

	SETTER(setPosition, m_position)
	SETTER(setScale, m_scale)
	SETTER(setEuler, m_euler)

	inline virtual ~Model() {}
};

