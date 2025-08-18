#pragma once
#include "Game/EngineBuildPreferences.hpp"


/**************************************************************************************
* Buffer
* On Default Heap
* Persistent, 

Constant Buffer:		ConstantBuffer<T>
	- Read only
	- Transform Matrix, Light Parameter, Camera Parameter, Material Constants

Structured Buffer:	StructuredBuffer<T> or RWStructuredBuffer<T>
	- Structured Array, interact with Index
	- Particles, Bones, PerInstance Data array


Formatted Buffer:	Buffer<T> or RWBuffer<T>
	- Buffer<float4> vertexColors, Vertex Color List
	- Buffer<uint> indices, Index Buffer
	- Look Up Table

Raw Buffer:			ByteAddressBuffer or RWByteAddressBuffer
	- Raw Data
	- Packed Date, non uniform length data


// ConstantBuffer: bufferSize = AlignUp(dataSize, 256)
// StructuredBuffer: bufferSize = elementCount * elementSize
// Formatted Buffer, Buffer: bufferSize = elementCount * elementSize
// Raw Buffer, ByteAddressBuffer: 4 bytes are one element
*/


#ifdef ENGINE_RENDER_D3D12

struct ID3D12Resource;

class Buffer
{
	friend class DX12Renderer;

public:
	uint64_t GetSize() const { return m_size; }

private:
	ID3D12Resource* m_resource = nullptr; // Resource->GetDesc()
	uint64_t m_size = 0;
	bool m_allowUnorderedAccess = false;
	unsigned int m_currentResourceState = 0;
	//D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;
};
#endif // ENGINE_RENDER_D3D12

