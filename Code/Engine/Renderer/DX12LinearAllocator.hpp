#pragma once
#include "Game/EngineBuildPreferences.hpp"

#ifdef ENGINE_RENDER_D3D12
#include <d3d12.h>

struct DynAlloc
{
	ID3D12Resource* m_buffer = nullptr; // Base Resource

	void* m_dataPtr = nullptr; // cpu-writeable address
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress; // gpu-visible address
	size_t m_size;	// reserved size of this allocation
	size_t m_offset; // offset from start of buffer resource
};


class DX12LinearAllocator
{
public:
	DX12LinearAllocator(ID3D12Device* device, size_t totalSize);
	~DX12LinearAllocator();

	// Alignment: cb - 256; vb/ib - 4 or16
	DynAlloc Allocate(size_t sizeInBytes, size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	DynAlloc AllocateAnyAlign(size_t sizeInBytes, size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	// Reset after the frame has complete its GPU work
	void Reset();

	//ID3D12Resource* GetResource() const { return m_uploadBuffer; }

private:
	size_t m_totalSize;
	size_t m_curOffset;
	ID3D12Resource* m_uploadBuffer = nullptr;
	void* m_mappedPtr = nullptr;
};
#endif // ENGINE_RENDER_D3D12


/*
constexpr int FrameCount = 3;
DX12LinearAllocator* m_frameAllocators[FrameCount];

// initialization
for (int i = 0; i < FrameCount; ++i)
{
	m_frameAllocators[i] = std::make_unique<LinearAllocator>(device, 2 * 1024 * 1024); // 2MB
}

// remember to clear it at the beginning of this frame, before record command
void BeginFrame(UINT frameIndex)
{
	m_frameAllocators[frameIndex]->Reset();
}

auto alloc = m_frameAllocators[frameIndex]->Allocate(sizeof(MyCB));
memcpy(alloc.cpuPtr, &cbData, sizeof(MyCB));
D3D12_GPU_VIRTUAL_ADDRESS cbAddress = alloc.gpuAddress;

struct VSConstants
{
	Matrix4 modelToProjection;
	Matrix4 modelToShadow;
	XMFLOAT3 viewerPos;
} vsConstants;
vsConstants.modelToProjection = ViewProjMat;
vsConstants.modelToShadow = m_SunShadow.GetShadowMatrix();
XMStoreFloat3(&vsConstants.viewerPos, viewerPos);

gfxContext.SetDynamicConstantBufferView(Renderer::kMeshConstants, sizeof(vsConstants), &vsConstants);


inline void GraphicsContext::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	memcpy(cb.DataPtr, BufferData, BufferSize);
	m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void Render(UINT frameIndex)
{
	// 1. Begin Frame reset linear allocator
	m_frameAllocators[frameIndex]->Reset();

	// 2. 
	auto alloc = m_frameAllocators[frameIndex]->Allocate(sizeof(MyCB));
	memcpy(alloc.cpuPtr, &cb, sizeof(MyCB));

	// 3. 
	cmdList->SetGraphicsRootConstantBufferView(0, alloc.gpuAddress);

	// ...
}
*/
