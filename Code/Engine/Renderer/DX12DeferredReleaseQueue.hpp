#pragma once
#include "Game/EngineBuildPreferences.hpp"

#ifdef ENGINE_RENDER_D3D12
#include <vector>
#include <cstdint>

struct IUnknown;

struct DeferredReleaseItem
{
	uint64_t fenceValue;
	IUnknown* resource;
};

class DX12DeferredReleaseQueue
{
public:
	DX12DeferredReleaseQueue() = default;
	~DX12DeferredReleaseQueue(); // Remember to Process all fence value, release all objects

	DX12DeferredReleaseQueue(const DX12DeferredReleaseQueue&) = delete;
	DX12DeferredReleaseQueue& operator=(const DX12DeferredReleaseQueue&) = delete;

	void Enqueue(uint64_t fenceValue, IUnknown* resource);
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


