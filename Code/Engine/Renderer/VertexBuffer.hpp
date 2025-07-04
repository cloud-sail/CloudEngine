#pragma once

//-----------------------------------------------------------------------------------------------
struct ID3D11Device;
struct ID3D11Buffer;

//-----------------------------------------------------------------------------------------------
class VertexBuffer
{
	friend class Renderer;

public:
	VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);
	VertexBuffer(const VertexBuffer& copy) = delete;
	virtual ~VertexBuffer();

	void Create();
	void Resize(unsigned int size);

	unsigned int GetSize();
	unsigned int GetStride();

private:
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	unsigned int m_size = 0; // total Size of the buffer in bytes
	unsigned int m_stride = 0;
};



