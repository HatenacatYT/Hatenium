#include "rendering/Renderer.h"

namespace hat {

bool Renderer::Init(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                               D3D11_SDK_VERSION, &sd, &swap_, &device_, &got, &ctx_);
    if (hr == DXGI_ERROR_UNSUPPORTED)   // no GPU / driver: fall back to the software rasterizer
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                           D3D11_SDK_VERSION, &sd, &swap_, &device_, &got, &ctx_);
    if (FAILED(hr)) return false;

    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(swap_->GetParent(IID_PPV_ARGS(&factory))) && factory) {
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        factory->Release();
    }
    CreateTarget();
    return rtv_ != nullptr;
}

void Renderer::CreateTarget() {
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(swap_->GetBuffer(0, IID_PPV_ARGS(&back))) && back) {
        device_->CreateRenderTargetView(back, nullptr, &rtv_);
        back->Release();
    }
}

void Renderer::ReleaseTarget() {
    if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
}

void Renderer::Resize(UINT w, UINT h) {
    if (!swap_ || w == 0 || h == 0) return;
    ReleaseTarget();
    swap_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    CreateTarget();
}

void Renderer::BeginFrame(const float rgba[4]) {
    if (!rtv_) return;
    ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
    ctx_->ClearRenderTargetView(rtv_, rgba);
}

bool Renderer::Present(bool vsync) {
    HRESULT hr = swap_->Present(vsync ? 1 : 0, 0);
    return hr != DXGI_STATUS_OCCLUDED;
}

ID3D11ShaderResourceView* Renderer::CreateTextureRGBA(const unsigned char* rgba, int w, int h) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = static_cast<UINT>(w); td.Height = static_cast<UINT>(h);
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = rgba; sd.SysMemPitch = static_cast<UINT>(w * 4);
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    if (SUCCEEDED(device_->CreateTexture2D(&td, &sd, &tex)) && tex) {
        device_->CreateShaderResourceView(tex, nullptr, &srv);
        tex->Release();
    }
    return srv;
}

void Renderer::Shutdown() {
    ReleaseTarget();
    if (swap_)   { swap_->Release();   swap_ = nullptr; }
    if (ctx_)    { ctx_->Release();    ctx_ = nullptr; }
    if (device_) { device_->Release(); device_ = nullptr; }
}

} // namespace hat
