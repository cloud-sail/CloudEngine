#include "Engine/Renderer/Skybox.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Game/EngineBuildPreferences.hpp"

Skybox::Skybox(SkyboxConfig const& config)
	: m_config(config)
{
	if (config.m_type == SkyboxType::CUBE_MAP)
	{
		m_texture = config.m_defaultRenderer->CreateOrGetTextureFromFile(config.m_fileName.c_str());
		SpriteSheet skybox = SpriteSheet(*m_texture, IntVec2(4, 3));

		float minX = -0.5f;
		float minY = -0.5f;
		float minZ = -0.5f;
		float maxX = 0.5f;
		float maxY = 0.5f;
		float maxZ = 0.5f;

		Vec3 nnn(minX, minY, minZ);
		Vec3 nnp(minX, minY, maxZ);
		Vec3 npn(minX, maxY, minZ);
		Vec3 npp(minX, maxY, maxZ);
		Vec3 pnn(maxX, minY, minZ);
		Vec3 pnp(maxX, minY, maxZ);
		Vec3 ppn(maxX, maxY, minZ);
		Vec3 ppp(maxX, maxY, maxZ);

		AddVertsForQuad3D(m_unlitVertexs, ppn, pnn, pnp, ppp, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(5)); // +x
		AddVertsForQuad3D(m_unlitVertexs, nnn, npn, npp, nnp, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(7)); // -x
		AddVertsForQuad3D(m_unlitVertexs, npn, ppn, ppp, npp, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(4)); // +y
		AddVertsForQuad3D(m_unlitVertexs, pnn, nnn, nnp, pnp, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(6)); // -y
		AddVertsForQuad3D(m_unlitVertexs, ppp, pnp, nnp, npp, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(1)); // +z
		AddVertsForQuad3D(m_unlitVertexs, npn, nnn, pnn, ppn, Rgba8::OPAQUE_WHITE, skybox.GetSpriteUVs(9)); // -z
	}
}

void Skybox::Render(Camera const& camera, Renderer* rendererOverride /*= nullptr*/) const
{
	Renderer* renderer = m_config.m_defaultRenderer;
	if (rendererOverride)
	{
		renderer = rendererOverride;
	}

	if (m_config.m_type == SkyboxType::CUBE_MAP)
	{
		Mat44 skyBoxMat44;
		skyBoxMat44.SetTranslation3D(camera.GetPosition());
		renderer->SetModelConstants(skyBoxMat44);

#ifdef ENGINE_RENDER_D3D11
		renderer->BindTexture(m_texture);
		renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
		// resource settings
		UnlitRenderResources resources;
		resources.diffuseTextureIndex = renderer->GetSrvIndexFromLoadedTexture(m_texture);
		resources.diffuseSamplerIndex = renderer->GetDefaultSamplerIndex(SamplerMode::BILINEAR_WRAP);
		resources.cameraConstantsIndex = renderer->GetCurrentCameraConstantsIndex();
		resources.modelConstantsIndex = renderer->GetCurrentModelConstantsIndex();

		renderer->SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &resources);
#endif // ENGINE_RENDER_D3D12

		// pipeline settings 
		renderer->BindShader(nullptr);
		renderer->SetBlendMode(BlendMode::OPAQUE);
		renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		renderer->SetDepthMode(DepthMode::DISABLED);
		renderer->SetRenderTargetFormats();

		renderer->DrawVertexArray(m_unlitVertexs);
	}
}
