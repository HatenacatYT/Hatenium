#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace hat {

// Minimal Direct3D 11 renderer: device + swap chain + back-buffer target.
class Renderer {
public:
    bool Init(HWND hwnd);
    void Shutdown();
    void Resize(UINT w, UINT h);
    void BeginFrame(const float rgba[4]);
    bool Present(bool vsync);            // false when the window is occluded
    ID3D11Device* Device() const { return device_; }
    ID3D11DeviceContext* Context() const { return ctx_; }
private:
    void CreateTarget();
    void ReleaseTarget();
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* ctx_ = nullptr;
    IDXGISwapChain* swap_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
};

} // namespace hat
