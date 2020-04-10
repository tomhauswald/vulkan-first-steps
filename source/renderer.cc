#include "renderer.h"

void Renderer::initialize() {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.vsyncEnabled);
	m_vulkanContext.createDepthBuffer();
	m_vulkanContext.createPipeline(
		"vert-sprite",
		"frag-textured",
		VPositionColorTexcoord::binding(),
		VPositionColorTexcoord::attributes()
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

void Renderer::renderSprite(Sprite const& sprite, Camera2d const& camera) {
	
	auto const* txr = sprite.texture();
	auto requiredBatches = static_cast<size_t>(std::ceil((m_spriteCounts[txr] + 1.0f) / USpriteBatch::size));
	while (m_spriteBatches[txr].size() < requiredBatches) {
		m_spriteBatches[txr].push_back({});
	}

	auto& batch = m_spriteBatches[txr].back();
	auto index = m_spriteCounts[txr] % USpriteBatch::size;

	batch.bounds[index] = camera.screenToNdcRect(sprite.position(), sprite.size());
	batch.textureAreas[index] = sprite.textureArea();
	batch.colors[index] = sprite.color();
	batch.rotations[index / 4][index % 4] = sprite.rotation();

	m_spriteCounts[txr]++;
}

void Renderer::renderSpriteBatches() {

	for (auto const& [pTex, batches] : m_spriteBatches) {
		bindTextureSlot(0, *pTex);
		for (auto const& batch : batches) {
			setUniforms(batch);
			renderMesh(m_spriteBatchMesh);
		}
	}

	m_spriteBatches.clear();
	m_spriteCounts.clear();
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