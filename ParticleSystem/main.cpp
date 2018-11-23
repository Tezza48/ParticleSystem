#include <chrono>
//#include <comdef.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <queue>
#include <stdio.h>
#include "ParticleEmitter.h"
#include "ParticleShader.h"
#include "Renderer.h"
#include "WindowManager.h"
#include "Utils.h"

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

using std::puts;
using std::printf;
using std::queue;
using namespace DirectX;
using namespace std::chrono;

#if DEBUG || _DEBUG
ID3D11Debug * debug;
#endif // DEBUG || _DEBUG

const int WIDTH = 800, HEIGHT = 800;
const int MAX_FRAMERATE_TIMES = 5;

bool isRunning = true;

MSG msg;

WindowManager window;
Renderer renderer;

ParticleEmitter emitter;
ParticleShader particleShader;

//UINT numParticles = 100;

void Start();
void Update(float dt, float t);
void Draw(float dt, float t);

float CalcAvgFramerate(queue<float> & buffer);

void Cleanup(void)
{
#if DEBUG || _DEBUG
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	SafeRelease(debug);
#endif // DEBUG || _DEBUG
}

int main(int argc, char ** argv)
{
	atexit(::Cleanup);

	// Create Win32 Window
	window = WindowManager();
	if (!window.Init(WIDTH, HEIGHT, ::WindowProc))
		return false;
	puts("Window Created Successfully");

	// Init D3D
	if (!renderer.Init()) return -1;
	if (!renderer.OnResize(WIDTH, HEIGHT, window.GetHandle())) return -1;

#if DEBUG || _DEBUG
	// 
	HRESULT hr = renderer.GetDevice()->QueryInterface(IID_PPV_ARGS(&debug));
	if (FAILED(hr)) return false;
#endif

	// Start
	Start();

	auto startTime = high_resolution_clock::now();
	auto lastTime = high_resolution_clock::now();

	queue<float> framerateBuffer;
	// Run
	while (isRunning)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{

			auto thisTime = high_resolution_clock::now();
			duration<float> delta = thisTime - lastTime;
			lastTime = thisTime;
			float dt = delta.count();

			framerateBuffer.push(dt);
			duration<float> appTime = thisTime - startTime;

			Update(dt, appTime.count());
			Draw(dt, appTime.count());

			printf("\rDrawing. Framerate = %.0f,\t Last Frame: %.4f,\t Time: %.1f", 1.0f / CalcAvgFramerate(framerateBuffer), delta.count(), appTime.count());
		}
	}
	puts("");
	return 0;
}

void Start()
{
	emitter.Init(renderer);

	particleShader.Init(renderer);
}

void Update(float dt, float t)
{

	float cameraDistance = 5.0f;

	XMFLOAT3 eyePos = XMFLOAT3(sin(t) * cameraDistance, 1.6f, cos(t) * cameraDistance);
	XMFLOAT3 target = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&target), XMLoadFloat3(&up));
	XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.01f, 100.0f);

	XMMATRIX vp = view * projection;

	CBuffer::Application appBuffer = { XMMatrixTranspose(vp) };
	particleShader.UploadApplicationCBuffer(renderer, appBuffer);

	CBuffer::PerFrame perFrame;
	perFrame.time = XMFLOAT4(t, dt, 0.0f, 1.0f);
	XMStoreFloat4(&perFrame.color, DirectX::Colors::CornflowerBlue);

	particleShader.UploadPerFrameCBuffer(renderer, perFrame);
}

void Draw(float dt, float t)
{
	renderer.ClearDepthStencil(1.0f, 0);
	renderer.ClearRenderTarget(DirectX::Colors::Black);

	renderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set State objects, Shaders and Input Layout
	particleShader.ApplyPass(renderer);

	emitter.Draw(renderer);

	renderer.SwapBuffers();
}

float CalcAvgFramerate(queue<float> & buffer)
{
	// pop until <= MAX_FRAMERATE_TIMES
	while (buffer.size() > MAX_FRAMERATE_TIMES)
		buffer.pop();
	float average = 0;
	for (size_t i = 0; i < buffer.size(); i++)
	{
		average += buffer._Get_container()[i];
	}
	return average / buffer.size();
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	msg = { window, message, wparam, lparam };
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			DestroyWindow(window);
			return 0;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		isRunning = false;
		return 0;
	}
	return DefWindowProc(window, message, wparam, lparam);
}