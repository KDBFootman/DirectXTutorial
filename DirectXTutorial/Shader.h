
#pragma once

#include <wrl/client.h>
#include <d3d11_2.h>
#include <DirectXMath.h>

class Shader {

public:
	Shader();
	~Shader() = default;

	HRESULT CreateShader(ID3D11Device* const);
	void SetRenderShader(ID3D11DeviceContext* const, DirectX::XMMATRIX&, DirectX::XMMATRIX&, DirectX::XMMATRIX&);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_VertexLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
};