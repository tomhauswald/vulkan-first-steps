#include "renderer.h"

void Renderer::initialize() {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.enableVsync);
	m_vulkanContext.createDepthBuffer();
	m_vulkanContext.createPipeline(
		m_settings.enable2d ? "vert-sprite" : "vert-textured",
		"frag-textured",
		VPositionColorTexcoord::binding(),
		VPositionColorTexcoord::attributes(),
		!m_settings.enable2d,
		m_settings.enable2d ? VK_FILTER_NEAREST : VK_FILTER_LINEAR
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
	for (auto const& layer : m_layerSpriteBatches) {
		for (auto const& [pTexture, mapEntry] : layer) {
			auto const& [numSprites, batches] = mapEntry;

			// Clear sprite batch data behind last entry in last batch.
			auto emptyBytes = (USpriteBatch::size - 1) - (numSprites % USpriteBatch::size);
			auto where = const_cast<char*>(
				reinterpret_cast<char const*>(&batches.back()) + sizeof(USpriteBatch) - emptyBytes
			);
			memset(where, 0, emptyBytes);
			
			bindTextureSlot(0, *pTexture);
			for(auto const& batch : batches) {
				setUniforms(batch);
				renderMesh(m_spriteBatchMesh);
			}
		}	
	}
	m_layerSpriteBatches = {};
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
