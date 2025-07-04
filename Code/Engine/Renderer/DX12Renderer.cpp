#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"

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

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "ThirdParty/directx/d3dx12.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#define DX_SAFE_RELEASE(dxObject)								\
{																\
	if (( dxObject ) != nullptr)								\
	{															\
		(dxObject)->Release();									\
		(dxObject) = nullptr;									\
	}															\
}

//-----------------------------------------------------------------------------------------------
static DXGI_FORMAT s_backBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
static DXGI_FORMAT s_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;




// Instance Rendering sample
static const D3D12_INPUT_ELEMENT_DESC g_InputLayout_Instancing[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
	{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
};


//-----------------------------------------------------------------------------------------------
// Sampler Desc
//-----------------------------------------------------------------------------------------------

static const CD3DX12_STATIC_SAMPLER_DESC g_StaticSamplers[] =
{
	// s0: pointWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		0,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP),	// addressW

	// s1: pointClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		1,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP),	// addressW

	// s2: linearWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		2,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP),	// addressW

	// s3: linearClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		3,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP),	// addressW

	// s4: anisotropicWrap
	CD3DX12_STATIC_SAMPLER_DESC(
		4,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,								// mipLODBias
		8),									// maxAnisotropy

	// s5: anisotropicClamp
	CD3DX12_STATIC_SAMPLER_DESC(
		5,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8),
};

























//-----------------------------------------------------------------------------------------------
DX12Renderer::DX12Renderer(RendererConfig const& config)
	: m_config(config)
{

}

void DX12Renderer::Startup()
{
	CreateDevice();
	CreateFenceAndHandleAndDescriptorSizes();
	// Check 4x MSAA Quality Support
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateRenderTargetView();
	CreateDepthStencilBufferAndView();

}

void DX12Renderer::Shutdown()
{
	FlushCommandQueue();
	CloseHandle(m_fenceEvent);

	DX_SAFE_RELEASE(m_dxgiFactory);
	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_device);

	DX_SAFE_RELEASE(m_commandQueue);
	DX_SAFE_RELEASE(m_commandAllocator);
	DX_SAFE_RELEASE(m_commandList);

	DX_SAFE_RELEASE(m_rtvHeap);
	DX_SAFE_RELEASE(m_dsvHeap);

	DX_SAFE_RELEASE(m_rootSignature);
	DX_SAFE_RELEASE(m_pipelineState);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_swapChainBuffer[i]);
	}


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
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		//D3D_FEATURE_LEVEL_12_2,
		IID_PPV_ARGS(&m_device));

	//HRESULT hardwareResult = D3D12CreateDevice(
	//	nullptr,             // default adapter
	//	D3D_FEATURE_LEVEL_11_0,
	//	IID_PPV_ARGS(&m_device));

	if (FAILED(hardwareResult))
	{
		ERROR_AND_DIE("Could not create D3D12 device.");
	}
	DX_SAFE_RELEASE(factory6);
	DX_SAFE_RELEASE(adapter);
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
		IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create command allocator.");
	}

	hr = m_device->CreateCommandList(
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		m_commandAllocator, 
		nullptr, //m_pipelineState, 
		IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not create the command list.")
	}

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	hr = m_commandList->Close();
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Could not close the command list.")
	}
}

void DX12Renderer::CreateSwapChain()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_config.m_window->GetClientDimensions().x;
	swapChainDesc.Height = m_config.m_window->GetClientDimensions().y;
	swapChainDesc.Format = s_backBufferFormat;
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
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::CreateDescriptorHeaps()
{
	HRESULT hr;

	// Describe and create a render target view (RTV) descriptor heap.
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

void DX12Renderer::CreateRenderTargetView()
{
	// #ToDo OnResize() will do lots of stuff
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
	depthStencilDesc.Format = s_depthStencilFormat; // If using SSAO, depth stencil needs to be changed
	//depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = s_depthStencilFormat;
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
	m_device->CreateDepthStencilView(
		m_depthStencilBuffer,
		nullptr, // This behavior inherits the resource format and dimension(if not typeless)
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Format = s_depthStencilFormat;
	//dsvDesc.Texture2D.MipSlice = 0;
	//m_device->CreateDepthStencilView(m_depthStencilBuffer, &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::SetDebugNameForBasicDXObjects()
{
	m_device->SetName(L"Device");

}

void DX12Renderer::SampleSetViewPortAndScissorRect()
{
	//D3D12_VIEWPORT viewport = {};
	//viewport.TopLeftX = 0.f;
	//viewport.TopLeftY = 0.f;
	//viewport.Width = 200.f;
	//viewport.Height = 100.f;
	//viewport.MinDepth = 0.0f;
	//viewport.MaxDepth = 1.0f;

	//D3D12_RECT scissorRect = {};
	//scissorRect.left = 0;
	//scissorRect.right = (LONG)viewport.Width;
	//scissorRect.top = 0;
	//scissorRect.bottom = (LONG)viewport.Height;

	//m_commandList->RSSetViewports(1, &viewport);
	//m_commandList->RSSetScissorRects(1, &scissorRect);
	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
}

void DX12Renderer::FlushCommandQueue()
{
	HRESULT hr;

	const uint64_t fenceValueForSignal = ++m_currentFenceValue;

	hr = m_commandQueue->Signal(m_fence, fenceValueForSignal);
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to signal.");
	}

	if (m_fence->GetCompletedValue() < fenceValueForSignal)
	{
		hr = m_fence->SetEventOnCompletion(fenceValueForSignal, m_fenceEvent);
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Failed to set the fence event");
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex(); // #ToDo check if needs to put here
}


/*
void InitDirect3DApp::Draw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));



	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);


	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	//-----------------------------------------------------------------------------------------------


	// Clear the back buffer and depth buffer. // Clear Screen
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);



	//-----------------------------------------------------------------------------------------------
	// End Frame
	//-----------------------------------------------------------------------------------------------
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}
*/