#pragma once
#include "Engine/Renderer/RendererCommon.hpp"

#ifdef ENGINE_RENDER_D3D12

#include <d3d12.h>

//https://github.com/TheRealMJP/DXRPathTracer
struct PersistentDescriptorAlloc
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_handles[FRAMES_IN_FLIGHT] = { }; // need to m_device->CreateXXXView(resource, resDesc, )
	uint32_t m_index = uint32_t(-1);
};

struct TempDescriptorAlloc
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_startCPUHandle = { };
	D3D12_GPU_DESCRIPTOR_HANDLE m_startGPUHandle = { };
	uint32_t m_startIndex = uint32_t(-1);
};

// Wrapper for Descriptor Heap
class DX12DescriptorHeap
{
public:
	DX12DescriptorHeap(uint32_t numPersistent, uint32_t numTemporary, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible);
	~DX12DescriptorHeap();

	void BeginFrame();
	
	PersistentDescriptorAlloc AllocatePersistent();
	void FreePersistent(uint32_t& idx);

	TempDescriptorAlloc AllocateTemporary(uint32_t count);

	// Get Current Frame Handle from Index
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandleFromIndex(uint32_t descriptorIdx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandleFromIndex(uint32_t descriptorIdx) const;
	uint32_t GetDescriptorSize() const { return m_descriptorSize; }
	ID3D12DescriptorHeap* GetCurrentHeap() const;
	uint32_t GetTotalNumDescriptors() const { return m_numPersistent + m_numTemporary; }
	uint32_t GetNumHeaps() const { return m_numHeaps; }

private:
	ID3D12DescriptorHeap* m_heaps[FRAMES_IN_FLIGHT] = { };

	// Persistent Descriptor
	uint32_t m_numPersistent = 0;
	uint32_t m_persistentAllocated = 0;
	std::vector<bool> m_persistentInUse;
	std::vector<uint32_t> m_deadList; // recycled persistent descriptor index, it works like an array-based stack 
	
	uint32_t m_numTemporary = 0;
	uint32_t m_temporaryAllocated = 0;

	uint32_t m_heapIndex = 0;         // current heap index for frame in flight
	uint32_t m_numHeaps = 0;          // = FRAMES_IN_FLIGHT or 1

	uint32_t m_descriptorSize = 0;   // size for single descriptor
	bool m_isShaderVisible = false;     

	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart[FRAMES_IN_FLIGHT] = { };
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart[FRAMES_IN_FLIGHT] = { };

	// Do not store cpu/gpu handle, if it is using frame in flight, just use index
	//void FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE& handle);
	//void FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE& handle);
	//uint32_t IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
	//uint32_t IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;


	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const;
};

#endif // ENGINE_RENDER_D3D12
