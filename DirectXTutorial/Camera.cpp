
#include "Camera.h"

using namespace DirectX;

namespace
{
	const XMVECTOR Up = { 0.0f,1.0f,0.0f,0.0f };
}


//--------------------------------------------------------------------------------------
Camera::Camera()
	:m_View()
	,m_Projection()
{
}


//--------------------------------------------------------------------------------------
void Camera::SetCamera(const XMVECTOR eye, const XMVECTOR at) {

	m_View = XMMatrixLookAtLH(eye, at, Up);
}

//--------------------------------------------------------------------------------------
void Camera::SetScreen(const float angle, const float aspect, const float nearZ, const float farZ) {

	m_Projection = XMMatrixPerspectiveFovLH(angle, aspect, nearZ, farZ);
}