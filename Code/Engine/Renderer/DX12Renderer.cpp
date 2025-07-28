#include "Engine/Renderer/DX12Renderer.hpp"

#ifdef ENGINE_RENDER_D3D12

#include "Engine/Core/Image.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/DX12GraphicsCommon.hpp"
#include "Engine/Renderer/DX12LinearAllocator.hpp"
#include "Engine/Renderer/DX12PipelineState.hpp"
#include "Engine/Renderer/DX12DeferredReleaseQueue.hpp"

//-----------------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

//#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "ThirdParty/directx/d3dx12.h"


#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_win32.h"
#include "ThirdParty/imgui/imgui_impl_dx12.h"


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


//-----------------------------------------------------------------------------------------------
STATIC ID3D12Device* DX12Renderer::s_device = nullptr;



//-----------------------------------------------------------------------------------------------
// Simple free list based allocator
// From Dear ImGui examples
struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		IM_ASSERT(Heap == nullptr && FreeIndices.empty());
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n - 1);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		IM_ASSERT(FreeIndices.Size > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		UNUSED(gpu_idx);
		IM_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};

static ExampleDescriptorHeapAllocator g_ImGuiSrvDescHeapAlloc;






//-----------------------------------------------------------------------------------------------
DX12Renderer::DX12Renderer(RendererConfig const& config)
	: Renderer(config)
{

}

void DX12Renderer::Startup()
{
	DX12Graphics::InitializeCommonState();
	m_deferredReleaseQueue = new DX12DeferredReleaseQueue();

	CreateDevice();
	CreateFenceAndHandleAndDescriptorSizes();
	// Check 4x MSAA Quality Support
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateRenderTargetViews();
	CreateDepthStencilBufferAndView();
	CreateGraphicsRootSignatures();
	CreateTextureHeap();

	InitializeDefaultShaderAndTexture();
	InitializeFrameResources();

	ImGuiStartup();
}

void DX12Renderer::BeginFrame()
{
	// Execute the commands before the first frame begin
	if (m_currFrameResource == nullptr)
	{
		ExecuteCommandListAndWait();
	}
	// Cycle through the circular frame resource array.
	m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % FRAMES_IN_FLIGHT;
	m_currFrameResource = m_frameResources[m_currFrameResourceIndex];
	// wait until the GPU has completed commands up to this fence point
	if (m_currFrameResource->m_fenceValue != 0)
	{
		WaitForFenceValue(m_currFrameResource->m_fenceValue);
	}

	m_deferredReleaseQueue->Process(m_currFrameResource->m_fenceValue);

	m_curGraphicsRootSignature = nullptr;
	m_curComputeRootSignature = nullptr;
	m_currentPipelineState = nullptr;

	// Reset command list, current frame command allocator, current frame linear allocator
	GUARANTEE_OR_DIE(SUCCEEDED(GetCurrentCommandAllocator()->Reset()), "Cannot reset current frame command allocator");
	GUARANTEE_OR_DIE(SUCCEEDED(m_commandList->Reset(GetCurrentCommandAllocator(), nullptr)), "Could not reset the command list");
	GetCurrentLinearAllocator()->Reset();

	//-----------------------------------------------------------------------------------------------
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_textureHeap };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Fixed Primitive Topology

	// Indicate that the back buffer will now be used to render.
	CD3DX12_RESOURCE_BARRIER backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &backBufferBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
	m_commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

	SetGraphicsRootSignature(m_graphicsRootSignatures[(int)GraphicsRootSignatureType::General]);

	// Set Constants
	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::PERFRAME_CONSTANTS, sizeof(PerFrameConstants), &m_perFrameConstants);
	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::ENGINE_CONSTANTS, sizeof(EngineConstants), &m_engineConstants);

	//-----------------------------------------------------------------------------------------------
	ImGuiBeginFrame();
}

void DX12Renderer::EndFrame()
{
	ImGuiEndFrame();

	// Indicate that the back buffer will now be used to present.
	CD3DX12_RESOURCE_BARRIER barrierB = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrierB);

	uint64_t fenceSignaled = ExecuteCommandList();
	m_currFrameResource->m_fenceValue = fenceSignaled;

	HRESULT hr = m_swapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.")
	}
	//GUARANTEE_OR_DIE(SUCCEEDED(m_swapChain->Present(0, 0)), "Swap chain could not present");
	m_currBackBuffer = m_swapChain->GetCurrentBackBufferIndex(); //m_currBackBuffer = (m_currBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void DX12Renderer::Shutdown()
{
	//-----------------------------------------------------------------------------------------------
	// Wait for all commands are processed
	FlushCommandQueue();
	m_swapChain->SetFullscreenState(FALSE, nullptr);
	ImGuiShutdown();

	CloseHandle(m_fenceEvent);
	//-----------------------------------------------------------------------------------------------
	s_device = nullptr;
	DestroyFrameResources();
	DX_SAFE_RELEASE(m_commandQueue);
	DX_SAFE_RELEASE(m_mainCommandAllocator);
	DX_SAFE_RELEASE(m_commandList);

	for (int i = 0; i < (int)GraphicsRootSignatureType::Count; ++i)
	{
		DX_SAFE_RELEASE(m_graphicsRootSignatures[i]);
	}

	delete m_desiredGraphicsPSO;
	m_desiredGraphicsPSO = nullptr;

	delete m_deferredReleaseQueue;
	m_deferredReleaseQueue = nullptr;

	PSO::DestroyAll();

	//-----------------------------------------------------------------------------------------------
	for (int textureIndex = 0; textureIndex < (int)m_loadedTextures.size(); ++textureIndex)
	{
		delete m_loadedTextures[textureIndex];
		m_loadedTextures[textureIndex] = nullptr;
	}
	m_defaultTexture = nullptr;
	m_defaultNormalTexture = nullptr;
	m_defaultSpecGlossEmitTexture = nullptr;

	for (int fontIndex = 0; fontIndex < (int)m_loadedFonts.size(); ++fontIndex)
	{
		delete m_loadedFonts[fontIndex];
		m_loadedFonts[fontIndex] = nullptr;
	}

	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		delete m_loadedShaders[shaderIndex];
		m_loadedShaders[shaderIndex] = nullptr;
	}
	m_defaultShader = nullptr;
	//-----------------------------------------------------------------------------------------------

	DX_SAFE_RELEASE(m_textureHeap);
	DX_SAFE_RELEASE(m_ImGuiSrvDescHeap);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_swapChainBuffer[i]);
	}
	DX_SAFE_RELEASE(m_depthStencilBuffer);

	DX_SAFE_RELEASE(m_swapChain);

	DX_SAFE_RELEASE(m_rtvHeap);
	DX_SAFE_RELEASE(m_dsvHeap);

	DX_SAFE_RELEASE(m_dxgiFactory);

	DX_SAFE_RELEASE(m_fence);

	DX12Graphics::DestroyCommonState();



//#if defined(ENGINE_DEBUG_RENDER)
//	ID3D12DebugDevice* debugInterface;
//	if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugInterface))))
//	{
//		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
//		debugInterface->Release();
//	}
//#endif

	DX_SAFE_RELEASE(m_device);

#if defined(ENGINE_DEBUG_RENDER)

	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		//pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		HRESULT hr = pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		if (FAILED(hr))
		{
			ERROR_AND_DIE("FAILED to report live objects");
		}
		pDebug->Release();
	}
#endif
}

void DX12Renderer::ClearScreen(Rgba8 const& clearColor)
{
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
	m_commandList->ClearRenderTargetView(rtvHandle, colorAsFloats, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DX12Renderer::BeginCamera(Camera const& camera)
{
	SetModelConstants();

	// Set Camera Constants
	CameraConstants cameraConstants;
	cameraConstants.WorldToCameraTransform = camera.GetWorldToCameraTransform();
	cameraConstants.CameraToRenderTransform = camera.GetCameraToRenderTransform();
	cameraConstants.RenderToClipTransform = camera.GetRenderToClipTransform();
	cameraConstants.CameraWorldPosition = camera.GetPosition();

	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::CAMERA_CONSTANTS, sizeof(CameraConstants), &cameraConstants);

	D3D12_VIEWPORT viewport = {};
	camera.GetDirectXViewport(Vec2(m_config.m_window->GetClientDimensions()),
		viewport.TopLeftX,
		viewport.TopLeftY,
		viewport.Width,
		viewport.Height);

	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	D3D12_RECT scissorRect = CD3DX12_RECT(0, 0, m_config.m_window->GetClientDimensions().x, m_config.m_window->GetClientDimensions().y);

	SetViewport(viewport);
	SetScissor(scissorRect);
}

void DX12Renderer::EndCamera(Camera const& camera)
{
	// DO nothing for now
	UNUSED(camera);
}

VertexBuffer* DX12Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return new VertexBuffer(this, m_device, size, stride);
}

IndexBuffer* DX12Renderer::CreateIndexBuffer(const unsigned int size)
{
	return new IndexBuffer(this, m_device, size);
}

ConstantBuffer* DX12Renderer::CreateConstantBuffer(const unsigned int size)
{
	UNUSED(size);
	ERROR_AND_DIE("Not implemented");
}

void DX12Renderer::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo)
{
	vbo->Upload(m_commandList, data, size);
}

void DX12Renderer::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo)
{
	ibo->Upload(m_commandList, data, size);
}

void DX12Renderer::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo)
{
	UNUSED(data);
	UNUSED(size);
	UNUSED(cbo);
	ERROR_AND_DIE("Not implemented");
}

void DX12Renderer::DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes)
{
	SetDynamicVertexBuffer(0, numVertexes, sizeof(Vertex_PCU), vertexes);
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	Draw((unsigned int)numVertexes);
}

void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& verts)
{
	SetDynamicVertexBuffer(0, verts.size(), sizeof(Vertex_PCU), verts.data());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	Draw((unsigned int)verts.size());
}

void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts)
{
	SetDynamicVertexBuffer(0, verts.size(), sizeof(Vertex_PCUTBN), verts.data());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	Draw((unsigned int)verts.size());
}

void DX12Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes)
{
	SetDynamicVertexBuffer(0, verts.size(), sizeof(Vertex_PCU), verts.data());
	SetDynamicIndexBuffer(indexes.size(), indexes.data());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	DrawIndexed((unsigned int)indexes.size());
}

void DX12Renderer::DrawIndexedVertexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes)
{
	SetDynamicVertexBuffer(0, verts.size(), sizeof(Vertex_PCUTBN), verts.data());
	SetDynamicIndexBuffer(indexes.size(), indexes.data());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	DrawIndexed((unsigned int)indexes.size());
}

void DX12Renderer::DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount)
{
	SetVertexBuffer(0, vbo->GetVertexBufferView());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	Draw(vertexCount);
}

void DX12Renderer::DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount)
{
	SetVertexBuffer(0, vbo->GetVertexBufferView());
	SetIndexBuffer(ibo->GetIndexBufferView());
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	DrawIndexed(indexCount);
}

void DX12Renderer::BindTexture(Texture const* texture, int slot /*= 0*/)
{
	// TODO bindless texture
	if (slot == 0)
	{
		if (texture == nullptr) texture = m_defaultTexture;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_0, gpuHandle);
	}
	else if (slot == 1)
	{
		if (texture == nullptr) texture = m_defaultNormalTexture;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_1, gpuHandle);
	}
	else if (slot == 2)
	{
		if (texture == nullptr) texture = m_defaultSpecGlossEmitTexture;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_2, gpuHandle);
	}
	else if (slot == 9)
	{
		if (texture == nullptr)
		{
			ERROR_AND_DIE("Texture Cube cannot be set to default now");
		};
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_CUBE, gpuHandle);
	}
}

void DX12Renderer::BindShader(Shader* shader)
{
	if (shader == nullptr)
	{
		shader = m_defaultShader;
	}
	m_desiredGraphicsPSO->SetShader(shader);
}

void DX12Renderer::SetBlendMode(BlendMode blendMode)
{
	m_desiredGraphicsPSO->SetBlendMode(blendMode);
}

void DX12Renderer::SetSamplerMode(SamplerMode samplerMode, int slot /*= 0*/)
{
	// we are using static sampler in directx 12
	UNUSED(samplerMode);
	UNUSED(slot);
}

void DX12Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_desiredGraphicsPSO->SetRasterizerMode(rasterizerMode);
}

void DX12Renderer::SetDepthMode(DepthMode depthMode)
{
	m_desiredGraphicsPSO->SetDepthMode(depthMode);
}

void DX12Renderer::SetEngineConstants(int debugInt /*= 0*/, float debugFloat /*= 0.f*/)
{
	m_engineConstants.m_debugInt = debugInt;
	m_engineConstants.m_debugFloat = debugFloat;
	if (IsBeforeTheFirstFrame()) return;
	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::ENGINE_CONSTANTS, sizeof(EngineConstants), &m_engineConstants);
}

void DX12Renderer::SetModelConstants(Mat44 const& modelToWorldTransform /*= Mat44()*/, Rgba8 const& modelColor /*= Rgba8::OPAQUE_WHITE*/)
{
	ModelConstants modelConstants;

	modelConstants.ModelToWorldTransform = modelToWorldTransform;
	modelColor.GetAsFloats(modelConstants.ModelColor);

	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::MODEL_CONSTANTS, sizeof(ModelConstants), &modelConstants);
}

void DX12Renderer::SetLightConstants(Vec3 const& sunDirection /*= Vec3(2.f, -1.f, -1.f)*/, float sunIntensity /*= 0.85f*/, float ambientIntensity /*= 0.35f*/)
{
	// Deprecated: It only keeps sun light
	UNUSED(ambientIntensity);
	Rgba8 color = Rgba8::OPAQUE_WHITE;
	color.ScaleAlpha(sunIntensity);
	color.GetAsFloats(m_lightConstants.m_sunColor);
	m_lightConstants.m_sunNormal = sunDirection.GetNormalized();
	if (IsBeforeTheFirstFrame()) return;

	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
}

void DX12Renderer::SetLightConstants(LightConstants const& lightConstants)
{
	m_lightConstants = lightConstants;
	if (IsBeforeTheFirstFrame()) return;

	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
}

void DX12Renderer::SetPerFrameConstants(PerFrameConstants const& perframeConstants)
{
	m_perFrameConstants = perframeConstants;
	if (IsBeforeTheFirstFrame()) return;

	SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::PERFRAME_CONSTANTS, sizeof(PerFrameConstants), &m_perFrameConstants);
}

Texture* DX12Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}
	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}


Texture* DX12Renderer::CreateOrGetTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(config.m_name.c_str());
	if (existingTexture)
	{
		return existingTexture;
	}
	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureCubeFromSixFaces(config);
	return newTexture;
}

BitmapFont* DX12Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	BitmapFont* existingBitmapFont = GetBitmapFont(bitmapFontFilePathWithNoExtension);
	if (existingBitmapFont)
	{
		return existingBitmapFont;
	}
	BitmapFont* newBitmapFont = CreateBitmapFont(bitmapFontFilePathWithNoExtension);
	return newBitmapFont;
}

Shader* DX12Renderer::CreateOrGetShader(char const* shaderName, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	Shader* existingShader = GetShader(shaderName);
	if (existingShader)
	{
		return existingShader;
	}
	Shader* newShader = CreateShader(shaderName, type);
	return newShader;
}

Shader* DX12Renderer::CreateOrGetShader(ShaderConfig const& config, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	Shader* existingShader = GetShader(config.m_name.c_str());
	if (existingShader)
	{
		return existingShader;
	}
	Shader* newShader = CreateShader(config, type);
	return newShader;
}

void DX12Renderer::AttachGeometryShader(Shader* shader, char const* shaderName)
{
	std::string shaderFilePath(shaderName);
	shaderFilePath = shaderFilePath + ".hlsl";
	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderFilePath);
	GUARANTEE_RECOVERABLE(result >= 0, "Could not read shader.");

	bool isSucceed;
	isSucceed = CompileShaderToByteCode_D3DCompile(shader->m_geometryShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource.c_str(), (shader->m_config.m_geometryEntryPoint).c_str(), "gs_5_1");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile geometry shader.");
}

Texture* DX12Renderer::GetTextureForFileName(char const* imageFilePath)
{
	for (int i = 0; i < static_cast<int>(m_loadedTextures.size()); ++i)
	{
		if (m_loadedTextures[i]->m_name == imageFilePath)
		{
			return m_loadedTextures[i];
		}
	}
	return nullptr;
}

Texture* DX12Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image fileImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(fileImage);
	return newTexture;
}

Texture* DX12Renderer::CreateTextureFromImage(Image const& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_name = image.GetImageFilePath();
	newTexture->m_dimensions = image.GetDimensions();
	newTexture->m_srvHeapIndex = static_cast<uint32_t>(m_loadedTextures.size());

	// 1. create default heap
	D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		image.GetDimensions().x,
		image.GetDimensions().y,
		1,
		1	// mipLevels
	);

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&newTexture->m_texture)
	);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture default heap");
#ifdef _DEBUG
	newTexture->m_texture->SetName(L"Texture default heap");
#endif // _DEBUG

	// 2. create temp upload heap
	ID3D12Resource* textureUploadHeap = nullptr;
	UINT64 uploadBufferSize = 0;
	m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	D3D12_HEAP_PROPERTIES heapProp1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC textureDesc1 = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	hr = m_device->CreateCommittedResource(
		&heapProp1,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc1,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture upload heap");
	
	textureUploadHeap->SetName(L"Texture upload heap");

	// 3. upload data
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = image.GetRawData();
	textureData.RowPitch = image.GetDimensions().x * 4; // RGBA8
	textureData.SlicePitch = textureData.RowPitch * image.GetDimensions().y;

	UpdateSubresources(m_commandList, newTexture->m_texture, textureUploadHeap, 0, 0, 1, &textureData);

	// 4. Transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		newTexture->m_texture,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// release temp upload heap
	EnqueueDeferredRelease(textureUploadHeap);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1; // same as desc.miplevels


	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_textureHeap->GetCPUDescriptorHandleForHeapStart(),
		newTexture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);

	m_device->CreateShaderResourceView(
		newTexture->m_texture,
		&srvDesc,
		cpuHandle
	);

	// Usage
	// ID3D12DescriptorHeap* heaps[] = { m_textureHeap };
	// m_commandList->SetDescriptorHeaps(1, heaps);
	//CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart(),
	//	newTexture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
	// m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuHandle);

	m_loadedTextures.push_back(newTexture);


	return newTexture;
}

Texture* DX12Renderer::CreateTextureCubeFromSixFaces(TextureCubeSixFacesConfig const& config)
{
	Image images[6] = {
		Image(config.m_rightImageFilePath.c_str(), false),
		Image(config.m_leftImageFilePath.c_str(), false),
		Image(config.m_upImageFilePath.c_str(), false),
		Image(config.m_downImageFilePath.c_str(), false),
		Image(config.m_forwardImageFilePath.c_str(), false),
		Image(config.m_backwardImageFilePath.c_str(), false)
	};
	
	GUARANTEE_OR_DIE(images[0].GetDimensions() == images[1].GetDimensions() &&
		images[1].GetDimensions() == images[2].GetDimensions() &&
		images[2].GetDimensions() == images[3].GetDimensions() &&
		images[3].GetDimensions() == images[4].GetDimensions() &&
		images[4].GetDimensions() == images[5].GetDimensions(),
		"Texture Cube Six Faces do not have same dimensions");

	int width = images[0].GetDimensions().x;
	int height = images[0].GetDimensions().y;
	GUARANTEE_OR_DIE(width == height, "Texture Cube face is not a square");

	Texture* newTexture = new Texture();
	newTexture->m_name = config.m_name;
	newTexture->m_srvHeapIndex = static_cast<uint32_t>(m_loadedTextures.size());
	newTexture->m_dimensions = images[0].GetDimensions();

	// 1. Create Texture Desc and Default Heap
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 6;
	textureDesc.MipLevels = 1; // 1 is no mipmap 0 is auto
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&newTexture->m_texture)
	);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture cube default heap");
#ifdef _DEBUG
	newTexture->m_texture->SetName(L"Texture cube default heap");
#endif // _DEBUG

	// 2. create temp upload heap
	UINT64 uploadBufferSize = 0;
	m_device->GetCopyableFootprints(&textureDesc, 0, 6, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	ID3D12Resource* textureUploadHeap = nullptr;
	D3D12_HEAP_PROPERTIES heapProp1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC textureDesc1 = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	hr = m_device->CreateCommittedResource(
		&heapProp1,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc1,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture cube upload heap");

	textureUploadHeap->SetName(L"Texture cube upload heap");

	// 3. fill data
	D3D12_SUBRESOURCE_DATA subresources[6];
	for (int i = 0; i < 6; ++i) 
	{
		subresources[i].pData = images[i].GetRawData();
		subresources[i].RowPitch = width * 4; // 4 = byte per pixel
		subresources[i].SlicePitch = subresources[i].RowPitch * height;
	}

	// 4. copy data to the upload heap
	UpdateSubresources(m_commandList, newTexture->m_texture, textureUploadHeap, 0, 0, 6, subresources);

	// 5. Transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		newTexture->m_texture,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// release temp upload heap
	EnqueueDeferredRelease(textureUploadHeap);

	// 6. Create SRV and put it into the heap
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	//srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1; // same as desc.miplevels
	//srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_textureHeap->GetCPUDescriptorHandleForHeapStart(),
		newTexture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);

	m_device->CreateShaderResourceView(
		newTexture->m_texture,
		&srvDesc,
		cpuHandle
	);

	m_loadedTextures.push_back(newTexture);

	return newTexture;
}

BitmapFont* DX12Renderer::GetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	for (int i = 0; i < static_cast<int>(m_loadedFonts.size()); ++i)
	{
		if (m_loadedFonts[i]->m_fontFilePathNameWithNoExtension == bitmapFontFilePathWithNoExtension)
		{
			return m_loadedFonts[i];
		}
	}
	return nullptr;
}

BitmapFont* DX12Renderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	std::string imageFilePath(bitmapFontFilePathWithNoExtension);
	imageFilePath += ".png";
	Texture* fontTexture = CreateOrGetTextureFromFile(imageFilePath.c_str());
	BitmapFont* newFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture);
	m_loadedFonts.push_back(newFont);
	return newFont;
}

Shader* DX12Renderer::GetShader(char const* shaderName)
{
	for (int i = 0; i < static_cast<int>(m_loadedShaders.size()); ++i)
	{
		if (m_loadedShaders[i]->m_config.m_name == shaderName)
		{
			return m_loadedShaders[i];
		}
	}
	return nullptr;
}

Shader* DX12Renderer::CreateShader(char const* shaderName, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	std::string shaderFilePath(shaderName);
	shaderFilePath = shaderFilePath + ".hlsl"; // e.g. "Data/Shaders/BlinnPhong" + ".hlsl"
	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderFilePath);
	GUARANTEE_RECOVERABLE(result >= 0, "Could not read shader.");
	return CreateShader(shaderName, shaderSource.c_str(), type);
}

Shader* DX12Renderer::CreateShader(char const* shaderName, char const* shaderSource, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	ShaderConfig shaderConfig;
	shaderConfig.m_name = shaderName;
	Shader* shader = new Shader(shaderConfig);

	shader->m_inputLayoutMode = type;

	bool isSucceed;
	isSucceed = CompileShaderToByteCode_D3DCompile(shader->m_vertexShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource, (shader->m_config.m_vertexEntryPoint).c_str(), "vs_5_1");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile vertex shader.");


	isSucceed = CompileShaderToByteCode_D3DCompile(shader->m_pixelShaderByteCode, (shader->m_config.m_name).c_str(), shaderSource, (shader->m_config.m_pixelEntryPoint).c_str(), "ps_5_1");
	GUARANTEE_RECOVERABLE(isSucceed, "Could not compile pixel shader.");

	m_loadedShaders.push_back(shader);

	return shader;
}

Shader* DX12Renderer::CreateShader(ShaderConfig const& config, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	std::string shaderFilePath = config.m_name + ".hlsl";
	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderFilePath);
	GUARANTEE_RECOVERABLE(result >= 0, "Could not read shader.");

	return CreateShader(config, shaderSource.c_str(), type);
}

Shader* DX12Renderer::CreateShader(ShaderConfig const& config, char const* shaderSource, VertexType type /*= VertexType::VERTEX_PCU*/)
{
	Shader* shader = new Shader(config);
	shader->m_inputLayoutMode = type;

	if (config.m_stages & SHADER_STAGE_VS)
	{
		bool ok = CompileShaderToByteCode(shader->m_vertexShaderByteCode, config.m_name.c_str(), shaderSource, (shader->m_config.m_vertexEntryPoint).c_str(), config.m_shaderModelVS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile vertex shader.");
	}

	if (config.m_stages & SHADER_STAGE_PS)
	{
		bool ok = CompileShaderToByteCode(shader->m_pixelShaderByteCode, config.m_name.c_str(), shaderSource, (shader->m_config.m_pixelEntryPoint).c_str(), config.m_shaderModelPS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile vertex shader.");
	}

	if (config.m_stages & SHADER_STAGE_GS) {
		bool ok = CompileShaderToByteCode(shader->m_geometryShaderByteCode, config.m_name.c_str(), shaderSource, config.m_geometryEntryPoint.c_str(), config.m_shaderModelGS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile geometry shader.");
	}

	if (config.m_stages & SHADER_STAGE_HS) {
		bool ok = CompileShaderToByteCode(shader->m_hullShaderByteCode, config.m_name.c_str(), shaderSource, config.m_hullEntryPoint.c_str(), config.m_shaderModelHS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile hull shader.");
	}

	if (config.m_stages & SHADER_STAGE_DS) {
		bool ok = CompileShaderToByteCode(shader->m_domainShaderByteCode, config.m_name.c_str(), shaderSource, config.m_domainEntryPoint.c_str(), config.m_shaderModelDS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile domain shader.");
	}
	// CS MS AS

	m_loadedShaders.push_back(shader);

	return shader;
}

bool DX12Renderer::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, const char* name, const char* source, const char* entryPoint, const char* target, ShaderCompilerType compilerType)
{
	if (compilerType == SHADER_COMPILER_D3DCOMPILE)
	{
		return CompileShaderToByteCode_D3DCompile(outByteCode, name, source, entryPoint, target);
	}
	else if (compilerType == SHADER_COMPILER_DXC)
	{
		return CompileShaderToByteCode_DXC(outByteCode, name, source, entryPoint, target);
	}
	return false;
}

bool DX12Renderer::CompileShaderToByteCode_D3DCompile(std::vector<unsigned char>& outByteCode, char const* name, char const* source, char const* entryPoint, char const* target)
{
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	HRESULT hr;
	hr = D3DCompile(
		source, strlen(source),
		name, nullptr, nullptr,
		entryPoint, target, shaderFlags,
		0, &shaderBlob, &errorBlob);
	if (SUCCEEDED(hr))
	{
		outByteCode.resize(shaderBlob->GetBufferSize());
		memcpy(
			outByteCode.data(),
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize()
		);
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
		}
		ERROR_AND_DIE(Stringf("Could not compile shader."));
	}

	shaderBlob->Release();
	if (errorBlob != NULL)
	{
		errorBlob->Release();
	}

	return true;
}

bool DX12Renderer::CompileShaderToByteCode_DXC(std::vector<unsigned char>& outByteCode, const char* name, const char* source, const char* entryPoint, const char* target)
{
	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll 
	HRESULT hr;
	IDxcUtils* pUtils = nullptr;
	IDxcCompiler3* pCompiler = nullptr;
	IDxcBlobEncoding* pSourceBlob = nullptr;
	IDxcIncludeHandler* pIncludeHandler = nullptr;
	IDxcResult* pResults = nullptr;
	IDxcBlob* pShader = nullptr;
	IDxcBlobUtf8* pErrors = nullptr;

	//
	// Create compiler and utils.
	//
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create DXC Utils");

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create DXC Compiler");

	//
	// Create default include handler. (You can create your own...)
	//
	hr = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create DXC Compiler");

	size_t sourceLength = strlen(source);
	hr = pUtils->CreateBlob(source, (UINT32)sourceLength, DXC_CP_UTF8, &pSourceBlob);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create source blob");

	// Assume all char is in ascii table
	//std::wstring wEntryPoint(entryPoint, entryPoint + strlen(entryPoint));
	//std::wstring wTarget(target, target + strlen(target));
	//std::wstring wName(name, name + strlen(name));

	std::wstring wEntryPoint{ ToWString(entryPoint) };
	std::wstring wTarget{ ToWString(target) };
	std::wstring wName{ ToWString(name) };

	//
	// COMMAND LINE:
	// dxc Default -E VertexMain -T vs_6_0 -Zi ...
	//
	LPCWSTR args[] =
	{
		wName.c_str(),					// Optional shader source file name for error reporting
										// and for PIX shader source view.  
		L"-E", wEntryPoint.c_str(),		// Entry point.
		L"-T", wTarget.c_str(),			// Target.
		//DXC_ARG_ALL_RESOURCES_BOUND,  // Allow more aggressive optimization, no safe values for reading unbounded resource
#if defined(ENGINE_DEBUG_RENDER)
		DXC_ARG_DEBUG,
		DXC_ARG_SKIP_OPTIMIZATIONS,
#else
		DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
		DXC_ARG_WARNINGS_ARE_ERRORS,

		L"-Qstrip_reflect",				// Strip reflection into a separate blob
		L"-Qstrip_debug",				// Strip debug information into a separate blob
	};

	DebuggerPrintf("Compiling ");
	DebuggerPrintf(name);


	//
	// Open source file.  
	//
	//CComPtr<IDxcBlobEncoding> pSource = nullptr;
	//pUtils->LoadFile(L"myshader.hlsl", nullptr, &pSource);
	//DxcBuffer Source;
	//Source.Ptr = pSource->GetBufferPointer();
	//Source.Size = pSource->GetBufferSize();
	//Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.


	//
	// Compile it with specified arguments.
	//
	DxcBuffer srcBuffer = {};
	srcBuffer.Ptr = pSourceBlob->GetBufferPointer();
	srcBuffer.Size = pSourceBlob->GetBufferSize();
	//srcBuffer.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.
	srcBuffer.Encoding = DXC_CP_UTF8;

	hr = pCompiler->Compile(
		&srcBuffer,
		args, _countof(args),
		pIncludeHandler,
		IID_PPV_ARGS(&pResults)
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not compile the shader"); // just means not fatal error

	//
	// Print errors if present.
	//
	hr = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not get compile result output");
	// IDxcCompiler3::Compile will always return an error buffer, but its length
	// will be zero if there are no warnings or errors.
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		DebuggerPrintf("\nDXC shader compilation error: \n");
		DebuggerPrintf((char*)pErrors->GetStringPointer());
	}
	else
	{
		DebuggerPrintf(" [ Succeeded ]");
	}
	DebuggerPrintf("\n");

	//
	// Quit if the compilation failed.
	//
	HRESULT hrStatus;
	hr = pResults->GetStatus(&hrStatus);
	GUARANTEE_OR_DIE(SUCCEEDED(hrStatus) && SUCCEEDED(hr), "Shader Compilation Failed");


	hr = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
	GUARANTEE_OR_DIE(SUCCEEDED(hr) && pShader != nullptr, "Could not read shader binary");

	outByteCode.resize(pShader->GetBufferSize());
	memcpy(
		outByteCode.data(),
		pShader->GetBufferPointer(),
		pShader->GetBufferSize()
	);

	if (pShader) pShader->Release();
	if (pErrors) pErrors->Release();
	if (pResults) pResults->Release();
	if (pIncludeHandler) pIncludeHandler->Release();
	if (pSourceBlob) pSourceBlob->Release();
	if (pCompiler) pCompiler->Release();
	if (pUtils) pUtils->Release();

	return true;
}

void DX12Renderer::CreateDevice()
{
	UINT dxgiFactoryFlags = 0;

#if defined(ENGINE_DEBUG_RENDER)
	//-----------------------------------------------------------------------------------------------
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ID3D12Debug* debugController;

		if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			ERROR_AND_DIE("Could not Get Debug Interface.");
		}
		debugController->EnableDebugLayer();
		// Enable additional debug layers.
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		DX_SAFE_RELEASE(debugController);
	}
#endif

	HRESULT factoryResult = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
	if (FAILED(factoryResult))
	{
		ERROR_AND_DIE("Could not create DXGI Factory.");
	}

	// Try to create hardware device.
	IDXGIFactory6* factory6;
	IDXGIAdapter1* adapter;
	if (FAILED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		ERROR_AND_DIE("Could not create temp factory.");
	}
	if (FAILED(factory6->EnumAdapterByGpuPreference(
		0, // best practice is doing for loop
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&adapter))))
	{
		ERROR_AND_DIE("Cound not find the GPU for adaptor.");
	}

	HRESULT hardwareResult = D3D12CreateDevice(
		adapter, // default adapter = nullptr
		D3D_FEATURE_LEVEL_11_0, //D3D_FEATURE_LEVEL_12_2,
		IID_PPV_ARGS(&m_device));

	if (FAILED(hardwareResult))
	{
		ERROR_AND_DIE("Could not create D3D12 device.");
	}
	DX_SAFE_RELEASE(factory6);
	DX_SAFE_RELEASE(adapter);

	s_device = m_device;
}

void DX12Renderer::CreateFenceAndHandleAndDescriptorSizes()
{
	HRESULT hr;

	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create the fence.")
	}

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ERROR_AND_DIE("Could not create a fence event");
	}

	m_rtvDescriptorSize			= m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize			= m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvUavDescriptorSize	= m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

void DX12Renderer::CreateCommandObjects()
{
	HRESULT hr;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create the command queue.");
	}

	// Create the command allocator
	hr = m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		IID_PPV_ARGS(&m_mainCommandAllocator));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create command allocator.");
	}

	hr = m_device->CreateCommandList(
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		m_mainCommandAllocator, 
		nullptr, // Initial PSO is nullptr
		IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create the command list.")
	}
	// Notes: the command list is not closed now, we will close it in the first begin frame
}

void DX12Renderer::CreateSwapChain()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_config.m_window->GetClientDimensions().x;
	swapChainDesc.Height = m_config.m_window->GetClientDimensions().y;
	swapChainDesc.Format = DX12Graphics::BackBufferFormat;
	//swapChainDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//swapChainDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; 

	IDXGISwapChain1* tempSwapChain1 = nullptr;
	hr = m_dxgiFactory->CreateSwapChainForHwnd(
		m_commandQueue,
		(HWND)m_config.m_window->GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&tempSwapChain1);
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create swap chain.");
	}

	hr = tempSwapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not Query Interface from swap chain.");
	}
	DX_SAFE_RELEASE(tempSwapChain1);
	m_currBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::CreateDescriptorHeaps()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create RTV Discriptor Heap.");
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create DSV Discriptor Heap.");
	}
}

void DX12Renderer::CreateRenderTargetViews()
{
	HRESULT hr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (unsigned int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		//DX_SAFE_RELEASE(m_swapChainBuffer[i]); // Remember this if we resize
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i]));
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Could not get render target from swap chain.");
		}
		m_device->CreateRenderTargetView(m_swapChainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}

void DX12Renderer::CreateDepthStencilBufferAndView()
{
	HRESULT hr;

	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_config.m_window->GetClientDimensions().x;
	depthStencilDesc.Height = m_config.m_window->GetClientDimensions().y;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DX12Graphics::DepthStencilFormat; // If using SSAO, depth stencil needs to be changed
	//depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = DX12Graphics::DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //D3D12_RESOURCE_STATE_COMMON need to Transition from common to depth write
		&optClear,
		IID_PPV_ARGS(&m_depthStencilBuffer));

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create resource for depth stencil buffer.");
	}

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	//m_device->CreateDepthStencilView(
	//	m_depthStencilBuffer,
	//	nullptr, // This behavior inherits the resource format and dimension(if not typeless)
	//	GetDepthStencilView());

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DX12Graphics::DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_device->CreateDepthStencilView(m_depthStencilBuffer, &dsvDesc, GetDepthStencilView());
}

void DX12Renderer::CreateGraphicsRootSignatures()
{
	// Root Signature 1.1

	CD3DX12_DESCRIPTOR_RANGE1 textureTable0;
	textureTable0.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

	CD3DX12_DESCRIPTOR_RANGE1 textureTable1;
	textureTable1.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

	CD3DX12_DESCRIPTOR_RANGE1 textureTable2;
	textureTable2.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

	CD3DX12_DESCRIPTOR_RANGE1 textureTable9;
	textureTable9.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

	CD3DX12_ROOT_PARAMETER1 slotRootParameters[(int)GeneralRootBinding::NUM];

	slotRootParameters[(int)GeneralRootBinding::MODEL_CONSTANTS]
		.InitAsConstantBufferView(k_modelConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_0]
		.InitAsDescriptorTable(1, &textureTable0, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_1]
		.InitAsDescriptorTable(1, &textureTable1, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_2]
		.InitAsDescriptorTable(1, &textureTable2, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::CAMERA_CONSTANTS]
		.InitAsConstantBufferView(k_cameraConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::PERFRAME_CONSTANTS]
		.InitAsConstantBufferView(k_perFrameConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::LIGHT_CONSTANTS]
		.InitAsConstantBufferView(k_lightConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::ENGINE_CONSTANTS]
		.InitAsConstantBufferView(k_engineConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);

	slotRootParameters[(int)GeneralRootBinding::TEXTURE_CUBE]
		.InitAsDescriptorTable(1, &textureTable9, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init_1_1(
		(UINT)GeneralRootBinding::NUM,
		slotRootParameters,
		(UINT)g_numStaticSamplers,
		g_staticSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
	);

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;

	//D3DX12SerializeVersionedRootSignature
	HRESULT hr = D3D12SerializeVersionedRootSignature(
		&rootSigDesc,
		&serializedRootSig,
		&errorBlob
	);

	if (errorBlob != nullptr)
	{
		DebuggerPrintf((char*)errorBlob->GetBufferPointer());
	}
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not serialize graphics root signature");

	hr = m_device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_graphicsRootSignatures[(int)GraphicsRootSignatureType::General])
	);

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create graphics root signature");
	}

	DX_SAFE_RELEASE(serializedRootSig);
	DX_SAFE_RELEASE(errorBlob);

	m_desiredGraphicsPSO = new GraphicsPSO();
}



//void DX12Renderer::CreateGraphicsRootSignatures()
//{
//	// Notes: Just temp solution for texture slot binding
//	// TODO check root signature 1.1
//	//-----------------------------------------------------------------------------------------------
//	// General
//	//-----------------------------------------------------------------------------------------------
//	// Descriptor Range
//	CD3DX12_DESCRIPTOR_RANGE textureTable0;
//	textureTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE textureTable1;
//	textureTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
//
//	CD3DX12_DESCRIPTOR_RANGE textureTable2;
//	textureTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
//
//	CD3DX12_DESCRIPTOR_RANGE textureTable9;
//	textureTable9.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9);
//
//	// Root Parameter
//	CD3DX12_ROOT_PARAMETER slotRootParameters[(int)GeneralRootBinding::NUM];
//
//	// Bind
//	slotRootParameters[(int)GeneralRootBinding::MODEL_CONSTANTS]
//		.InitAsConstantBufferView(k_modelConstantsSlot, 0, D3D12_SHADER_VISIBILITY_ALL); // b3
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_0]
//		.InitAsDescriptorTable(1, &textureTable0, D3D12_SHADER_VISIBILITY_ALL); // t0
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_1]
//		.InitAsDescriptorTable(1, &textureTable1, D3D12_SHADER_VISIBILITY_ALL); // t1
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_2]
//		.InitAsDescriptorTable(1, &textureTable2, D3D12_SHADER_VISIBILITY_ALL); // t2
//
//	slotRootParameters[(int)GeneralRootBinding::CAMERA_CONSTANTS]
//		.InitAsConstantBufferView(k_cameraConstantsSlot, 0, D3D12_SHADER_VISIBILITY_ALL); // b2
//
//	slotRootParameters[(int)GeneralRootBinding::PERFRAME_CONSTANTS]
//		.InitAsConstantBufferView(k_perFrameConstantsSlot, 0, D3D12_SHADER_VISIBILITY_ALL); // b1
//
//	slotRootParameters[(int)GeneralRootBinding::LIGHT_CONSTANTS]
//		.InitAsConstantBufferView(k_lightConstantsSlot, 0, D3D12_SHADER_VISIBILITY_ALL); // b4
//
//	slotRootParameters[(int)GeneralRootBinding::ENGINE_CONSTANTS]
//		.InitAsConstantBufferView(k_engineConstantsSlot, 0, D3D12_SHADER_VISIBILITY_ALL); // b0
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_CUBE]
//		.InitAsDescriptorTable(1, &textureTable9, D3D12_SHADER_VISIBILITY_ALL); // t9
//
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
//	rootSigDesc.Init(
//		(UINT)GeneralRootBinding::NUM,
//		slotRootParameters,
//		(UINT)g_numStaticSamplers,
//		g_staticSamplers,
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
//		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
//		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);
//
//	ID3DBlob* serializedRootSig = nullptr;
//	ID3DBlob* errorBlob = nullptr;
//
//	HRESULT hr = D3D12SerializeRootSignature(
//		&rootSigDesc,
//		D3D_ROOT_SIGNATURE_VERSION_1,
//		&serializedRootSig,
//		&errorBlob);
//
//	if (errorBlob != nullptr)
//	{
//		DebuggerPrintf((char*)errorBlob->GetBufferPointer());
//	}
//	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not serialize graphics root signature");
//
//	hr = m_device->CreateRootSignature(
//		0,
//		serializedRootSig->GetBufferPointer(),
//		serializedRootSig->GetBufferSize(),
//		IID_PPV_ARGS(&m_graphicsRootSignatures[(int)GraphicsRootSignatureType::General])
//	);
//
//	if (FAILED(hr))
//	{
//		ERROR_AND_DIE("Could not create graphics root signature");
//	}
//
//	DX_SAFE_RELEASE(serializedRootSig);
//	DX_SAFE_RELEASE(errorBlob);
//
//	//-----------------------------------------------------------------------------------------------
//	// Initialize desired Graphics PSO
//	//-----------------------------------------------------------------------------------------------
//	m_desiredGraphicsPSO = new GraphicsPSO();
//}

void DX12Renderer::CreateTextureHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = DX12Graphics::MAX_TEXTURE_NUM;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_textureHeap));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create texture heap");
	}
#ifdef _DEBUG
	m_textureHeap->SetName(L"texture heap");
#endif // _DEBUG
}

void DX12Renderer::InitializeDefaultShaderAndTexture()
{
	m_defaultTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8::OPAQUE_WHITE, "DefaultDiffuse"));
	m_defaultNormalTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 255, 255), "DefaultNormal"));
	m_defaultSpecGlossEmitTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 0, 255), "DefaultSpecGlossEmit"));
	
	m_defaultShader = CreateShader("Default", g_defaultShaderSource);
}

void DX12Renderer::InitializeFrameResources()
{
	m_mainLinearAllocator = new DX12LinearAllocator(m_device, MAX_MAIN_LINEAR_ALLOCATOR_SIZE);
	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		m_frameResources[i] = new FrameResource();
		m_frameResources[i]->m_linearAllocator = new DX12LinearAllocator(m_device, MAX_LINEAR_ALLOCATOR_SIZE);
		
		HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(m_frameResources[i]->m_commandAllocator)));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create command allocator.");
	}
}

void DX12Renderer::DestroyFrameResources()
{
	// Remember to Flush the command queue before destroy
	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		DX_SAFE_RELEASE(m_frameResources[i]->m_commandAllocator);
		delete m_frameResources[i]->m_linearAllocator;
		m_frameResources[i]->m_linearAllocator = nullptr;
		delete m_frameResources[i];
		m_frameResources[i] = nullptr;
	}
	delete m_mainLinearAllocator;
	m_mainLinearAllocator = nullptr;
}

void DX12Renderer::SetDebugNameForBasicDXObjects()
{
	m_device->SetName(L"Device");

}

ID3D12Resource* DX12Renderer::GetCurrentBackBuffer() const
{
	return m_swapChainBuffer[m_currBackBuffer];
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_currBackBuffer,
		m_rtvDescriptorSize
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetDepthStencilView() const
{
	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

DX12LinearAllocator* DX12Renderer::GetCurrentLinearAllocator() const
{
	if (IsBeforeTheFirstFrame())
	{
		return m_mainLinearAllocator;
	}
	return m_currFrameResource->m_linearAllocator;
}

uint64_t DX12Renderer::Signal()
{
	// Is different to other's first signal then increment 1
	uint64_t fenceValueForSignal = m_nextFenceValueForSignal;
	m_nextFenceValueForSignal++;

	HRESULT hr = m_commandQueue->Signal(m_fence, fenceValueForSignal);
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to signal.");
	}

	return fenceValueForSignal;
}

void DX12Renderer::WaitForFenceValue(uint64_t fenceValue)
{
	if (m_fence->GetCompletedValue() < fenceValue)
	{
		HRESULT	hr = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Failed to set the fence event");
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}


void DX12Renderer::FlushCommandQueue()
{
	WaitForFenceValue(Signal());
}

uint64_t DX12Renderer::ExecuteCommandList()
{
	GUARANTEE_OR_DIE(SUCCEEDED(m_commandList->Close()), "Could not close the command list");
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	uint64_t fenceValue = Signal();
	return fenceValue;
}

void DX12Renderer::ExecuteCommandListAndWait()
{
	WaitForFenceValue(ExecuteCommandList());
}

void DX12Renderer::EnqueueDeferredRelease(IUnknown* resource)
{
	m_deferredReleaseQueue->Enqueue(m_nextFenceValueForSignal, resource);
}

void DX12Renderer::OnResize()
{
	if (m_device == nullptr) return;
	FlushCommandQueue();
	
	// Release the rtvs and dsv
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_swapChainBuffer[i]);
	}
	DX_SAFE_RELEASE(m_depthStencilBuffer);

	// Resize the swap chain
	HRESULT hr;
	hr = m_swapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		m_config.m_window->GetClientDimensions().x, 
		m_config.m_window->GetClientDimensions().y,
		DX12Graphics::BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not resize swapchain buffer");

	m_currBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
	CreateDepthStencilBufferAndView();

	FireEvent(WINDOW_RESIZE_EVENT);

	DebuggerPrintf("OnResize\n");
}

void DX12Renderer::SetGraphicsRootSignature(ID3D12RootSignature* rootSig)
{
	if (rootSig == m_curGraphicsRootSignature)
		return;

	m_commandList->SetGraphicsRootSignature(rootSig);
	m_desiredGraphicsPSO->SetRootSignature(m_graphicsRootSignatures[(int)GraphicsRootSignatureType::General]);
	m_curGraphicsRootSignature = rootSig;
}

void DX12Renderer::SetComputeRootSignature(ID3D12RootSignature* rootSig)
{
	if (rootSig == m_curComputeRootSignature)
		return;

	m_commandList->SetComputeRootSignature(rootSig);
	m_curComputeRootSignature = rootSig;
}

void DX12Renderer::SetPipelineState(PSO const& pso)
{
	ID3D12PipelineState* pipelineState = pso.GetPipelineStateObject();
	if (pipelineState == m_currentPipelineState)
		return;

	m_commandList->SetPipelineState(pipelineState);
	m_currentPipelineState = pipelineState;
}

void DX12Renderer::SetViewport(D3D12_VIEWPORT const& viewport)
{
	m_commandList->RSSetViewports(1, &viewport);
}

void DX12Renderer::SetScissor(D3D12_RECT const& rect)
{
	m_commandList->RSSetScissorRects(1, &rect);
}

void DX12Renderer::SetConstantArray(unsigned int rootParameterIndex, unsigned int numConstants, void const* pConstants)
{
	m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, pConstants, 0);
}

void DX12Renderer::SetConstantBuffer(unsigned int rootParamterIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
{
	m_commandList->SetGraphicsRootConstantBufferView(rootParamterIndex, cbv);
}

void DX12Renderer::SetDynamicConstantBuffer(unsigned int rootParamterIndex, size_t bufferSize, const void* bufferData)
{
	DynAlloc cb = GetCurrentLinearAllocator()->Allocate(bufferSize);
	memcpy(cb.m_dataPtr, bufferData, bufferSize);
	m_commandList->SetGraphicsRootConstantBufferView(rootParamterIndex, cb.m_gpuAddress);
}

void DX12Renderer::SetDynamicVertexBuffer(unsigned int slot, size_t numVertices, size_t vertexStride, void const* vertexData)
{
	size_t bufferSize = numVertices * vertexStride;
	DynAlloc vb = GetCurrentLinearAllocator()->Allocate(bufferSize, 16);
	memcpy(vb.m_dataPtr, vertexData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = vb.m_gpuAddress;
	view.SizeInBytes = (UINT)bufferSize; // or aligned Size (slightly larger)
	view.StrideInBytes = (UINT)vertexStride;

	m_commandList->IASetVertexBuffers(slot, 1, &view);
}

void DX12Renderer::SetDynamicIndexBuffer(size_t numIndexes, const void* indexData)
{
	size_t bufferSize = numIndexes * sizeof(unsigned int);
	DynAlloc ib = GetCurrentLinearAllocator()->Allocate(bufferSize, 16);
	memcpy(ib.m_dataPtr, indexData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = ib.m_gpuAddress;
	view.SizeInBytes = (UINT)bufferSize;
	view.Format = DXGI_FORMAT_R32_UINT;

	m_commandList->IASetIndexBuffer(&view);
}

void DX12Renderer::SetDynmaicSRV(unsigned int rootParameterIndex, size_t bufferSize, void const* bufferData)
{
	DynAlloc cb = GetCurrentLinearAllocator()->Allocate(bufferSize);
	memcpy(cb.m_dataPtr, bufferData, bufferSize);
	m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, cb.m_gpuAddress);
}

void DX12Renderer::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView)
{
	m_commandList->IASetIndexBuffer(&ibView);
}

void DX12Renderer::SetVertexBuffer(unsigned int slot, const D3D12_VERTEX_BUFFER_VIEW& vbView)
{
	SetVertexBuffers(slot, 1, &vbView);
}

void DX12Renderer::SetVertexBuffers(unsigned int startSlot, unsigned int count, const D3D12_VERTEX_BUFFER_VIEW vbViews[])
{
	m_commandList->IASetVertexBuffers(startSlot, count, vbViews);
}

void DX12Renderer::Draw(unsigned int vertexCount, unsigned int startVertexLocation /*= 0*/)
{
	DrawInstanced(vertexCount, 1, startVertexLocation, 0);
}

void DX12Renderer::DrawIndexed(unsigned int indexCount, unsigned int startIndexLocation /*= 0*/, unsigned int baseVertexLocation /*= 0*/)
{
	DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void DX12Renderer::DrawInstanced(unsigned int vertexCountPerInstance, unsigned int instanceCount, unsigned int startVertexLocation /*= 0*/, unsigned int startInstanceLocation /*= 0*/)
{
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void DX12Renderer::DrawIndexedInstanced(unsigned int indexCountPerInstance, unsigned int instanceCount, unsigned int startIndexLocation, unsigned int baseVertexLocation, unsigned int startInstanceLocation)
{
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void DX12Renderer::BeginRenderEvent(char const* eventName)
{
	UNUSED(eventName);
	//ERROR_AND_DIE("Not implemented");
}

void DX12Renderer::EndRenderEvent(char const* optional_eventName)
{
	UNUSED(optional_eventName);
	//ERROR_AND_DIE("Not implemented");
}


void DX12Renderer::ImGuiStartup()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = IMGUI_SRV_HEAP_SIZE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ImGuiSrvDescHeap));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create imgui srv heap");
	}
#ifdef _DEBUG
	m_ImGuiSrvDescHeap->SetName(L"ImGui SRV heap");
#endif // _DEBUG
	g_ImGuiSrvDescHeapAlloc.Create(m_device, m_ImGuiSrvDescHeap);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();


	ImGui_ImplWin32_Init((HWND)m_config.m_window->GetHwnd());

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = m_device;
	init_info.CommandQueue = m_commandQueue;
	init_info.NumFramesInFlight = FRAMES_IN_FLIGHT;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	init_info.SrvDescriptorHeap = m_ImGuiSrvDescHeap;
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_ImGuiSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_ImGuiSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&init_info);

	// #ToDo: Load Fonts
}

void DX12Renderer::ImGuiBeginFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	//bool show_demo_window = true;
	//if (show_demo_window)
	//	ImGui::ShowDemoWindow(&show_demo_window);
}

void DX12Renderer::ImGuiEndFrame()
{
	ImGui::Render();
	m_commandList->SetDescriptorHeaps(1, &m_ImGuiSrvDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);
}

void DX12Renderer::ImGuiShutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


#endif // ENGINE_RENDER_D3D12
