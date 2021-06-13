
#pragma once

#include <wrl/client.h>
#include <d3d11_2.h>

class Model {

public:
	Model();
	~Model() = default;

	HRESULT CreateBuffer(ID3D11Device* const);
	void RenderModel(ID3D11DeviceContext* const);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
};