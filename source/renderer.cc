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

void test(Renderer& r) {
	std::vector<Sprite const*> pFrameSprites(USpriteBatch::size);
}

void Renderer::renderSpriteBatch(Camera2d const& camera, std::vector<Sprite> const& sprites) {

	auto batch = std::make_unique<USpriteBatch>();
	for (auto i : range(USpriteBatch::size)) {
		batch->bounds[i] = camera.screenToNdcRect(sprites[i].bounds);
		batch->textureAreas[i] = sprites[i].textureArea;
		batch->colors[i] = sprites[i].color;
		batch->rotations[i / 4][i % 4] = sprites[i].rotation;
	}

	setUniforms(*batch);
	bindTextureSlot(0, *sprites[0].pTexture);
	renderMesh(m_spriteBatchMesh);
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