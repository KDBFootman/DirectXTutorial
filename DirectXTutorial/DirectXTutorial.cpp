
#include <windows.h>
#include <memory>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "Device.h"

using namespace DirectX;

namespace
{
	//--------------------------------------------------------------------------------------
	// Helper for compiling shaders
	//--------------------------------------------------------------------------------------
	HRESULT CompileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
		HRESULT hr = S_OK;

		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined (DEBUG)||(_DEBUG)
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows
		// the shaders to be optimized and to run exactly the way they will run in
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;
		// Disable optimizations to further improve shader debugging
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
		hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, pErrorBlob.ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob) {
				OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
				pErrorBlob.Reset();
			}
			return hr;
		}
		pErrorBlob.Reset();

		return S_OK;
	}
}

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE				g_hInst = nullptr;
HWND					g_hWnd = nullptr;

Microsoft::WRL::ComPtr<ID3D11VertexShader> g_VertexShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11PixelShader> g_PixelShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11InputLayout> g_VertexLayout = nullptr;
Microsoft::WRL::ComPtr<ID3D11Buffer> g_VertexBuffer = nullptr;

std::unique_ptr<Device>	g_Device;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE, int);
HRESULT InitDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Move();
void Render();
void Clear();
void CleanupDevice();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow))) {
		return 0;
	}

	g_Device = std::make_unique<Device>();

	if (FAILED(InitDevice())) {
		CleanupDevice();
		g_Device.reset();
		return 0;
	}

	MSG msg = {};
	while (msg.message != WM_QUIT) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Move();
			Render();
		}
	}

	CleanupDevice();
	g_Device.reset();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow) {
	
	// Register class
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(hInstance, IDI_WINLOGO);

	if (!RegisterClassEx(&wcex)) {
		return E_FAIL;
	}

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0,0,720,480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	// For fullscreen
	//g_hWnd = CreateWindowEx(WS_EX_TOPMOST, L"TutorialWindowClass", L"DirectX Tutorial", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	g_hWnd = CreateWindowEx(0, L"TutorialWindowClass", L"DirectX Tutorial", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd) {
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice() {

	HRESULT hr = S_OK;

	RECT rc = {};
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	g_Device->SetWindow(g_hWnd, width, height);

	hr = g_Device->CreateDevice();
	if (FAILED(hr)) {
		return hr;
	}

	// Compile the vertex shader
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial.hlsli", "VS", "vs_4_0", vsBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_Device->GetD3DDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, g_VertexShader.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		vsBlob.Reset();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_Device->GetD3DDevice()->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), g_VertexLayout.ReleaseAndGetAddressOf());
	vsBlob.Reset();
	if (FAILED(hr)) {
		return hr;
	}

	// Set the input layout
	//g_Device->GetD3DDeviceContext()->IASetInputLayout(g_VertexLayout.Get());

	// Compile the pixel shader
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial.hlsli", "PS", "ps_4_0", psBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_Device->GetD3DDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, g_PixelShader.ReleaseAndGetAddressOf());
	psBlob.Reset();
	if (FAILED(hr)) {
		return hr;
	}

	// Create the vertex buffer
	SimpleVertex vertices[] =
	{
		XMFLOAT3(0.0f,0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,-0.5f,0.5f),
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;

	hr = g_Device->GetD3DDevice()->CreateBuffer(&bd, &InitData, g_VertexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return hr;
	}

	// Set vertex buffer
	//UINT stride = sizeof(SimpleVertex);
	//UINT offset = 0;
	//g_Device->GetD3DDeviceContext()->IASetVertexBuffers(0, 1, g_VertexBuffer.GetAddressOf(), &stride, &offset);

	// Set primitive topology
	//g_Device->GetD3DDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT ps = {};
	HDC hdc = nullptr;

	switch (message) {

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Move the frame
//--------------------------------------------------------------------------------------
void Move() {

}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render() {

	Clear();

	// Set Vertex & Pixel Shader
	g_Device->GetD3DDeviceContext()->VSSetShader(g_VertexShader.Get(), nullptr, 0);
	g_Device->GetD3DDeviceContext()->PSSetShader(g_PixelShader.Get(), nullptr, 0);

	// Set the input layout
	g_Device->GetD3DDeviceContext()->IASetInputLayout(g_VertexLayout.Get());

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_Device->GetD3DDeviceContext()->IASetVertexBuffers(0, 1, g_VertexBuffer.GetAddressOf(), &stride, &offset);

	// Set primitive topology
	g_Device->GetD3DDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Render a triangle
	g_Device->GetD3DDeviceContext()->Draw(3, 0);

	g_Device->Present();
}


//--------------------------------------------------------------------------------------
// Just clear the backbuffer
//--------------------------------------------------------------------------------------
void Clear() {

	auto context = g_Device->GetD3DDeviceContext();
	auto renderTarget = g_Device->GetRenderTargetView();

	float ClearColor[4] = { 0.0f,0.125f,0.3f,1.0f };
	context->ClearRenderTargetView(renderTarget, ClearColor);
	context->OMSetRenderTargets(1, &renderTarget, nullptr);

	// Set the viewport
	auto viewport = g_Device->GetScreenViewport();
	context->RSSetViewports(1, &viewport);
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice() {

	auto context = g_Device->GetD3DDeviceContext();
	context->ClearState();

	g_VertexBuffer.Reset();
	g_VertexLayout.Reset();
	g_VertexShader.Reset();
	g_PixelShader.Reset();
}