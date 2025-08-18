#pragma once
#include "Game/EngineBuildPreferences.hpp"

#ifdef ENGINE_RENDER_D3D12
#include <vector>
#include <cstdint>

struct IUnknown;
class DX12DescriptorHeap;


enum class DeferredReleaseType
{
	Resource,
	Descriptor
};


struct DeferredReleaseItem
{
	uint64_t fenceValue;
	DeferredReleaseType type = DeferredReleaseType::Resource;

	// Resource
	IUnknown* resource = nullptr;

	// Descriptor
	DX12DescriptorHeap* heap = nullptr;
	uint32_t index = uint32_t(-1);

	DeferredReleaseItem(uint64_t f, IUnknown* res)
		: fenceValue(f)
		, type(DeferredReleaseType::Resource)
		, resource(res) 
	{}

	DeferredReleaseItem(uint64_t f, DX12DescriptorHeap* h, uint32_t idx)
		: fenceValue(f)
		, type(DeferredReleaseType::Descriptor)
		, heap(h)
		, index(idx)
	{}
};

class DX12DeferredReleaseQueue
{
public:
	DX12DeferredReleaseQueue() = default;
	~DX12DeferredReleaseQueue(); // Remember to Process all fence value, release all objects

	DX12DeferredReleaseQueue(const DX12DeferredReleaseQueue&) = delete;
	DX12DeferredReleaseQueue& operator=(const DX12DeferredReleaseQueue&) = delete;

	void EnqueueResource(uint64_t fenceValue, IUnknown* resource);
	void EnqueueDescriptor(uint64_t fenceValue, DX12DescriptorHeap* heap, uint32_t index);
	void Process(uint64_t completedFenceValue);



private:
	std::vector<DeferredReleaseItem> m_queue;

};
#endif // ENGINE_RENDER_D3D12


/*
Usage:
g_deferredReleaseQueue.Enqueue(currentFenceValue, resource);

DX12Renderer::EndFrame()
{
	//UINT64 completedFenceValue = m_fence->GetCompletedValue();
	g_deferredReleaseQueue.Process(completedFenceValue);
}
*/


