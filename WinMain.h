#pragma once
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")

#include<dxgi1_6.h>
#include<d3d12.h>
#include<vector>
#include<cassert>
#include<Windows.h>
#include<DirectXMath.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace DirectX;
void WaitDrawDone();
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

//�E�B���h�E------------------------------------------------------------------------
LPCWSTR	WindowTitle = L"loveDX12";
const int ClientWidth = 1280;
const int ClientHeight = 720;
const int ClientPosX = (GetSystemMetrics(SM_CXSCREEN) - ClientWidth) / 2;//�����\��
const int ClientPosY = (GetSystemMetrics(SM_CYSCREEN) - ClientHeight) / 2;//�����\��
const float Aspect = static_cast<float>(ClientWidth) / ClientHeight;
#if 1 
DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
#else 
DWORD WindowStyle = WS_POPUP;//Alt + F4�ŕ���
#endif
HWND HWnd;

//�f�o�C�X--------------------------------------------------------------------------
ID3D12Device* Device;
//�R�}���h
ID3D12CommandAllocator* CommandAllocator;
ID3D12GraphicsCommandList* CommandList;
ID3D12CommandQueue* CommandQueue;
//�t�F���X
ID3D12Fence* Fence;
HANDLE FenceEvent;
UINT64 FenceValue;
//�f�o�b�O
HRESULT Hr;

//���\�[�X----------------------------------------------------------------------
//�o�b�N�o�b�t�@
IDXGISwapChain4* SwapChain;
ID3D12Resource* BackBufs[2];
UINT BackBufIdx;
ID3D12DescriptorHeap* BbvHeap;//"Bbv"��"BackBufView"�̗�
UINT BbvHeapSize;
//�f�v�X�X�e���V���o�b�t�@
ID3D12Resource* DepthStencilBuf;
ID3D12DescriptorHeap* DsvHeap;//"Dsv"��"DepthStencilBufView"�̗�

//���_�o�b�t�@
ID3D12Resource* PositionBuf;
D3D12_VERTEX_BUFFER_VIEW PositionBufView;
ID3D12Resource* TexcoordBuf;
D3D12_VERTEX_BUFFER_VIEW TexcoordBufView;
UINT NumVertices;
//���_�C���f�b�N�X�o�b�t�@
ID3D12Resource* IndexBuf;
D3D12_INDEX_BUFFER_VIEW	IndexBufView;
UINT NumIndices;

//�R���X�^���g�o�b�t�@�A�e�N�X�`���o�b�t�@
struct CONST_BUF0 {
    XMMATRIX mat;
};
struct CONST_BUF1 {
    XMFLOAT4 diffuse;
};
CONST_BUF0* MapConstBuf0;
CONST_BUF1* MapConstBuf1;
ID3D12Resource* ConstBuf0;
ID3D12Resource* ConstBuf1;
ID3D12Resource* TextureBuf;
ID3D12DescriptorHeap* CbvTbvHeap;//Cbv��ConstantBufferView�ATbv��TextureBefferView�̗�

//�p�C�v���C��--------------------------------------------------------------------
ID3D12RootSignature* RootSignature;
ID3D12PipelineState* PipelineState;
D3D12_VIEWPORT Viewport;
D3D12_RECT ScissorRect;

//BIN_FILE12.h�Ƃ����w�b�_��ǉ�����������������
#include<fstream>
//�R���p�C���ς݃V�F�[�_��ǂݍ��ރt�@�C���o�b�t�@
class BIN_FILE12 {
public:
    BIN_FILE12(const char* fileName) :Succeeded(false)
    {
        std::ifstream ifs(fileName, std::ios::binary);
        if (ifs.fail()) {
            return;
        }
        Succeeded = true;
        std::istreambuf_iterator<char> first(ifs);
        std::istreambuf_iterator<char> last;
        Buffer.assign(first, last);
        ifs.close();
    }
    bool succeeded() const
    {
        return Succeeded;
    }
    unsigned char* code() const
    {
        char* p = const_cast<char*>(Buffer.data());
        return reinterpret_cast<unsigned char*>(p);
    }
    size_t size() const
    {
        return Buffer.size();
    }
private:
    std::string Buffer;
    bool Succeeded;
};
