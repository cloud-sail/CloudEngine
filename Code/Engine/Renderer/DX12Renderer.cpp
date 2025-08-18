#include "Engine/Renderer/DX12Renderer.hpp"

#ifdef ENGINE_RENDER_D3D12

#include "Engine/Core/Image.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/Buffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/DX12DescriptorHeap.hpp"
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
	//for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) 
	//{
	//	m_swapChainBufferRtvIndex[i] = INVALID_INDEX_U32;
	//}
}

void DX12Renderer::Startup()
{
	DX12Graphics::InitializeCommonState();
	m_deferredReleaseQueue = new DX12DeferredReleaseQueue();

	CreateDevice();
	CreateFenceAndHandle();
	// Check 4x MSAA Quality Support
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateRenderTargetViews();
	CreateDepthStencilBufferAndView();
	//CreateGraphicsRootSignatures();
	CreateBindlessRootSignature();
	CreateTextureHeap();

	InitializeDefaultShaderAndTexture();
	InitializeDefaultSamplers();
	InitializeFrameResources();

	ImGuiStartup();
}

void DX12Renderer::BeginFrame()
{
	// Execute the commands before the first frame begin 
	// or commandList is opened when not between Renderer::BeginFrame and Renderer::EndFrame
	if (/*m_currFrameResource == nullptr ||*/ m_isCommandListOpened)
	{
		ExecuteCommandListAndWait();
		m_isCommandListOpened = false;
	}
	// Cycle through the circular frame resource array.
	m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % FRAMES_IN_FLIGHT;
	m_currFrameResource = m_frameResources[m_currFrameResourceIndex];

	m_rtvDescriptorHeap->BeginFrame();
	m_dsvDescriptorHeap->BeginFrame();
	m_cbvSrvUavDescriptorHeap->BeginFrame();
	m_samplerDescriptorHeap->BeginFrame();

	// wait until the GPU has completed commands up to this fence point
	if (m_currFrameResource->m_fenceValue != 0)
	{
		WaitForFenceValue(m_currFrameResource->m_fenceValue);
	}

	m_deferredReleaseQueue->Process(m_currFrameResource->m_fenceValue);






	// Reset command list, current frame command allocator, current frame linear allocator
	GUARANTEE_OR_DIE(SUCCEEDED(GetCurrentCommandAllocator()->Reset()), "Cannot reset current frame command allocator");
	GUARANTEE_OR_DIE(SUCCEEDED(m_commandList->Reset(GetCurrentCommandAllocator(), nullptr)), "Could not reset the command list");
	GetCurrentLinearAllocator()->Reset();

	//-----------------------------------------------------------------------------------------------
	m_curGraphicsRootSignature = nullptr;
	m_curComputeRootSignature = nullptr;
	m_currentPipelineState = nullptr;

	//ID3D12DescriptorHeap* descriptorHeaps[] = { m_textureHeap };
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
	// SetDescriptorHeaps must be called, passing the corresponding heaps, before a call to SetGraphicsRootSignature or SetComputeRootSignature 
	// that uses either CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED or SAMPLER_HEAP_DIRECTLY_INDEXED flags. 
	// This is in order to make sure the correct heap pointers are available when the root signature is set. 
	// Additionally, SetDescriptorHeaps may not be called after SetGraphicsRootSignature or SetComputeRootSignature with different heap pointers 
	// before a Draw or Dispatch.
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvUavDescriptorHeap->GetCurrentHeap(), m_samplerDescriptorHeap->GetCurrentHeap() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Fixed Primitive Topology

	// Indicate that the back buffer will now be used to render.
	CD3DX12_RESOURCE_BARRIER backBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &backBufferBarrier);

	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
	SetDefaultRenderTargets();

	//SetGraphicsRootSignature(m_graphicsRootSignatures[(int)GraphicsRootSignatureType::General]);
	SetGraphicsRootSignature(m_bindlessRootSignature);
	SetComputeRootSignature(m_bindlessRootSignature);

	// Set Constants
	m_tempEngineConstantsIndex = INVALID_INDEX_U32;
	m_tempPerFrameConstantsIndex = INVALID_INDEX_U32;
	m_tempLightConstantsIndex = INVALID_INDEX_U32;
	m_tempCameraConstantsIndex = INVALID_INDEX_U32;
	m_tempModelConstantsIndex = INVALID_INDEX_U32;

	m_tempEngineConstantsIndex = AllocateTempConstantBuffer(sizeof(EngineConstants), &m_engineConstants);
	m_tempLightConstantsIndex = AllocateTempConstantBuffer(sizeof(LightConstants), &m_lightConstants);
	m_tempPerFrameConstantsIndex = AllocateTempConstantBuffer(sizeof(PerFrameConstants), &m_perFrameConstants);

	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::PERFRAME_CONSTANTS, sizeof(PerFrameConstants), &m_perFrameConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::ENGINE_CONSTANTS, sizeof(EngineConstants), &m_engineConstants);


	// Some things need to be done, after the command list is reset (start to record new commands)
	// Begin a new frame or ExecuteAndWait (read back from GPU and need to get the data in the same frame)


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
	// Release Loaded Texture
	for (int textureIndex = 0; textureIndex < (int)m_loadedTextures.size(); ++textureIndex)
	{
		delete m_loadedTextures[textureIndex];
		m_loadedTextures[textureIndex] = nullptr;
	}
	//m_defaultTexture = nullptr;
	//m_defaultNormalTexture = nullptr;
	//m_defaultSpecGlossEmitTexture = nullptr;

	delete m_defaultDepthBuffer;
	m_defaultDepthBuffer = nullptr;

	for (unsigned int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		EnqueueDeferredRelease(m_swapChainBufferHandles[i]);
		//EnqueueDeferredRelease(DescriptorHeapType::RTV, m_swapChainBufferRtvIndex[i]);
	}
	EnqueueDeferredRelease(m_defaultDepthBufferHandle);
	//EnqueueDeferredRelease(DescriptorHeapType::DSV, m_defaultDsvIndex);

	// Release all deferred released resource and descriptor
	delete m_deferredReleaseQueue;
	m_deferredReleaseQueue = nullptr;


	//-----------------------------------------------------------------------------------------------
	//for (int i = 0; i < (int)GraphicsRootSignatureType::Count; ++i)
	//{
	//	DX_SAFE_RELEASE(m_graphicsRootSignatures[i]);
	//}
	DX_SAFE_RELEASE(m_bindlessRootSignature);

	delete m_desiredGraphicsPSO;
	m_desiredGraphicsPSO = nullptr;

	delete m_desiredComputePSO;
	m_desiredComputePSO = nullptr;

	PSO::DestroyAll();
	//-----------------------------------------------------------------------------------------------
	// Release fundamental members
	s_device = nullptr;
	DestroyFrameResources();
	DX_SAFE_RELEASE(m_commandQueue);
	DX_SAFE_RELEASE(m_mainCommandAllocator);
	DX_SAFE_RELEASE(m_commandList);

	delete m_rtvDescriptorHeap;
	delete m_dsvDescriptorHeap;
	delete m_cbvSrvUavDescriptorHeap;
	delete m_samplerDescriptorHeap;
	m_rtvDescriptorHeap = nullptr;
	m_dsvDescriptorHeap = nullptr;
	m_cbvSrvUavDescriptorHeap = nullptr;
	m_samplerDescriptorHeap = nullptr;


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

	//DX_SAFE_RELEASE(m_depthStencilBuffer);

	DX_SAFE_RELEASE(m_swapChain);

	DX_SAFE_RELEASE(m_dxgiFactory);

	DX_SAFE_RELEASE(m_fence);

	DX12Graphics::DestroyCommonState();


	//-----------------------------------------------------------------------------------------------
	// Debug Layer, Memory Leak Checks

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

	TransitionToDepthWrite(*m_defaultDepthBuffer);
	// m_swapChainBuffer resource state cannot be changed, only changed in begin frame and end frame

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
	cameraConstants.ClipToWorldTransform = camera.GetClipToWorldTransform();

	m_tempCameraConstantsIndex = AllocateTempConstantBuffer(sizeof(CameraConstants), &cameraConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::CAMERA_CONSTANTS, sizeof(CameraConstants), &cameraConstants);

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

void DX12Renderer::DrawProcedural(unsigned int vertexCount)
{
	// Not use vertex buffer, only use uint vertexID : SV_VertexID
	m_commandList->IASetVertexBuffers(0, 0, nullptr); // for safety
	m_desiredGraphicsPSO->Finalize();
	SetPipelineState(*m_desiredGraphicsPSO);
	Draw(vertexCount);
}

void DX12Renderer::Dispatch1D(unsigned int threadCountX, unsigned int groupSizeX /*= 64*/)
{
	m_desiredComputePSO->Finalize();
	SetPipelineState(*m_desiredComputePSO);
	Dispatch(RendererMath::CeilDivide(threadCountX, groupSizeX),
		1,
		1);
}

void DX12Renderer::Dispatch2D(unsigned int threadCountX, unsigned int threadCountY, unsigned int groupSizeX /*= 8*/, unsigned int groupSizeY /*= 8*/)
{
	m_desiredComputePSO->Finalize();
	SetPipelineState(*m_desiredComputePSO);
	Dispatch(RendererMath::CeilDivide(threadCountX, groupSizeX),
		RendererMath::CeilDivide(threadCountY, groupSizeY),
		1);
}

void DX12Renderer::Dispatch3D(unsigned int threadCountX, unsigned int threadCountY, unsigned int threadCountZ, unsigned int groupSizeX, unsigned int groupSizeY, unsigned int groupSizeZ)
{
	m_desiredComputePSO->Finalize();
	SetPipelineState(*m_desiredComputePSO);
	Dispatch(RendererMath::CeilDivide(threadCountX, groupSizeX),
		RendererMath::CeilDivide(threadCountY, groupSizeY),
		RendererMath::CeilDivide(threadCountZ, groupSizeZ));
}

void DX12Renderer::DispatchRaw(unsigned int groupCountX, unsigned int groupCountY, unsigned int groupCountZ)
{
	m_desiredComputePSO->Finalize();
	SetPipelineState(*m_desiredComputePSO);
	Dispatch(groupCountX,
		groupCountY,
		groupCountZ);
}

void DX12Renderer::BindTexture(Texture const* texture, int slot /*= 0*/)
{
	UNUSED(texture);
	UNUSED(slot);
	ERROR_AND_DIE("Bindless Rendering, cannot use");
	

	// TODO bindless texture
	//if (slot == 0)
	//{
	//	if (texture == nullptr) texture = m_defaultTexture;
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	//	gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
	//	m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_0, gpuHandle);
	//}
	//else if (slot == 1)
	//{
	//	if (texture == nullptr) texture = m_defaultNormalTexture;
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	//	gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
	//	m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_1, gpuHandle);
	//}
	//else if (slot == 2)
	//{
	//	if (texture == nullptr) texture = m_defaultSpecGlossEmitTexture;
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	//	gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
	//	m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_SLOT_2, gpuHandle);
	//}
	//else if (slot == 9)
	//{
	//	if (texture == nullptr)
	//	{
	//		ERROR_AND_DIE("Texture Cube cannot be set to default now");
	//	};
	//	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_textureHeap->GetGPUDescriptorHandleForHeapStart());
	//	gpuHandle.Offset(texture->m_srvHeapIndex, m_cbvSrvUavDescriptorSize);
	//	m_commandList->SetGraphicsRootDescriptorTable((UINT)GeneralRootBinding::TEXTURE_CUBE, gpuHandle);
	//}
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
	ERROR_AND_DIE("Bindless Rendering, cannot use");
}

void DX12Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_desiredGraphicsPSO->SetRasterizerMode(rasterizerMode);
}

void DX12Renderer::SetDepthMode(DepthMode depthMode)
{
	m_desiredGraphicsPSO->SetDepthMode(depthMode);
}

void DX12Renderer::SetRenderTargetFormats(const std::vector<DXGI_FORMAT>& rtvFormats /*= { DXGI_FORMAT_R8G8B8A8_UNORM }*/, DXGI_FORMAT dsvFormat /*= DXGI_FORMAT_D24_UNORM_S8_UINT*/, unsigned int msaaCount /*= 1*/, unsigned int msaaQuality /*= 0*/)
{
	m_desiredGraphicsPSO->SetRenderTargetFormats(
		static_cast<UINT>(rtvFormats.size()),
		rtvFormats.empty() ? nullptr : rtvFormats.data(),
		dsvFormat,
		msaaCount,
		msaaQuality);
}

void DX12Renderer::BindComputeShader(Shader* csShader)
{
	m_desiredComputePSO->SetShader(csShader);
}

void DX12Renderer::SetEngineConstants(int debugInt /*= 0*/, float debugFloat /*= 0.f*/)
{
	m_engineConstants.m_debugInt = debugInt;
	m_engineConstants.m_debugFloat = debugFloat;
	if (IsBeforeTheFirstFrame()) return;

	m_tempEngineConstantsIndex = AllocateTempConstantBuffer(sizeof(EngineConstants), &m_engineConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::ENGINE_CONSTANTS, sizeof(EngineConstants), &m_engineConstants);
}

void DX12Renderer::SetModelConstants(Mat44 const& modelToWorldTransform /*= Mat44()*/, Rgba8 const& modelColor /*= Rgba8::OPAQUE_WHITE*/)
{
	ModelConstants modelConstants;

	modelConstants.ModelToWorldTransform = modelToWorldTransform;
	modelColor.GetAsFloats(modelConstants.ModelColor);

	//if (IsBeforeTheFirstFrame()) return; // no need to check
	m_tempModelConstantsIndex = AllocateTempConstantBuffer(sizeof(ModelConstants), &modelConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::MODEL_CONSTANTS, sizeof(ModelConstants), &modelConstants);
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

	m_tempLightConstantsIndex = AllocateTempConstantBuffer(sizeof(LightConstants), &m_lightConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
}

void DX12Renderer::SetLightConstants(LightConstants const& lightConstants)
{
	m_lightConstants = lightConstants;
	if (IsBeforeTheFirstFrame()) return;
	m_tempLightConstantsIndex = AllocateTempConstantBuffer(sizeof(LightConstants), &m_lightConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::LIGHT_CONSTANTS, sizeof(LightConstants), &m_lightConstants);
}

void DX12Renderer::SetPerFrameConstants(PerFrameConstants const& perframeConstants)
{
	m_perFrameConstants = perframeConstants;
	if (IsBeforeTheFirstFrame()) return;
	m_tempPerFrameConstantsIndex = AllocateTempConstantBuffer(sizeof(PerFrameConstants), &m_perFrameConstants);
	//SetDynamicConstantBuffer((unsigned int)GeneralRootBinding::PERFRAME_CONSTANTS, sizeof(PerFrameConstants), &m_perFrameConstants);
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

uint32_t DX12Renderer::GetSrvIndexFromLoadedTexture(Texture const* texture, DefaultTexture type /*= DefaultTexture::WhiteOpaque2D*/)
{
	if (texture == nullptr || texture->m_srvHeapIndex == INVALID_INDEX_U32)
	{
		return GetDefaultTextureSrvIndex(type);
	}
	return texture->m_srvHeapIndex;
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
	//ERROR_AND_DIE("Please use ShaderConfig to create shader, DXC and SM6.6 is required");

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
	// Notes: LoadedTexture is readOnly. we could reuse the code but we still write it completely.
	// It will not be destroyed until shutdown
	// May Copy From one texture to another (not well designed)

	// 1. create default heap
	D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		image.GetDimensions().x,
		image.GetDimensions().y,
		1,
		1	// mipLevels
	);

	ID3D12Resource* resource = nullptr;
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture default heap");
#ifdef _DEBUG
	resource->SetName(ToWString(image.GetImageFilePath()).c_str());
#endif // _DEBUG

	// 2. create temp upload heap
	ID3D12Resource* textureUploadHeap = nullptr;
	UINT64 uploadBufferSize = 0;
	m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);
	//UINT64 uploadBufferSize = GetRequiredIntermediateSize(newTexture->m_resource, 0, 1); // d3dx12.h


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

	UpdateSubresources(m_commandList, resource, textureUploadHeap, 0, 0, 1, &textureData);

	// 4. Transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// release temp upload heap
	EnqueueDeferredRelease(textureUploadHeap);

	// 5. Allocate a SRV Descriptor
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1; // same as desc.miplevels

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(resource, &srvDesc, alloc.m_handles[i]);
	}

	Texture* newTexture = new Texture(this);
	newTexture->m_name = image.GetImageFilePath();
	newTexture->m_dimensions = image.GetDimensions();
	newTexture->m_depth = 1;
	newTexture->m_resource = resource;
	newTexture->m_srvHeapIndex = alloc.m_index; // important

	newTexture->m_mipLevels = 1;
	newTexture->m_sampleCount = 1;
	newTexture->m_sampleQuality = 0;
	newTexture->m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	newTexture->m_supportSRV = true;
	newTexture->m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	newTexture->m_resourceDimension = DXResourceDimension::TEXTURE2D;

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

	ID3D12Resource* resource = nullptr;
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture cube default heap");
#ifdef _DEBUG
	resource->SetName(ToWString(config.m_name).c_str());
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
	UpdateSubresources(m_commandList, resource, textureUploadHeap, 0, 0, 6, subresources);

	// 5. Transition state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);

	// release temp upload heap
	EnqueueDeferredRelease(textureUploadHeap);

	// 6. Create SRV and put it into the heap
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(resource, &srvDesc, alloc.m_handles[i]);
	}

	Texture* newTexture = new Texture(this);
	newTexture->m_name = config.m_name;
	newTexture->m_dimensions = IntVec2(width, height);
	newTexture->m_depth = 6;
	newTexture->m_resource = resource;
	newTexture->m_srvHeapIndex = alloc.m_index; // important

	newTexture->m_mipLevels = 1;
	newTexture->m_sampleCount = 1;
	newTexture->m_sampleQuality = 0;
	newTexture->m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	newTexture->m_supportSRV = true;
	newTexture->m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	newTexture->m_resourceDimension = DXResourceDimension::TEXTURE2D;

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

	if (config.m_stages & SHADER_STAGE_CS) {
		bool ok = CompileShaderToByteCode(shader->m_computeShaderByteCode, config.m_name.c_str(), shaderSource, config.m_computeEntryPoint.c_str(), config.m_shaderModelCS.c_str(), config.m_compilerType);
		GUARANTEE_RECOVERABLE(ok, "Could not compile compute shader.");
	}
	// MS AS

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
	const wchar_t* saveFolder = L".\\pdbs\\";
	CreateDirectoryW(saveFolder, nullptr);

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
		//L"-Fd", L".\\pdbs\\",
		//DXC_ARG_ALL_RESOURCES_BOUND,  // Allow more aggressive optimization, no safe values for reading unbounded resource
#if defined(ENGINE_DEBUG_RENDER)
		DXC_ARG_DEBUG,
		DXC_ARG_SKIP_OPTIMIZATIONS,
#else
		DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
		DXC_ARG_WARNINGS_ARE_ERRORS,

		//L"-Qstrip_reflect",				// Strip reflection into a separate blob
		//L"-Qstrip_debug",				// Strip debug information into a separate blob
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


	//
	// Output PDB file
	//
	IDxcBlob* pPDB = nullptr;
	IDxcBlobUtf16* pPDBName = nullptr;
	hr = pResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), &pPDBName);
	if (SUCCEEDED(hr) && pPDB && pPDBName)
	{
		FILE* fPDB = nullptr;

		const wchar_t* originalName = pPDBName->GetStringPointer();

		std::wstring fullPath = std::wstring(saveFolder) + originalName;

		_wfopen_s(&fPDB, fullPath.c_str(), L"wb");
		if (fPDB)
		{
			fwrite(pPDB->GetBufferPointer(), pPDB->GetBufferSize(), 1, fPDB);
			fclose(fPDB);
			DebuggerPrintf("PDB saved as: ");
			DebuggerWPrintf(fullPath.c_str());
			DebuggerPrintf("\n");
		}
		else
		{
			DebuggerPrintf("Failed to save PDB file!\n");
		}
	}

	if (pPDB) pPDB->Release();
	if (pPDBName) pPDBName->Release();

	if (pShader) pShader->Release();
	if (pErrors) pErrors->Release();
	if (pResults) pResults->Release();
	if (pIncludeHandler) pIncludeHandler->Release();
	if (pSourceBlob) pSourceBlob->Release();
	if (pCompiler) pCompiler->Release();
	if (pUtils) pUtils->Release();

	return true;
}

uint32_t DX12Renderer::GetDefaultTextureSrvIndex(DefaultTexture type)
{
	return m_defaultTextures[(int)type]->m_srvHeapIndex;
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

void DX12Renderer::CreateFenceAndHandle()
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
	m_isCommandListOpened = true;
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
	//HRESULT hr;

	//D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	//rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	//rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	//rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//rtvHeapDesc.NodeMask = 0;
	//hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	//if (FAILED(hr))
	//{
	//	ERROR_AND_DIE("Could not create RTV Discriptor Heap.");
	//}

	//D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	//dsvHeapDesc.NumDescriptors = 1;
	//dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//dsvHeapDesc.NodeMask = 0;
	//hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
	//if (FAILED(hr))
	//{
	//	ERROR_AND_DIE("Could not create DSV Discriptor Heap.");
	//}

	m_rtvDescriptorHeap = new DX12DescriptorHeap(256, 0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
	m_dsvDescriptorHeap = new DX12DescriptorHeap(256, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
	m_cbvSrvUavDescriptorHeap = new DX12DescriptorHeap(2048, 2048, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_samplerDescriptorHeap = new DX12DescriptorHeap(256, 0, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void DX12Renderer::CreateRenderTargetViews()
{
	HRESULT hr;

	for (unsigned int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		DX_SAFE_RELEASE(m_swapChainBuffer[i]); // redundancy release
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i]));
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Could not get render target from swap chain.");
		}
		//EnqueueDeferredRelease(DescriptorHeapType::RTV, m_swapChainBufferRtvIndex[i]);
		EnqueueDeferredRelease(m_swapChainBufferHandles[i]);

		PersistentDescriptorAlloc alloc = m_rtvDescriptorHeap->AllocatePersistent();

		m_swapChainBufferHandles[i].m_type = DescriptorHeapType::RTV;
		m_swapChainBufferHandles[i].m_index = alloc.m_index;

		//m_swapChainBufferRtvIndex[i] = alloc.m_index;

		for (uint32_t j = 0; j < m_rtvDescriptorHeap->GetNumHeaps(); ++j)
		{ 
			m_device->CreateRenderTargetView(m_swapChainBuffer[i], nullptr, alloc.m_handles[j]);
		}
	}
}

void DX12Renderer::CreateDepthStencilBufferAndView()
{
	if (m_defaultDepthBuffer != nullptr)
	{
		delete m_defaultDepthBuffer;
		m_defaultDepthBuffer = nullptr;
	}


	TextureInit dbInit;
	dbInit.m_width = m_config.m_window->GetClientDimensions().x;
	dbInit.m_height = m_config.m_window->GetClientDimensions().y;
	dbInit.m_format = DXGI_FORMAT_R24G8_TYPELESS;
	dbInit.m_allowDSV = true;
	dbInit.m_dsvClearFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dbInit.m_name = "Default Depth Buffer";
	dbInit.m_debugName = L"Default Depth Buffer";

	m_defaultDepthBuffer = CreateTexture(dbInit);
	TransitionToDepthWrite(*m_defaultDepthBuffer);

	EnqueueDeferredRelease(m_defaultDepthBufferHandle);
	//EnqueueDeferredRelease(DescriptorHeapType::DSV, m_defaultDsvIndex);
	m_defaultDepthBufferHandle = AllocateDSV(*m_defaultDepthBuffer, DXGI_FORMAT_D24_UNORM_S8_UINT);
	//m_defaultDsvIndex = AllocateDSV(*m_defaultDepthBuffer, DXGI_FORMAT_D24_UNORM_S8_UINT);


}

//void DX12Renderer::CreateGraphicsRootSignatures()
//{
//	// Root Signature 1.1     "RootConstants(b0, num32BitConstants=64, visibility = SHADER_VISIBILITY_ALL),"    
//
//	CD3DX12_DESCRIPTOR_RANGE1 textureTable0;
//	textureTable0.Init(
//		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
//		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE1 textureTable1;
//	textureTable1.Init(
//		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0,
//		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE1 textureTable2;
//	textureTable2.Init(
//		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0,
//		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE1 textureTable9;
//	textureTable9.Init(
//		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0,
//		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
//
//	CD3DX12_ROOT_PARAMETER1 slotRootParameters[(int)GeneralRootBinding::NUM];
//
//	slotRootParameters[(int)GeneralRootBinding::MODEL_CONSTANTS]
//		.InitAsConstantBufferView(k_modelConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_0]
//		.InitAsDescriptorTable(1, &textureTable0, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_1]
//		.InitAsDescriptorTable(1, &textureTable1, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_SLOT_2]
//		.InitAsDescriptorTable(1, &textureTable2, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::CAMERA_CONSTANTS]
//		.InitAsConstantBufferView(k_cameraConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::PERFRAME_CONSTANTS]
//		.InitAsConstantBufferView(k_perFrameConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::LIGHT_CONSTANTS]
//		.InitAsConstantBufferView(k_lightConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::ENGINE_CONSTANTS]
//		.InitAsConstantBufferView(k_engineConstantsSlot, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_ALL);
//
//	slotRootParameters[(int)GeneralRootBinding::TEXTURE_CUBE]
//		.InitAsDescriptorTable(1, &textureTable9, D3D12_SHADER_VISIBILITY_ALL);
//
//	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
//	rootSigDesc.Init_1_1(
//		(UINT)GeneralRootBinding::NUM,
//		slotRootParameters,
//		(UINT)g_numStaticSamplers,
//		g_staticSamplers,
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
//		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
//		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
//	);
//
//	ID3DBlob* serializedRootSig = nullptr;
//	ID3DBlob* errorBlob = nullptr;
//
//	//D3DX12SerializeVersionedRootSignature
//	HRESULT hr = D3D12SerializeVersionedRootSignature(
//		&rootSigDesc,
//		&serializedRootSig,
//		&errorBlob
//	);
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
//	m_desiredGraphicsPSO = new GraphicsPSO();
//}



void DX12Renderer::CreateBindlessRootSignature()
{
	//-----------------------------------------------------------------------------------------------
	// Root Signature 1.0
	//CD3DX12_ROOT_PARAMETER slotRootParameters[1];
	//slotRootParameters[0].InitAsConstants(64, 0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0

	//CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	//rootSigDesc.Init(
	//	1,
	//	slotRootParameters,
	//	(UINT)g_numStaticSamplers,
	//	g_staticSamplers,
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	//	D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
	//	D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);

	//ID3DBlob* serializedRootSig = nullptr;
	//ID3DBlob* errorBlob = nullptr;

	//HRESULT hr = D3D12SerializeRootSignature(
	//	&rootSigDesc,
	//	D3D_ROOT_SIGNATURE_VERSION_1,
	//	&serializedRootSig,
	//	&errorBlob);

	//if (errorBlob != nullptr)
	//{
	//	DebuggerPrintf((char*)errorBlob->GetBufferPointer());
	//}
	//GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not serialize bindless root signature");

	//hr = m_device->CreateRootSignature(
	//	0,
	//	serializedRootSig->GetBufferPointer(),
	//	serializedRootSig->GetBufferSize(),
	//	IID_PPV_ARGS(&m_bindlessRootSignature)
	//);

	//DX_SAFE_RELEASE(serializedRootSig);
	//DX_SAFE_RELEASE(errorBlob);

	//-----------------------------------------------------------------------------------------------
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_ROOT_PARAMETER1 slotRootParameters[1];
	slotRootParameters[0].InitAsConstants(
		64, // 64 * 32bit
		0,  // register b0
		0,  // register space 0
		D3D12_SHADER_VISIBILITY_ALL
	);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init_1_1(
		1,                    
		slotRootParameters,
		(UINT)g_numStaticSamplers,
		g_staticSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
		D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
	);

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DX12SerializeVersionedRootSignature(
		&rootSigDesc,
		featureData.HighestVersion, // 1.1 or fallback 1.0
		&serializedRootSig,
		&errorBlob
	);

	if (errorBlob != nullptr)
	{
		DebuggerPrintf((char*)errorBlob->GetBufferPointer());
	}

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not serialize bindless root signature");

	hr = m_device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_bindlessRootSignature)
	);

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create root signature");
	}

	DX_SAFE_RELEASE(serializedRootSig);
	DX_SAFE_RELEASE(errorBlob);

	m_desiredGraphicsPSO = new GraphicsPSO();
	m_desiredComputePSO = new ComputePSO();
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
	//m_defaultTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8::OPAQUE_WHITE, "DefaultDiffuse"));
	//m_defaultNormalTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 255, 255), "DefaultNormal"));
	//m_defaultSpecGlossEmitTexture = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 0, 255), "DefaultSpecGlossEmit"));
	
	m_defaultTextures[(int)DefaultTexture::Magenta2D] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8::MAGENTA, "Magenta2D"));
	m_defaultTextures[(int)DefaultTexture::BlackOpaque2D] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(0, 0, 0, 255), "BlackOpaque2D"));
	m_defaultTextures[(int)DefaultTexture::BlackTransparent2D] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(0, 0, 0, 0), "BlackTransparent2D"));
	m_defaultTextures[(int)DefaultTexture::WhiteOpaque2D] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8::OPAQUE_WHITE, "WhiteOpaque2D"));
	m_defaultTextures[(int)DefaultTexture::DefaultNormalMap] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 255, 255), "DefaultNormalMap"));
	m_defaultTextures[(int)DefaultTexture::DefaultSpecGlossEmitMap] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(127, 127, 0, 255), "DefaultSpecGlossEmitMap"));
	m_defaultTextures[(int)DefaultTexture::DefaultOcclusionRoughnessMetalnessMap] = CreateTextureFromImage(Image(IntVec2(2, 2), Rgba8(255, 127, 0, 255), "DefaultOcclusionRoughnessMetalnessMap"));

	{
		Image checkerBoard(IntVec2(2, 2), Rgba8(0, 0, 0, 255), "CheckerboardMagentaBlack2D");
		checkerBoard.SetTexelColor(IntVec2(0, 0), Rgba8::MAGENTA);
		checkerBoard.SetTexelColor(IntVec2(1, 1), Rgba8::MAGENTA);
		m_defaultTextures[(int)DefaultTexture::CheckerboardMagentaBlack2D] = CreateTextureFromImage(checkerBoard);
	}

	ShaderConfig defaultShaderConfig;
	defaultShaderConfig.m_name = "Data/Shaders/Unlit";
	m_defaultShader = CreateOrGetShader(defaultShaderConfig, VertexType::VERTEX_PCU);;
}

void DX12Renderer::InitializeDefaultSamplers()
{
	D3D12_SAMPLER_DESC samplerDesc_pointWrap = {};
	samplerDesc_pointWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc_pointWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc_pointWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc_pointWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc_pointWrap.MipLODBias = 0.0f;
	samplerDesc_pointWrap.MaxAnisotropy = 1;
	samplerDesc_pointWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc_pointWrap.BorderColor[0] = 0.0f;
	samplerDesc_pointWrap.BorderColor[1] = 0.0f;
	samplerDesc_pointWrap.BorderColor[2] = 0.0f;
	samplerDesc_pointWrap.BorderColor[3] = 0.0f;
	samplerDesc_pointWrap.MinLOD = 0.0f;
	samplerDesc_pointWrap.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_SAMPLER_DESC samplerDesc_pointClamp = samplerDesc_pointWrap;
	samplerDesc_pointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc_pointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc_pointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_SAMPLER_DESC samplerDesc_linearWrap = samplerDesc_pointWrap;
	samplerDesc_linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	D3D12_SAMPLER_DESC samplerDesc_linearClamp = samplerDesc_pointClamp;
	samplerDesc_linearClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	D3D12_SAMPLER_DESC samplerDesc_anisoWrap = samplerDesc_pointWrap;
	samplerDesc_anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc_anisoWrap.MaxAnisotropy = 8;

	D3D12_SAMPLER_DESC samplerDesc_anisoClamp = samplerDesc_pointClamp;
	samplerDesc_anisoClamp.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc_anisoWrap.MaxAnisotropy = 8;

	PersistentDescriptorAlloc alloc;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_pointWrap, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::POINT_WARP] = alloc.m_index;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_pointClamp, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::POINT_CLAMP] = alloc.m_index;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_linearWrap, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::BILINEAR_WRAP] = alloc.m_index;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_linearClamp, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::BILINEAR_CLAMP] = alloc.m_index;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_anisoWrap, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::ANISO_WARP] = alloc.m_index;

	alloc = m_samplerDescriptorHeap->AllocatePersistent();
	for (uint32_t i = 0; i < m_samplerDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateSampler(&samplerDesc_anisoClamp, alloc.m_handles[i]);
	}
	m_defaultSamplers[(int)SamplerMode::ANISO_CLAMP] = alloc.m_index;
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
	return GetCPUHandleFromDescriptorHandle(m_swapChainBufferHandles[m_currBackBuffer]);
	//return m_rtvDescriptorHeap->CPUHandleFromIndex(m_swapChainBufferRtvIndex[m_currBackBuffer]);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetDepthStencilView() const
{
	return GetCPUHandleFromDescriptorHandle(m_defaultDepthBufferHandle);
	//return m_dsvDescriptorHeap->CPUHandleFromIndex(m_defaultDsvIndex);
	//return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}


D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetCPUHandleFromDescriptorHandle(DescriptorHandle const& handle) const
{
	switch (handle.m_type)
	{
	case DescriptorHeapType::RTV:
		return m_rtvDescriptorHeap->CPUHandleFromIndex(handle.m_index);
		break;
	case DescriptorHeapType::DSV:
		return m_dsvDescriptorHeap->CPUHandleFromIndex(handle.m_index);
		break;
	case DescriptorHeapType::CBV_SRV_UAV:
		return m_cbvSrvUavDescriptorHeap->CPUHandleFromIndex(handle.m_index);
		break;
	case DescriptorHeapType::SAMPLER:
		return m_samplerDescriptorHeap->CPUHandleFromIndex(handle.m_index);
		break;
	default:
		ERROR_AND_DIE("Descriptor Handle does not have descriptor type.");
		break;
	}
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
	m_deferredReleaseQueue->EnqueueResource(m_nextFenceValueForSignal, resource);
}

void DX12Renderer::EnqueueDeferredRelease(DX12DescriptorHeap* heap, uint32_t index)
{
	m_deferredReleaseQueue->EnqueueDescriptor(m_nextFenceValueForSignal, heap, index);
}

void DX12Renderer::EnqueueDeferredRelease(DescriptorHeapType heapType, uint32_t index)
{
	switch (heapType)
	{
	case DescriptorHeapType::RTV:
		EnqueueDeferredRelease(m_rtvDescriptorHeap, index);
		break;
	case DescriptorHeapType::DSV:
		EnqueueDeferredRelease(m_dsvDescriptorHeap, index);
		break;
	case DescriptorHeapType::CBV_SRV_UAV:
		EnqueueDeferredRelease(m_cbvSrvUavDescriptorHeap, index);
		break;
	case DescriptorHeapType::SAMPLER:
		EnqueueDeferredRelease(m_samplerDescriptorHeap, index);
		break;
	default:
		break;
	}
}

void DX12Renderer::EnqueueDeferredRelease(DescriptorHandle& handle)
{
	EnqueueDeferredRelease(handle.m_type, handle.m_index);
	handle.Reset();
}

void DX12Renderer::OnResize()
{
	if (m_device == nullptr) return;
	FlushCommandQueue();
	
	// Must Release swap chain buffer before resize!
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_swapChainBuffer[i]);
	}
	//DX_SAFE_RELEASE(m_depthStencilBuffer);

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

	// ToDo: Extract as a function
	if (!m_isCommandListOpened)
	{
		GUARANTEE_OR_DIE(SUCCEEDED(m_mainCommandAllocator->Reset()), "Cannot reset main command allocator");
		GUARANTEE_OR_DIE(SUCCEEDED(m_commandList->Reset(m_mainCommandAllocator, nullptr)), "Could not reset the command list");
		m_isCommandListOpened = true;
	}

	CreateRenderTargetViews();
	CreateDepthStencilBufferAndView();

	FireEvent(WINDOW_RESIZE_EVENT);

	DebuggerPrintf("OnResize\n");
}




void DX12Renderer::SetDefaultRenderTargets()
{
	TransitionToDepthWrite(*m_defaultDepthBuffer);
	// m_swapChainBuffer resource state cannot be changed, only changed in begin frame and end frame

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}


void DX12Renderer::SetRenderTargetsByIndex(std::vector<uint32_t> rtvIndexs, uint32_t dsvIndex)
{
	constexpr uint32_t MAX_RTV = 8;
	GUARANTEE_OR_DIE(rtvIndexs.size() <= MAX_RTV, "Maximum number of RTVs is 8");

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[MAX_RTV];
	for (int i = 0; i < (int)rtvIndexs.size(); ++i)
	{
		GUARANTEE_OR_DIE(rtvIndexs[i] != INVALID_INDEX_U32, "Invalid RTV");
		rtvHandles[i] = m_rtvDescriptorHeap->CPUHandleFromIndex(rtvIndexs[i]);
	}

	if (dsvIndex != INVALID_INDEX_U32)
	{
		SetRenderTargets((UINT)rtvIndexs.size(), rtvHandles, m_dsvDescriptorHeap->CPUHandleFromIndex(dsvIndex));
	}
	else
	{
		SetRenderTargets((UINT)rtvIndexs.size(), rtvHandles);
	}
}


void DX12Renderer::ClearRenderTargetByIndex(uint32_t rtvIndex, Rgba8 const& clearColor)
{
	if (rtvIndex == INVALID_INDEX_U32) return;

	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);

	ClearRenderTarget(m_rtvDescriptorHeap->CPUHandleFromIndex(rtvIndex), colorAsFloats);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
	m_commandList->ClearRenderTargetView(rtvHandle, colorAsFloats, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DX12Renderer::ClearDepthAndStencilByIndex(uint32_t dsvIndex, float clearDepth /*= 1.f*/, uint8_t clearStencil /*= 0*/)
{
	if (dsvIndex == INVALID_INDEX_U32) return;

	ClearDepthAndStencil(m_dsvDescriptorHeap->CPUHandleFromIndex(dsvIndex), clearDepth, clearStencil);
}

void DX12Renderer::SetGraphicsBindlessResources(size_t resourceSizeInBytes, const void* pResource)
{
	GUARANTEE_OR_DIE(resourceSizeInBytes % 4 == 0, "Resource struct is wrong");

	unsigned int numConstants = static_cast<unsigned int>(resourceSizeInBytes / 4);
	SetConstantArray(0, numConstants, pResource);
}

void DX12Renderer::SetComputeBindlessResources(size_t resourceSizeInBytes, const void* pResource)
{
	GUARANTEE_OR_DIE(resourceSizeInBytes % 4 == 0, "Resource struct is wrong");
	unsigned int numConstants = static_cast<unsigned int>(resourceSizeInBytes / 4);
	SetComputeConstantArray(0, numConstants, pResource);
}

Texture* DX12Renderer::CreateTexture(TextureInit const& init)
{
	// Create Texture Resource
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if (init.m_allowRTV) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (init.m_allowDSV) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (init.m_allowUAV) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (!init.m_allowSRV) flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = ToDX12Dimension(init.m_resourceDimension);
	desc.Alignment = 0;
	desc.Width = init.m_width;
	desc.Height = init.m_height;
	desc.DepthOrArraySize = init.m_depthOrArraySize;
	desc.MipLevels = init.m_mipLevels;
	desc.Format = init.m_format;
	desc.SampleDesc.Count = init.m_sampleCount;
	desc.SampleDesc.Quality = init.m_sampleQuality;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	// Only RTV/DSV need clear value
	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_CLEAR_VALUE* pClearValue = nullptr;
	if (init.m_allowRTV && init.m_rtvClearFormat != DXGI_FORMAT_UNKNOWN)
	{
		clearValue.Format = init.m_rtvClearFormat;
		memcpy(clearValue.Color, init.m_rtvClearColor, sizeof(float) * 4);
		pClearValue = &clearValue;
	}
	if (init.m_allowDSV && init.m_dsvClearFormat != DXGI_FORMAT_UNKNOWN)
	{
		clearValue.Format = init.m_dsvClearFormat;
		clearValue.DepthStencil.Depth = init.m_dsvClearDepth;
		clearValue.DepthStencil.Stencil = init.m_dsvClearStencil;
		pClearValue = &clearValue;
	}

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ID3D12Resource* resource = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		pClearValue,
		IID_PPV_ARGS(&resource)
	);

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create resource for texture.");
	}

#ifdef _DEBUG
	resource->SetName(init.m_debugName);
#endif // _DEBUG

	// Create Texture*
	Texture* texture = new Texture(this);
	texture->m_name = init.m_name;
	texture->m_dimensions = IntVec2(init.m_width, init.m_height);
	texture->m_depth = init.m_depthOrArraySize;
	texture->m_mipLevels = init.m_mipLevels;
	texture->m_sampleCount = init.m_sampleCount;
	texture->m_sampleQuality = init.m_sampleQuality;
	texture->m_format = init.m_format;

	texture->m_supportSRV = init.m_allowSRV;
	texture->m_supportRTV = init.m_allowRTV;
	texture->m_supportDSV = init.m_allowDSV;
	texture->m_supportUAV = init.m_allowUAV;

	texture->m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_COMMON);
	texture->m_resource = resource;
	texture->m_resourceDimension = init.m_resourceDimension;

	if (init.m_allowDSV)
	{
		texture->m_dsvClearDepth = init.m_dsvClearDepth;
		texture->m_dsvClearStencil = init.m_dsvClearStencil;
	}

	// Remember to call method in Renderer to release the texture
	return texture;
}

void DX12Renderer::UpdateTexture(Texture& texture, const void* data)
{
	// Tie 1 version only allow mipLevels = 1
	// not support 3D texture
	GUARANTEE_OR_DIE(texture.m_resourceDimension != DXResourceDimension::TEXTURE3D, "3D texture not support");
	GUARANTEE_OR_DIE(texture.m_mipLevels == 1, "Only support none mip map");

	D3D12_RESOURCE_DESC textureDesc = texture.m_resource->GetDesc();
	UINT texelSize = DXGI_FORMAT_GetStride(texture.m_format);
	UINT width = texture.GetWidth();
	UINT height = texture.GetHeight();
	UINT depth = texture.GetDepth();

	UINT arraySize = depth; // TODO: 3D texture array size is 1

	UINT64 rowPitch = width * texelSize;
	UINT64 slicePitch = rowPitch * height;

	UINT numSubResources = texture.m_mipLevels * arraySize;

	UINT64 uploadBufferSize = 0;
	m_device->GetCopyableFootprints(&textureDesc, 0, numSubResources, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	//-----------------------------------------------------------------------------------------------
	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	ID3D12Resource* uploadBuffer = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create texture upload heap");

	uploadBuffer->SetName(L"Texture update upload buffer");

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubResources);
	const uint8_t* src = static_cast<const uint8_t*>(data);
	for (UINT i = 0; i < numSubResources; ++i) // TODO: two for loop outer: arraySize inner: mipLevels
	{
		subresources[i].pData = src + i * slicePitch;
		subresources[i].RowPitch = rowPitch;
		subresources[i].SlicePitch = slicePitch;
	}

	UpdateSubresources(m_commandList, texture.m_resource, uploadBuffer, 0, 0, numSubResources, subresources.data());

	EnqueueDeferredRelease(uploadBuffer);
}

void DX12Renderer::CopyTexture(Texture& dst, Texture& src)
{
	// If want to copy to different format resource, use compute shader to do type transform
	GUARANTEE_OR_DIE(dst.GetDimensions() == src.GetDimensions(), "Texture Dimensions not same");
	GUARANTEE_OR_DIE(dst.GetDepth() == src.GetDepth(), "Texture Height not same");
	GUARANTEE_OR_DIE(dst.m_format == src.m_format, "Texture format not same");

	TransitionToCopySource(src);
	TransitionToCopyDest(dst);

	m_commandList->CopyResource(dst.m_resource, src.m_resource);
}

void DX12Renderer::DestroyTexture(Texture*& texture)
{
	if (texture != nullptr)
	{
		//EnqueueDeferredRelease(texture->m_resource);
		delete texture;
		texture = nullptr;
	}
}

DescriptorHandle DX12Renderer::AllocateSRV(Texture& texture, DXGI_FORMAT format /*= DXGI_FORMAT_UNKNOWN*/, bool isCubeMap /*= false*/)
{
	if (!texture.m_supportSRV)
		return {};

	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvDesc.Format = (format != DXGI_FORMAT_UNKNOWN) ? format : texture.m_format;

	if (isCubeMap) 
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = texture.m_mipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else 
	{
		switch (texture.m_resourceDimension)
		{
		case DXResourceDimension::TEXTURE1D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = texture.m_mipLevels;
			srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
			break;
		case DXResourceDimension::TEXTURE2D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = texture.m_mipLevels;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			break;
		case DXResourceDimension::TEXTURE3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = texture.m_mipLevels;
			srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
			break;
		default:
			return {};
		}
	}

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(texture.m_resource, &srvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateUAV(Texture& texture, DXGI_FORMAT format /*= DXGI_FORMAT_UNKNOWN*/)
{
	if (!texture.m_supportUAV)
		return {};

	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = (format != DXGI_FORMAT_UNKNOWN) ? format : texture.m_format;

	switch (texture.m_resourceDimension)
	{
	case DXResourceDimension::TEXTURE1D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = 0;
		break;
	case DXResourceDimension::TEXTURE2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;
		break;
	case DXResourceDimension::TEXTURE3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = 0;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = UINT(-1);
		break;
	default:
		return {};
	}

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateUnorderedAccessView(texture.m_resource, nullptr, &uavDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateRTV(Texture& texture, DXGI_FORMAT format /*= DXGI_FORMAT_UNKNOWN*/)
{
	if (!texture.m_supportRTV)
		return {};

	PersistentDescriptorAlloc alloc = m_rtvDescriptorHeap->AllocatePersistent();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = (format != DXGI_FORMAT_UNKNOWN) ? format : texture.m_format;

	switch (texture.m_resourceDimension)
	{
	case DXResourceDimension::TEXTURE1D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		rtvDesc.Texture1D.MipSlice = 0;
		break;
	case DXResourceDimension::TEXTURE2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		break;
	case DXResourceDimension::TEXTURE3D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.MipSlice = 0;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = UINT(-1);
		break;
	default:
		return {};
	}

	for (uint32_t i = 0; i < m_rtvDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateRenderTargetView(texture.m_resource, &rtvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::RTV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateDSV(Texture& texture, DXGI_FORMAT format /*= DXGI_FORMAT_UNKNOWN*/)
{
	if (!texture.m_supportDSV)
		return {};

	PersistentDescriptorAlloc alloc = m_dsvDescriptorHeap->AllocatePersistent();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = (format != DXGI_FORMAT_UNKNOWN) ? format : texture.m_format;

	switch (texture.m_resourceDimension)
	{
	case DXResourceDimension::TEXTURE1D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		dsvDesc.Texture1D.MipSlice = 0;
		break;
	case DXResourceDimension::TEXTURE2D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		break;
	default:
		return {};
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	for (uint32_t i = 0; i < m_dsvDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateDepthStencilView(texture.m_resource, &dsvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::DSV;
	result.m_index = alloc.m_index;
	return result;
}

uint32_t DX12Renderer::AllocateTempConstantBuffer(size_t bufferSize, const void* bufferData)
{
	DynAlloc bufferAlloc = GetCurrentLinearAllocator()->Allocate(bufferSize);
	memcpy(bufferAlloc.m_dataPtr, bufferData, bufferSize);

	TempDescriptorAlloc descriptorAlloc = m_cbvSrvUavDescriptorHeap->AllocateTemporary(1);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = bufferAlloc.m_gpuAddress;
	cbvDesc.SizeInBytes = (UINT)bufferAlloc.m_size;

	m_device->CreateConstantBufferView(&cbvDesc, descriptorAlloc.m_startCPUHandle);

	return descriptorAlloc.m_startIndex;
}

uint32_t DX12Renderer::AllocateTempStructuredBuffer(size_t bufferSize, const void* bufferData, unsigned int elementSize, unsigned int numElements)
{
	GUARANTEE_OR_DIE(elementSize > 0, "Element size is not positive.");
	DynAlloc bufferAlloc = GetCurrentLinearAllocator()->AllocateAnyAlign(bufferSize, elementSize);
	GUARANTEE_OR_DIE(bufferAlloc.m_offset % elementSize == 0, "Allocated memory is not aligned");

	memcpy(bufferAlloc.m_dataPtr, bufferData, bufferSize);

	TempDescriptorAlloc descriptorAlloc = m_cbvSrvUavDescriptorHeap->AllocateTemporary(1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = (bufferAlloc.m_offset / elementSize);
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = elementSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_device->CreateShaderResourceView(bufferAlloc.m_buffer, &srvDesc, descriptorAlloc.m_startCPUHandle);

	return descriptorAlloc.m_startIndex;
}

uint32_t DX12Renderer::AllocateTempFormattedBuffer(size_t bufferSize, const void* bufferData, DXGI_FORMAT format, unsigned int numElements)
{
	uint32_t stride = DXGI_FORMAT_GetStride(format);
	GUARANTEE_OR_DIE(stride > 0, "Unknown Format");

	// #ToDo: check if the stride is power of 2, then use ->Allocate
	DynAlloc bufferAlloc = GetCurrentLinearAllocator()->AllocateAnyAlign(bufferSize, stride);
	GUARANTEE_OR_DIE(bufferAlloc.m_offset % stride == 0, "Allocated memory is not aligned");

	memcpy(bufferAlloc.m_dataPtr, bufferData, bufferSize);

	TempDescriptorAlloc descriptorAlloc = m_cbvSrvUavDescriptorHeap->AllocateTemporary(1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = (bufferAlloc.m_offset / stride);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = numElements;
	m_device->CreateShaderResourceView(bufferAlloc.m_buffer, &srvDesc, descriptorAlloc.m_startCPUHandle);

	return descriptorAlloc.m_startIndex;
}

uint32_t DX12Renderer::AllocateTempRawBuffer(size_t bufferSize, const void* bufferData, unsigned int numElements)
{
	uint32_t const stride = 4;

	DynAlloc bufferAlloc = GetCurrentLinearAllocator()->Allocate(bufferSize, stride);
	GUARANTEE_OR_DIE(bufferAlloc.m_offset % stride == 0, "Allocated memory is not aligned");

	memcpy(bufferAlloc.m_dataPtr, bufferData, bufferSize);

	TempDescriptorAlloc descriptorAlloc = m_cbvSrvUavDescriptorHeap->AllocateTemporary(1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = (bufferAlloc.m_offset / stride);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	srvDesc.Buffer.NumElements = numElements;
	m_device->CreateShaderResourceView(bufferAlloc.m_buffer, &srvDesc, descriptorAlloc.m_startCPUHandle);

	return descriptorAlloc.m_startIndex;
}

Buffer* DX12Renderer::CreateBuffer(BufferInit const& init)
{
	Buffer* newBuffer = new Buffer();

	uint64_t bufferSize = init.m_size;
	if (init.m_isConstantBuffer)
	{
		bufferSize = RendererMath::AlignUp(bufferSize, 256);
	}

	newBuffer->m_size = bufferSize;
	newBuffer->m_allowUnorderedAccess = init.m_allowUnorderedAccess;
	newBuffer->m_currentResourceState = static_cast<unsigned int>(D3D12_RESOURCE_STATE_COMMON);
	
	D3D12_RESOURCE_FLAGS flags = init.m_allowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&newBuffer->m_resource));

	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create buffer.");
	}

#ifdef _DEBUG
	newBuffer->m_resource->SetName(init.m_name);
#endif // _DEBUG

	return newBuffer;
}

void DX12Renderer::UpdateBuffer(Buffer& buffer, size_t dataSize, void const* bufferData, size_t dstOffset /*= 0*/)
{
	DynAlloc uploadAlloc = GetCurrentLinearAllocator()->Allocate(dataSize);
	memcpy(uploadAlloc.m_dataPtr, bufferData, dataSize);

	TransitionResource(buffer, D3D12_RESOURCE_STATE_COPY_DEST);

	m_commandList->CopyBufferRegion(
		buffer.m_resource,
		dstOffset,
		uploadAlloc.m_buffer,
		uploadAlloc.m_offset,
		dataSize
	);
}

void DX12Renderer::DestroyBuffer(Buffer*& buffer)
{
	if (buffer != nullptr)
	{
		EnqueueDeferredRelease(buffer->m_resource);
		delete buffer;
		buffer = nullptr;
	}
}

DescriptorHandle DX12Renderer::AllocateConstantBufferView(Buffer const& buffer)
{
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = buffer.m_resource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)buffer.m_size;

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateConstantBufferView(&cbvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateStructuredBufferSRV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = firstElement;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = elementSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(buffer.m_resource, &srvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateStructuredBufferUAV(Buffer const& buffer, unsigned int elementSize, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	GUARANTEE_OR_DIE(buffer.m_allowUnorderedAccess, "Buffer must be created with allowUnordererAccess=true");

	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.CounterOffsetInBytes = 0; // Append&Consume Buffer
	uavDesc.Buffer.FirstElement = firstElement;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.StructureByteStride = elementSize;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;


	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateUnorderedAccessView(buffer.m_resource, nullptr, &uavDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateRawBufferSRV(const Buffer& buffer, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.Buffer.FirstElement = firstElement;
	srvDesc.Buffer.NumElements = numElements; // 4 bytes per element
	srvDesc.Buffer.StructureByteStride = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW; // must have raw flag

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(buffer.m_resource, &srvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateRawBufferUAV(const Buffer& buffer, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	GUARANTEE_OR_DIE(buffer.m_allowUnorderedAccess, "Buffer must be created with allowUnordererAccess=true");

	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = { };
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.CounterOffsetInBytes = 0; // Append&Consume Buffer
	uavDesc.Buffer.FirstElement = firstElement;
	uavDesc.Buffer.NumElements = numElements; // 4 bytes per element
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW; // must have raw flag


	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateUnorderedAccessView(buffer.m_resource, nullptr, &uavDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateFormattedBufferSRV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.Buffer.FirstElement = firstElement;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateShaderResourceView(buffer.m_resource, &srvDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

DescriptorHandle DX12Renderer::AllocateFormattedBufferUAV(Buffer const& buffer, DXGI_FORMAT format, unsigned int numElements, uint64_t firstElement /*= 0*/)
{
	GUARANTEE_OR_DIE(buffer.m_allowUnorderedAccess, "Buffer must be created with allowUnordererAccess=true");

	PersistentDescriptorAlloc alloc = m_cbvSrvUavDescriptorHeap->AllocatePersistent();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = { };
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = format;
	uavDesc.Buffer.CounterOffsetInBytes = 0; // Append&Consume Buffer
	uavDesc.Buffer.FirstElement = firstElement;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;


	for (uint32_t i = 0; i < m_cbvSrvUavDescriptorHeap->GetNumHeaps(); ++i)
	{
		m_device->CreateUnorderedAccessView(buffer.m_resource, nullptr, &uavDesc, alloc.m_handles[i]);
	}

	DescriptorHandle result;
	result.m_type = DescriptorHeapType::CBV_SRV_UAV;
	result.m_index = alloc.m_index;
	return result;
}

void DX12Renderer::TransitionToGenericRead(Buffer& buffer)
{
	TransitionResource(buffer, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void DX12Renderer::TransitionToUnorderedAccess(Buffer& buffer)
{
	TransitionResource(buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void DX12Renderer::TransitionToUnorderedAccess(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void DX12Renderer::TransitionToDepthWrite(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DX12Renderer::TransitionToPixelShaderResource(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12Renderer::TransitionToAllShaderResource(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DX12Renderer::TransitionToCopyDest(Buffer& buffer)
{
	TransitionResource(buffer, D3D12_RESOURCE_STATE_COPY_DEST);
}


void DX12Renderer::TransitionToCopyDest(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_COPY_DEST);
}

void DX12Renderer::TransitionToCopySource(Buffer& buffer)
{
	TransitionResource(buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

void DX12Renderer::TransitionToCopySource(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

void DX12Renderer::TransitionToRenderTarget(Texture& texture)
{
	TransitionResource(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DX12Renderer::AddUAVBarrier(Buffer& buffer)
{
	D3D12_RESOURCE_BARRIER barrier = { };
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = buffer.m_resource;
	m_commandList->ResourceBarrier(1, &barrier);
}

void DX12Renderer::AddUAVBarrier(Texture& texture)
{
	D3D12_RESOURCE_BARRIER barrier = { };
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = texture.m_resource;
	m_commandList->ResourceBarrier(1, &barrier);
}

void DX12Renderer::CopyBuffer(Buffer& dst, size_t dstOffset, Buffer& src, size_t srcOffset, size_t numBytes)
{
	TransitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);

	m_commandList->CopyBufferRegion(
		dst.m_resource, dstOffset,
		src.m_resource, srcOffset,
		numBytes
	);
	// User need to change the resource state back
}

void DX12Renderer::TransitionResource(Buffer& buffer, D3D12_RESOURCE_STATES newState)
{
	if (static_cast<D3D12_RESOURCE_STATES>(buffer.m_currentResourceState) != newState)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.m_resource, static_cast<D3D12_RESOURCE_STATES>(buffer.m_currentResourceState), newState);
		m_commandList->ResourceBarrier(1, &barrier);
		buffer.m_currentResourceState = static_cast<unsigned int>(newState);
	}
}

void DX12Renderer::TransitionResource(Texture& texture, D3D12_RESOURCE_STATES newState)
{
	if (static_cast<D3D12_RESOURCE_STATES>(texture.m_currentResourceState) != newState)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.m_resource, static_cast<D3D12_RESOURCE_STATES>(texture.m_currentResourceState), newState);
		m_commandList->ResourceBarrier(1, &barrier);
		texture.m_currentResourceState = static_cast<unsigned int>(newState);
	}
}

void DX12Renderer::SetGraphicsRootSignature(ID3D12RootSignature* rootSig)
{
	if (rootSig == m_curGraphicsRootSignature)
		return;

	m_commandList->SetGraphicsRootSignature(rootSig);
	m_desiredGraphicsPSO->SetRootSignature(rootSig);
	m_curGraphicsRootSignature = rootSig;
}

void DX12Renderer::SetComputeRootSignature(ID3D12RootSignature* rootSig)
{
	if (rootSig == m_curComputeRootSignature)
		return;

	m_commandList->SetComputeRootSignature(rootSig);
	m_desiredComputePSO->SetRootSignature(rootSig);
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

void DX12Renderer::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, float color[4], D3D12_RECT* rect /*= nullptr*/)
{
	m_commandList->ClearRenderTargetView(rtv, color, (rect == nullptr) ? 0 : 1, rect);
}

void DX12Renderer::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth /*= 1.f*/, uint8_t clearStencil /*= 0*/)
{
	m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, clearDepth, clearStencil, 0, nullptr);
}

void DX12Renderer::ClearStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth /*= 1.f*/, uint8_t clearStencil /*= 0*/)
{
	m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_STENCIL, clearDepth, clearStencil, 0, nullptr);
}

void DX12Renderer::ClearDepthAndStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float clearDepth /*= 1.f*/, uint8_t clearStencil /*= 0*/)
{
	m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearDepth, clearStencil, 0, nullptr);
}

void DX12Renderer::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
	m_commandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

void DX12Renderer::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	m_commandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
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

void DX12Renderer::SetComputeConstantArray(unsigned int rootParameterIndex, unsigned int numConstants, void const* pConstants)
{
	m_commandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, pConstants, 0);
}

void DX12Renderer::Dispatch(unsigned int groupCountX /*= 1*/, unsigned int groupCountY /*= 1*/, unsigned int groupCountZ /*= 1*/)
{
	m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
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
	SetDefaultRenderTargets(); // ImGui needs to be render to full screen quad 
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
