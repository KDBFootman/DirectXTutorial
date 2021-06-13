
#pragma once

#include <DirectXMath.h>

__declspec(align(16))
class Camera {

public:
	Camera();
	~Camera() = default;

	void SetCamera(const DirectX::XMVECTOR, const DirectX::XMVECTOR);
	void SetScreen(const float, const float, const float, const float);

	DirectX::XMMATRIX GetView() const { return m_View; }
	DirectX::XMMATRIX GetProjection() const { return m_Projection; }

private:
	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;
};