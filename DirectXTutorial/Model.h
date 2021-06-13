
#pragma once

#include <functional>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <DirectXMath.h>

__declspec(align(16))
class Model {

public:
	Model();
	~Model() = default;

	HRESULT CreateModel(ID3D11Device* const);
	void MoveModel(_In_opt_ std::function<DirectX::XMMATRIX()> customFunction = nullptr);
	void RenderModel(ID3D11DeviceContext* const);

	DirectX::XMMATRIX GetWorld() const { return m_World; }

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;

	DirectX::XMMATRIX m_World;
};