#pragma once
#include "Engine/Renderer/RendererCommon.hpp"

#ifdef ENGINE_RENDER_D3D12

#include <d3d12.h>

//https://github.com/TheRealMJP/DXRPathTracer
struct PersistentDescriptorAlloc
{
	D3D12_CPU_DESCRIPTOR_HANDLE Handles[FRAMES_IN_FLIGHT] = { };
	uint32_t Index = uint32_t(-1);
};

struct TempDescriptorAlloc
{
	D3D12_CPU_DESCRIPTOR_HANDLE StartCPUHandle = { };
	D3D12_GPU_DESCRIPTOR_HANDLE StartGPUHandle = { };
	uint32_t StartIndex = uint32_t(-1);
};

// Wrapper for Descriptor Heap
class DX12DescriptorHeap
{

private:
	ID3D12DescriptorHeap* Heaps[FRAMES_IN_FLIGHT] = { };

	// Persistent Descriptor
	uint32_t m_numPersistent = 0;
	uint32_t m_persistentAllocated = 0;
	std::vector<uint32_t> m_deadList; // recycled persistent descriptor index, it works like an array-based stack 
	
	uint32_t m_numTemporary = 0;
	int64_t m_temporaryAllocated = 0; 

	uint32_t m_heapIndex = 0;         // current heap index for frame in flight
	uint32_t m_numHeaps = 0;          // = FRAMES_IN_FLIGHT or 1

	uint32_t DescriptorSize = 0;   // size for single descriptor
	bool ShaderVisible = false;     
	D3D12_DESCRIPTOR_HEAP_TYPE HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUStart[FRAMES_IN_FLIGHT] = { };
	D3D12_GPU_DESCRIPTOR_HANDLE GPUStart[FRAMES_IN_FLIGHT] = { };


	~DX12DescriptorHeap();
	void Init(uint32_t numPersistent, uint32_t numTemporary, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool shaderVisible);
	void Shutdown();

	PersistentDescriptorAlloc AllocatePersistent();
	void FreePersistent(uint32_t& idx);
	void FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE& handle);
	void FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE& handle);

	TempDescriptorAlloc AllocateTemporary(uint32_t count);
	void EndFrame();

	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandleFromIndex(uint32_t descriptorIdx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandleFromIndex(uint32_t descriptorIdx) const;

	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIdx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandleFromIndex(uint32_t descriptorIdx, uint64_t heapIdx) const;

	uint32_t IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
	uint32_t IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

	ID3D12DescriptorHeap* GetCurrentHeap() const;
	uint32_t GetTotalNumDescriptors() const { return m_numPersistent + m_numTemporary; }

};

#endif // ENGINE_RENDER_D3D12
