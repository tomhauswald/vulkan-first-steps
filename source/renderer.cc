#include "renderer.h"

void Renderer::initialize(bool mode2d) {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.vsyncEnabled);
	m_vulkanContext.createDepthBuffer();
	m_vulkanContext.createPipeline(
		mode2d ? "vert-sprite" : "vert-textured",
		"frag-textured",
		VPositionColorTexcoord::binding(),
		VPositionColorTexcoord::attributes(),
		!mode2d,
		mode2d ? VK_FILTER_NEAREST : VK_FILTER_LINEAR
	);

	auto w = m_settings.resolution.x;
	auto h = m_settings.resolution.y;
	m_viewportQuad.setVertices({
		{ {0, 0, 0}, {1,1,1}, {0,0} },
		{ {w, 0, 0}, {1,1,1}, {1,0} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {0, h, 0}, {1,1,1}, {0,1} },
		{ {0, 0, 0}, {1,1,1}, {0,0} },
	});

	auto unitQuadVertices = std::vector<VPositionColorTexcoord> {
		{ {0, 0, 0}, {1,1,1}, {0,0} },
		{ {1, 0, 0}, {1,1,1}, {1,0} },
		{ {1, 1, 0}, {1,1,1}, {1,1} },
		{ {1, 1, 0}, {1,1,1}, {1,1} },
		{ {0, 1, 0}, {1,1,1}, {0,1} },
		{ {0, 0, 0}, {1,1,1}, {0,0} }
	};
	m_unitQuad.setVertices(unitQuadVertices);

	auto spriteBatchVertices = std::vector<VPositionColorTexcoord>(
		unitQuadVertices.size() * USpriteBatch::size
	);
	
	for (auto i : range(spriteBatchVertices.size())) {
		spriteBatchVertices[i] = unitQuadVertices[i % unitQuadVertices.size()];
	}

	m_spriteBatchMesh.setVertices(std::move(spriteBatchVertices));

	glfwShowWindow(m_pWindow);
}

void Renderer::renderSpriteBatches() {
	for (auto& layer : m_layerSprites) {
		for (auto& [pTexture, sprites] : layer) {

			bindTextureSlot(0, *pTexture);

			auto numBatches = (size_t)std::ceil(sprites.size() / (float)USpriteBatch::size);
			auto pBatches = std::make_unique<USpriteBatch[]>(numBatches);

			for (auto spriteIndex : range(sprites.size())) {

				auto batchIndex = spriteIndex / USpriteBatch::size;
				auto& batch = pBatches[batchIndex];
				auto const* sprite = sprites[spriteIndex];

				auto k = spriteIndex % USpriteBatch::size;
				batch.bounds[k] = m_camera2d.worldToNdcRect(sprite->position(), sprite->size());
				batch.textureAreas[k] = sprite->textureArea();
				batch.colors[k] = sprite->color();
				batch.trigonometry[k / 2][2 * (k % 2) + 0] = glm::sin(glm::radians(sprite->rotation()));
				batch.trigonometry[k / 2][2 * (k % 2) + 1] = glm::cos(glm::radians(sprite->rotation()));
			}
			
			for (auto batchIndex : range(numBatches)) {
				setUniforms(pBatches[batchIndex]);
				renderMesh(m_spriteBatchMesh);
			}
		}
		layer.clear();
	}
}

void Renderer::createWindow() {

	// Initialize GLFW.
	crashIf(!glfwInit());

	// Create GLFW window.
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	m_pWindow = glfwCreateWindow(
		static_cast<int>(m_settings.resolution.x),
		static_cast<int>(m_settings.resolution.y),
		m_settings.windowTitle.c_str(),
		nullptr,
		nullptr
	);

	// Window creation must succeed.
	crashIf(!m_pWindow);
}