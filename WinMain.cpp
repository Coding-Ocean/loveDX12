#include"WinMain.h"

INT WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ INT)
{
	//�E�B���h�E
	{
		//�E�B���h�E�N���X�o�^
		WNDCLASSEX windowClass = {};
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WndProc;
		windowClass.hInstance = GetModuleHandle(0);
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		windowClass.lpszClassName = L"GAME_WINDOW";
		RegisterClassEx(&windowClass);
		//�\���ʒu�A�E�B���h�E�̑傫������
		RECT windowRect = { 0, 0, ClientWidth, ClientHeight };
		AdjustWindowRect(&windowRect, WindowStyle, FALSE);
		int windowPosX = ClientPosX + windowRect.left;
		int windowPosY = ClientPosY + windowRect.top;
		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;
		//�E�B���h�E������
		HWnd = CreateWindowEx(
			NULL,
			L"GAME_WINDOW",
			WindowTitle,
			WindowStyle,
			windowPosX,
			windowPosY,
			windowWidth,
			windowHeight,
			NULL,		//�e�E�B���h�E�Ȃ�
			NULL,		//���j���[�Ȃ�
			GetModuleHandle(0),
			NULL);		//�����E�B���h�E�Ȃ�
		//�E�B���h�E�\��
		ShowWindow(HWnd, SW_SHOW);
	}
	//�f�o�C�X
	{
#ifdef _DEBUG
		//�f�o�b�O���[�h�ł́A�f�o�b�O���C���[��L��������
		ID3D12Debug* debug;
		Hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		assert(SUCCEEDED(Hr));
		debug->EnableDebugLayer();
		debug->Release();
#endif
		//�f�o�C�X������(�ȈՃo�[�W����)
		{
			Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
			assert(SUCCEEDED(Hr));
		}
		//�R�}���h
		{
			//�R�}���h�A���P�[�^������
			Hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&CommandAllocator));
			assert(SUCCEEDED(Hr));

			//�R�}���h���X�g������
			Hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
				CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
			assert(SUCCEEDED(Hr));

			//�R�}���h�L���[������
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		//GPU�^�C���A�E�g���L��
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;		//���ڃR�}���h�L���[
			Hr = Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&CommandQueue));
			assert(SUCCEEDED(Hr));
		}
		//�t�F���X
		{
			//GPU�̏����������`�F�b�N����t�F���X������
			Hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
			assert(SUCCEEDED(Hr));
			FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
			assert(FenceEvent != nullptr);
			FenceValue = 1;
		}
	}
	//���\�[�X		
	{
		//�����_�[�^�[�Q�b�g
		{
			//�X���b�v�`�F�C��������(�����Ƀo�b�N�o�b�t�@���܂܂�Ă���)
			{
				//DXGI�t�@�N�g��������
				IDXGIFactory4* dxgiFactory;
				Hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
				assert(SUCCEEDED(Hr));

				//�X���b�v�`�F�C��������
				DXGI_SWAP_CHAIN_DESC1 desc = {};
				desc.BufferCount = 2; //�o�b�N�o�b�t�@2��
				desc.Width = ClientWidth;
				desc.Height = ClientHeight;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				desc.SampleDesc.Count = 1;
				IDXGISwapChain1* swapChain1;
				Hr = dxgiFactory->CreateSwapChainForHwnd(
					CommandQueue, HWnd, &desc, nullptr, nullptr, &swapChain1);
				assert(SUCCEEDED(Hr));

				//IDXGISwapChain4�C���^�[�t�F�C�X���T�|�[�g���Ă��邩�q�˂�
				Hr = swapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain));
				assert(SUCCEEDED(Hr));

				swapChain1->Release();
				dxgiFactory->Release();
			}
			//�o�b�N�o�b�t�@�u�r���[�v�̓��ꕨ�ł���u�f�B�X�N���v�^�q�[�v�v������
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 2;//�o�b�N�o�b�t�@�r���[�Q��
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//RenderTargetView
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//�V�F�[�_����A�N�Z�X���Ȃ��̂�NONE��OK
				Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&BbvHeap));
				assert(SUCCEEDED(Hr));
			}
			//�o�b�N�o�b�t�@�u�r���[�v���u�f�B�X�N���v�^�q�[�v�v�ɂ���
			{
				D3D12_CPU_DESCRIPTOR_HANDLE hBbvHeap
					= BbvHeap->GetCPUDescriptorHandleForHeapStart();

				BbvHeapSize
					= Device->GetDescriptorHandleIncrementSize(
						D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

				for (UINT idx = 0; idx < 2; idx++) {
					//�o�b�N�o�b�t�@�����o��
					Hr = SwapChain->GetBuffer(idx, IID_PPV_ARGS(&BackBufs[idx]));
					assert(SUCCEEDED(Hr));
					//�o�b�N�o�b�t�@�̃r���[���q�[�v�ɂ���
					hBbvHeap.ptr += idx * BbvHeapSize;
					Device->CreateRenderTargetView(BackBufs[idx], nullptr, hBbvHeap);
				}
			}
			//�f�v�X�X�e���V���o�b�t�@������
			{
				D3D12_HEAP_PROPERTIES prop = {};
				prop.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULT��������UNKNOWN�ł悵
				prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				D3D12_RESOURCE_DESC desc = {};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2�����̃e�N�X�`���f�[�^�Ƃ���
				desc.Width = ClientWidth;//���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���
				desc.Height = ClientHeight;//��ɓ���
				desc.DepthOrArraySize = 1;//�e�N�X�`���z��ł��Ȃ���3D�e�N�X�`���ł��Ȃ�
				desc.Format = DXGI_FORMAT_D32_FLOAT;//�[�x�l�������ݗp�t�H�[�}�b�g
				desc.SampleDesc.Count = 1;//�T���v����1�s�N�Z��������1��
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//���̃o�b�t�@�͐[�x�X�e���V���Ƃ��Ďg�p���܂�
				desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				desc.MipLevels = 1;
				//�f�v�X�X�e���V���o�b�t�@���N���A����l
				D3D12_CLEAR_VALUE depthClearValue = {};
				depthClearValue.DepthStencil.Depth = 1.0f;//�[���P(�ő�l)�ŃN���A
				depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32bit�[�x�l�Ƃ��ăN���A
				//�f�v�X�X�e���V���o�b�t�@�����
				Hr = Device->CreateCommittedResource(
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, //�f�v�X�������݂Ɏg�p
					&depthClearValue,
					IID_PPV_ARGS(&DepthStencilBuf));
				assert(SUCCEEDED(Hr));
			}
			//�f�v�X�X�e���V���o�b�t�@�u�r���[�v�̓��ꕨ�ł���u�f�X�N���v�^�q�[�v�v������
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};//�[�x�Ɏg����Ƃ��������킩��΂���
				desc.NumDescriptors = 1;//�[�x�r���[1�̂�
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//�f�v�X�X�e���V���r���[�Ƃ��Ďg��
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DsvHeap));
				assert(SUCCEEDED(Hr));
			}
			//�f�v�X�X�e���V���o�b�t�@�u�r���[�v���u�f�B�X�N���v�^�q�[�v�v�ɂ���
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_D32_FLOAT;//�f�v�X�l��32bit�g�p
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
				desc.Flags = D3D12_DSV_FLAG_NONE;//�t���O�͓��ɂȂ�
				D3D12_CPU_DESCRIPTOR_HANDLE hDsvHeap
					= DsvHeap->GetCPUDescriptorHandleForHeapStart();
				Device->CreateDepthStencilView(DepthStencilBuf, &desc, hDsvHeap);
			}
		}


		//�RD���f���̃e�L�X�g�f�[�^���J��
		std::ifstream file("assets\\plane\\plane.txt");
		assert(!file.fail());

		std::string dataType;
		int numVertices = 0;
		int numElements = 0;
		UINT size = 0;

		//���_�o�b�t�@�A�ʒu�B
		{
			//�ʒu�̐��f�[�^
			file >> dataType;
			assert(dataType == "positions");
			file >> numVertices;//���_��
			numElements = numVertices * 3;//�v�f���G
			size = sizeof(float) * numElements;//�S�o�C�g��
			std::vector<float>positions(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> positions[i];
			}

			NumVertices = numVertices;//�C���f�b�N�X���g�p���Ȃ��`��̎��ɁA������g�p����̂Ŏ���Ă���

			//�ʒu�̃o�b�t�@������
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//�S�o�C�g��
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			Hr = Device->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&PositionBuf));
			assert(SUCCEEDED(Hr));

			//�ʒu�o�b�t�@�ɐ��f�[�^���R�s�[
			float* mappedBuf;
			Hr = PositionBuf->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuf));
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, positions.data(), size);
			//std::copy(positions.begin(), positions.end(), mappedBuf);
			PositionBuf->Unmap(0, nullptr);

			//�ʒu�o�b�t�@�̃r���[�����������Ă����B�i�f�B�X�N���v�^�q�[�v�ɍ��Ȃ��Ă悢�j
			PositionBufView.BufferLocation = PositionBuf->GetGPUVirtualAddress();
			PositionBufView.SizeInBytes = size;
			PositionBufView.StrideInBytes = sizeof(float) * 3;//�P���_�̃o�C�g��
		}
		//���_�o�b�t�@�A�e�N�X�`�����W�B
		{
			//�e�N�X�`�����W�̐��f�[�^
			file >> dataType;
			assert(dataType == "texcoords");
			file >> numVertices;//���_��
			numElements = numVertices * 2;//�v�f���G
			size = sizeof(float) * numElements;//�S�o�C�g��
			std::vector<float> texcoords(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> texcoords[i];
			}

			//�e�N�X�`�����W�o�b�t�@������
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//�S�o�C�g��
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			Hr = Device->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&TexcoordBuf));
			assert(SUCCEEDED(Hr));

			//�e�N�X�`�����W�o�b�t�@�ɐ��f�[�^���R�s�[
			float* mappedBuf;
			Hr = TexcoordBuf->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuf));
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, texcoords.data(), size);
			//std::copy(texcoords.begin(), texcoords.end(), mappedBuf);
			TexcoordBuf->Unmap(0, nullptr);

			//�e�N�X�`�����W�o�b�t�@�̃r���[�����������Ă����B�i�f�B�X�N���v�^�q�[�v�ɍ��Ȃ��Ă悢�j
			TexcoordBufView.BufferLocation = TexcoordBuf->GetGPUVirtualAddress();
			TexcoordBufView.SizeInBytes = size;//�S�o�C�g��
			TexcoordBufView.StrideInBytes = sizeof(float) * 2;//1���_�̃o�C�g��
		}
		//���_�C���f�b�N�X�o�b�t�@
		{
			//�C���f�b�N�X�̐��f�[�^
			file >> dataType;
			assert(dataType == "indices");
			file >> numElements;//�C���f�b�N�X�͂��ꂪ�v�f���G
			size = sizeof(UINT16) * numElements;//�S�o�C�g��
			std::vector<UINT16> indices(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> indices[i];
			}

			NumIndices = numElements;//�C���f�b�N�X���g�p����`��̎��Ɏg�p����̂Ŏ���Ă���

			//�C���f�b�N�X�o�b�t�@������
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//�S�o�C�g��
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			Hr = Device->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&IndexBuf));
			assert(SUCCEEDED(Hr));

			//������o�b�t�@�Ƀf�[�^���R�s�[
			UINT16* mappedBuf = nullptr;
			Hr = IndexBuf->Map(0, nullptr, (void**)&mappedBuf);
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, indices.data(), size);
			//std::copy(indices.begin(), indices.end(), mappedBuf);
			IndexBuf->Unmap(0, nullptr);

			//�C���f�b�N�X�o�b�t�@�r���[������
			IndexBufView.BufferLocation = IndexBuf->GetGPUVirtualAddress();
			IndexBufView.SizeInBytes = size;
			IndexBufView.Format = DXGI_FORMAT_R16_UINT;
		}
		//�R���X�^���g�o�b�t�@�ƃe�N�X�`���o�b�t�@ �� �f�B�X�N���v�^�q�[�v
		{
			//�R���X�^���g�o�b�t�@�O������
			{
				D3D12_HEAP_PROPERTIES prop = {};
				prop.Type = D3D12_HEAP_TYPE_UPLOAD;
				prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				prop.CreationNodeMask = 1;
				prop.VisibleNodeMask = 1;
				D3D12_RESOURCE_DESC desc = {};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Alignment = 0;
				desc.Width = (sizeof(CONST_BUF0) + 255) & ~255;//�T�C�Y��256�̔{���ɂ���
				desc.Height = 1;
				desc.DepthOrArraySize = 1;
				desc.MipLevels = 1;
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.SampleDesc = { 1, 0 };
				desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				desc.Flags = D3D12_RESOURCE_FLAG_NONE;
				Hr = Device->CreateCommittedResource(
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&ConstBuf0));
				assert(SUCCEEDED(Hr));

				//�}�b�v���Ă����āA���C�����[�v���ōX�V����B
				Hr = ConstBuf0->Map(0, nullptr, (void**)&MapConstBuf0);
				assert(SUCCEEDED(Hr));
			}
			//�R���X�^���g�o�b�t�@�P������
			{
				D3D12_HEAP_PROPERTIES prop = {};
				prop.Type = D3D12_HEAP_TYPE_UPLOAD;
				prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				prop.CreationNodeMask = 1;
				prop.VisibleNodeMask = 1;
				D3D12_RESOURCE_DESC desc = {};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Alignment = 0;
				desc.Width = (sizeof(CONST_BUF1) + 255) & ~255;//�T�C�Y��256�̔{���ɂ���
				desc.Height = 1;
				desc.DepthOrArraySize = 1;
				desc.MipLevels = 1;
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.SampleDesc = { 1, 0 };
				desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				desc.Flags = D3D12_RESOURCE_FLAG_NONE;
				Hr = Device->CreateCommittedResource(
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&ConstBuf1));
				assert(SUCCEEDED(Hr));

				//�}�b�v���Ă����āA���C�����[�v���ōX�V����B
				Hr = ConstBuf1->Map(0, nullptr, (void**)&MapConstBuf1);
				assert(SUCCEEDED(Hr));
			}
			//�e�N�X�`���o�b�t�@������(Microsoft���� Direct Memory Access�o�[�W����)
			{
				//�t�@�C������ǂݍ���
				file >> dataType;
				assert(dataType == "texture");
				std::string filename;
				file >> filename;

				//�t�@�C����ǂݍ��݁A���f�[�^�����o��
				unsigned char* pixels = nullptr;
				int width = 0, height = 0, bytePerPixel = 4;
				pixels = stbi_load(filename.c_str(), &width, &height, nullptr, bytePerPixel);
				assert(pixels != nullptr);

				//�P�s�̃s�b�`��256�̔{���ɂ��Ă���(�o�b�t�@�T�C�Y��256�̔{���łȂ���΂����Ȃ�)
				const UINT64 alignedRowPitch = (width * bytePerPixel + 0xff) & ~0xff;

				//�A�b�v���[�h�p���ԃo�b�t�@������A���f�[�^���R�s�[���Ă���
				ID3D12Resource* uploadBuf;
				{
					//�e�N�X�`���ł͂Ȃ��t�c�[�̃o�b�t�@�Ƃ��Ă���
					D3D12_HEAP_PROPERTIES prop = {};
					prop.Type = D3D12_HEAP_TYPE_UPLOAD;
					prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					prop.CreationNodeMask = 0;
					prop.VisibleNodeMask = 0;
					D3D12_RESOURCE_DESC desc = {};
					desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					desc.Format = DXGI_FORMAT_UNKNOWN;
					desc.Width = alignedRowPitch * height;
					desc.Height = 1;
					desc.DepthOrArraySize = 1;
					desc.MipLevels = 1;
					desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//�A�������f�[�^�ł���
					desc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�
					desc.SampleDesc.Count = 1;//�ʏ�e�N�X�`���Ȃ̂ŃA���`�F�����Ȃ�
					desc.SampleDesc.Quality = 0;
					Hr = Device->CreateCommittedResource(
						&prop,
						D3D12_HEAP_FLAG_NONE,
						&desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&uploadBuf));
					assert(SUCCEEDED(Hr));

					//���f�[�^��uploadbuff�Ɉ�U�R�s�[���܂�
					uint8_t* mapBuf = nullptr;
					Hr = uploadBuf->Map(0, nullptr, (void**)&mapBuf);//�}�b�v
					auto srcAddress = pixels;
					auto originalRowPitch = width * bytePerPixel;
					for (int y = 0; y < height; ++y) {
						memcpy(mapBuf, srcAddress, originalRowPitch);
						//1�s���Ƃ̒�������킹�Ă��
						srcAddress += originalRowPitch;
						mapBuf += alignedRowPitch;
					}
					uploadBuf->Unmap(0, nullptr);//�A���}�b�v
				}

				//�����āA�ŏI�R�s�[��ł���e�N�X�`���o�b�t�@�����
				{
					D3D12_HEAP_PROPERTIES prop = {};
					prop.Type = D3D12_HEAP_TYPE_DEFAULT;
					prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					prop.CreationNodeMask = 0;
					prop.VisibleNodeMask = 0;
					D3D12_RESOURCE_DESC desc = {};
					desc.MipLevels = 1;
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.Width = width;
					desc.Height = height;
					desc.Flags = D3D12_RESOURCE_FLAG_NONE;
					desc.DepthOrArraySize = 1;
					desc.SampleDesc.Count = 1;
					desc.SampleDesc.Quality = 0;
					desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					Hr = Device->CreateCommittedResource(
						&prop,
						D3D12_HEAP_FLAG_NONE,
						&desc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						IID_PPV_ARGS(&TextureBuf));
					assert(SUCCEEDED(Hr));
				}

				//uploadBuf����textureBuf�փR�s�[���钷�����̂肪�n�܂�܂�

				//�܂��R�s�[�����P�[�V�����̏����E�t�b�g�v�����g�w��
				D3D12_TEXTURE_COPY_LOCATION src = {};
				src.pResource = uploadBuf;
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				src.PlacedFootprint.Footprint.Width = static_cast<UINT>(width);
				src.PlacedFootprint.Footprint.Height = static_cast<UINT>(height);
				src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(1);
				src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(alignedRowPitch);
				src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				//�R�s�[�惍�P�[�V�����̏����E�T�u���\�[�X�C���f�b�N�X�w��
				D3D12_TEXTURE_COPY_LOCATION dst = {};
				dst.pResource = TextureBuf;
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = 0;

				//�R�}���h���X�g�ŃR�s�[��\�񂵂܂���I�I�I
				CommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
				//���Ă��Ƃ̓o���A������̂ł�
				D3D12_RESOURCE_BARRIER barrier = {};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = TextureBuf;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				CommandList->ResourceBarrier(1, &barrier);
				//uploadBuf�A�����[�h
				CommandList->DiscardResource(uploadBuf, nullptr);
				//�R�}���h���X�g�����
				CommandList->Close();
				//���s
				ID3D12CommandList* commandLists[] = { CommandList };
				CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
				//���\�[�X��GPU�ɓ]�������܂őҋ@����
				WaitDrawDone();

				//�R�}���h�A���P�[�^�����Z�b�g
				HRESULT Hr = CommandAllocator->Reset();
				assert(SUCCEEDED(Hr));
				//�R�}���h���X�g�����Z�b�g
				Hr = CommandList->Reset(CommandAllocator, nullptr);
				assert(SUCCEEDED(Hr));

				//�J��
				uploadBuf->Release();
				stbi_image_free(pixels);
			}{}
			//�f�B�X�N���v�^�q�[�v������A�����Ƀr���[������
			{
				//�u�r���[�v�̓��ꕨ�ł���u�f�B�X�N���v�^�q�[�v�v������
				{
					D3D12_DESCRIPTOR_HEAP_DESC desc = {};
					desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
					desc.NumDescriptors = 3;//�R���X�^���g�o�b�t�@�Q�ƃe�N�X�`���o�b�t�@�P��
					desc.NodeMask = 0;
					desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
					Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&CbvTbvHeap));
					assert(SUCCEEDED(Hr));
				}

				auto hCbvTbvHeap = CbvTbvHeap->GetCPUDescriptorHandleForHeapStart();
				auto heapSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				//�R���X�^���g�o�b�t�@�O�́u�r���[�v���u�f�B�X�N���v�^�q�[�v�v�ɂ���
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					desc.BufferLocation = ConstBuf0->GetGPUVirtualAddress();
					desc.SizeInBytes = static_cast<UINT>(ConstBuf0->GetDesc().Width);
					Device->CreateConstantBufferView(&desc, hCbvTbvHeap);
				}

				hCbvTbvHeap.ptr += heapSize;

				//�R���X�^���g�o�b�t�@�P�́u�r���[�v���u�f�B�X�N���v�^�q�[�v�v�ɂ���
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					desc.BufferLocation = ConstBuf1->GetGPUVirtualAddress();
					desc.SizeInBytes = static_cast<UINT>(ConstBuf1->GetDesc().Width);
					Device->CreateConstantBufferView(&desc, hCbvTbvHeap);
				}

				hCbvTbvHeap.ptr += heapSize;

				//�e�N�X�`���o�b�t�@�́u�r���[�v���u�f�B�X�N���v�^�q�[�v�v�ɂ���
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
					desc.Format = TextureBuf->GetDesc().Format;
					desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
					desc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1
					Device->CreateShaderResourceView(TextureBuf, &desc, hCbvTbvHeap);
				}
			}
		}

	}{}
	//�p�C�v���C��
	{
		//���[�g�V�O�l�`��
		{
			//�f�B�X�N���v�^�����W�B�f�B�X�N���v�^�q�[�v�ƃV�F�[�_��R�Â�����������B
			D3D12_DESCRIPTOR_RANGE  range[3] = {};
			UINT b0 = 0;
			range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			range[0].BaseShaderRegister = b0;
			range[0].NumDescriptors = 1;
			range[0].RegisterSpace = 0;
			range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			UINT b1 = 1;
			range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			range[1].BaseShaderRegister = b1;
			range[1].NumDescriptors = 1;
			range[1].RegisterSpace = 0;
			range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			UINT t0 = 0;
			range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range[2].BaseShaderRegister = t0;
			range[2].NumDescriptors = 1;
			range[2].RegisterSpace = 0;
			range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			//���[�g�p�����^���f�B�X�N���v�^�e�[�u���Ƃ��Ďg�p
			D3D12_ROOT_PARAMETER rootParam[1] = {};
			rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParam[0].DescriptorTable.pDescriptorRanges = range;
			rootParam[0].DescriptorTable.NumDescriptorRanges = _countof(range);
			rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			//�T���v���̋L�q�B���̃T���v�����V�F�[�_�[�� s0 �ɃZ�b�g�����
			D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
			samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//��Ԃ��Ȃ�(�j�A���X�g�l�C�o�[)
			samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
			samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
			samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
			samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�{�[�_�[�̎��͍�
			samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
			samplerDesc[0].MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
			samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//�I�[�o�[�T���v�����O�̍ۃ��T���v�����O���Ȃ��H
			samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����̂݉�

			//���[�g�V�O�j�`���̋L�q
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			desc.pParameters = rootParam;
			desc.NumParameters = _countof(rootParam);
			desc.pStaticSamplers = samplerDesc;//�T���v���[�̐擪�A�h���X
			desc.NumStaticSamplers = _countof(samplerDesc);//�T���v���[��

			//���[�g�V�O�l�`�����V���A���C�Y��blob(��)������B
			ID3DBlob* blob;
			Hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
			assert(SUCCEEDED(Hr));

			//���[�g�V�O�l�`��������
			Hr = Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
				IID_PPV_ARGS(&RootSignature));
			assert(SUCCEEDED(Hr));
			blob->Release();
		}

		//�V�F�[�_�ǂݍ���
		BIN_FILE12 vs("assets\\cso\\VertexShader.cso");
		assert(vs.succeeded());
		BIN_FILE12 ps("assets\\cso\\PixelShader.cso");
		assert(ps.succeeded());

		//�ȉ��A�e��L�q

		UINT slot0 = 0, slot1 = 1;
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, slot0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, slot1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC rasterDesc = {};
		rasterDesc.FrontCounterClockwise = true;
		rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.ForcedSampleCount = 0;
		rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = true;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].LogicOpEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
		depthStencilDesc.StencilEnable = false;//�X�e���V���o�b�t�@�͎g��Ȃ�


		//�����܂ł̋L�q���܂Ƃ߂ăp�C�v���C���X�e�[�g�I�u�W�F�N�g������
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = RootSignature;
		pipelineDesc.VS = { vs.code(), vs.size() };
		pipelineDesc.PS = { ps.code(), ps.size() };
		pipelineDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		pipelineDesc.RasterizerState = rasterDesc;
		pipelineDesc.BlendState = blendDesc;
		pipelineDesc.DepthStencilState = depthStencilDesc;
		pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineDesc.SampleMask = UINT_MAX;
		pipelineDesc.SampleDesc.Count = 1;
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		Hr = Device->CreateGraphicsPipelineState(
			&pipelineDesc,
			IID_PPV_ARGS(&PipelineState)
		);
		assert(SUCCEEDED(Hr));

		//�o�͗̈��ݒ�
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = ClientWidth;
		Viewport.Height = ClientHeight;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;

		//�؂����`��ݒ�
		ScissorRect.left = 0;
		ScissorRect.top = 0;
		ScissorRect.right = ClientWidth;
		ScissorRect.bottom = ClientHeight;
	}
	//���C�����[�v
	while (true)
	{
		//�E�B���h�E���b�Z�[�W�̎擾�A���o
		{
			MSG msg = { 0 };
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				continue;
			}
		}
		//�R���X�^���g�o�b�t�@�X�V
		{
			//��]�p���W�A��
			static float radian = 0;
			float radius = 0.4f;
			radian += 0.01f;

			//���[���h�}�g���b�N�X
			XMMATRIX world = XMMatrixRotationY(radian);
			//�r���[�}�g���b�N�X
			XMVECTOR eye = { 0, 3, -10 }, focus = { 0, 3, 0 }, up = { 0, 1, 0 };
			XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
			//�v���W�F�N�V�����}�g���b�N�X
			XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, Aspect, 1.0f, 21.0f);

			//�R���X�^���g�o�b�t�@�O�X�V
			MapConstBuf0->mat = world * view * proj;
			//�R���X�^���g�o�b�t�@�P�X�V
			MapConstBuf1->diffuse = { 1.0f,1.0f,1.0f,1.0f };
		}
		//�o�b�N�o�b�t�@���N���A
		{
			//���݂̃o�b�N�o�b�t�@�̃C���f�b�N�X���擾�B���̃v���O�����̏ꍇ0 or 1�ɂȂ�B
			BackBufIdx = SwapChain->GetCurrentBackBufferIndex();

			//�o���A�Ńo�b�N�o�b�t�@��`��^�[�Q�b�g�ɐ؂�ւ���
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//���̃o���A�͏�ԑJ�ڃ^�C�v
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = BackBufs[BackBufIdx];//���\�[�X�̓o�b�N�o�b�t�@
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;//�J�ڑO��Present
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;//�J�ڌ�͕`��^�[�Q�b�g
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			CommandList->ResourceBarrier(1, &barrier);

			//�o�b�N�o�b�t�@�̏ꏊ���w���f�B�X�N���v�^�q�[�v�n���h����p�ӂ���
			auto hBbvHeap = BbvHeap->GetCPUDescriptorHandleForHeapStart();
			hBbvHeap.ptr += BackBufIdx * BbvHeapSize;
			//�f�v�X�X�e���V���o�b�t�@�̃f�B�X�N���v�^�n���h����p�ӂ���
			auto hDsvHeap = DsvHeap->GetCPUDescriptorHandleForHeapStart();
			//�o�b�N�o�b�t�@�ƃf�v�X�X�e���V���o�b�t�@��`��^�[�Q�b�g�Ƃ��Đݒ肷��
			CommandList->OMSetRenderTargets(1, &hBbvHeap, false, &hDsvHeap);

			//�`��^�[�Q�b�g���N���A����
			const float clearColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };
			CommandList->ClearRenderTargetView(hBbvHeap, clearColor, 0, nullptr);

			//�f�v�X�X�e���V���o�b�t�@���N���A����
			CommandList->ClearDepthStencilView(hDsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			//�r���[�|�[�g�ƃV�U�[��`��ݒ�
			CommandList->RSSetViewports(1, &Viewport);
			CommandList->RSSetScissorRects(1, &ScissorRect);
		}
		//�o�b�N�o�b�t�@�ɕ`��
		{
			//�p�C�v���C���X�e�[�g���Z�b�g
			CommandList->SetPipelineState(PipelineState);
			//���[�g�V�O�j�`�����Z�b�g
			CommandList->SetGraphicsRootSignature(RootSignature);

			//���_���Z�b�g
			D3D12_VERTEX_BUFFER_VIEW vertexBufViews[] = { PositionBufView, TexcoordBufView, };
			CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CommandList->IASetVertexBuffers(0, _countof(vertexBufViews), vertexBufViews);

			//�f�B�X�N���v�^�q�[�v��GPU�ɃZ�b�g�B�܂��A�V�F�[�_�ƕR�Â��Ă��Ȃ��B
			CommandList->SetDescriptorHeaps(1, &CbvTbvHeap);
			//�f�B�X�N���v�^�q�[�v���f�B�X�N���v�^�e�[�u���ɃZ�b�g�B�V�F�[�_�ƕR�Â��B
			CommandList->SetGraphicsRootDescriptorTable(0, CbvTbvHeap->GetGPUDescriptorHandleForHeapStart());
#if 1
			//�`��B�C���f�b�N�X���g�p���Ȃ�
			CommandList->DrawInstanced(NumVertices, 1, 0, 0);
#else
			//�`��B�C���f�b�N�X���g�p����
			CommandList->IASetIndexBuffer(&IndexBufView);
			CommandList->DrawIndexedInstanced(NumIndices, 1, 0, 0, 0);
#endif
		}
		//�o�b�N�o�b�t�@��\��
		{
			//�o���A�Ńo�b�N�o�b�t�@��\���p�ɐ؂�ւ���
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//���̃o���A�͏�ԑJ�ڃ^�C�v
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = BackBufs[BackBufIdx];//���\�[�X�̓o�b�N�o�b�t�@
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;//�J�ڑO�͕`��^�[�Q�b�g
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;//�J�ڌ��Present
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			CommandList->ResourceBarrier(1, &barrier);

			//�R�}���h���X�g���N���[�Y����
			CommandList->Close();
			//�R�}���h���X�g�����s����
			ID3D12CommandList* commandLists[] = { CommandList };
			CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
			//�`�抮����҂�
			WaitDrawDone();

			//�o�b�N�o�b�t�@��\��
			SwapChain->Present(1, 0);

			//�R�}���h�A���P�[�^�����Z�b�g
			Hr = CommandAllocator->Reset();
			assert(SUCCEEDED(Hr));
			//�R�}���h���X�g�����Z�b�g
			Hr = CommandList->Reset(CommandAllocator, nullptr);
			assert(SUCCEEDED(Hr));
		}
	}{}
	//���
	{
		WaitDrawDone();

		PipelineState->Release();
		RootSignature->Release();

		CbvTbvHeap->Release();
		TextureBuf->Release();
		ConstBuf1->Unmap(0, nullptr);
		ConstBuf1->Release();
		ConstBuf0->Unmap(0, nullptr);
		ConstBuf0->Release();

		IndexBuf->Release();
		TexcoordBuf->Release();
		PositionBuf->Release();

		DsvHeap->Release();
		DepthStencilBuf->Release();
		BbvHeap->Release();
		BackBufs[0]->Release();
		BackBufs[1]->Release();
		SwapChain->Release();

		CloseHandle(FenceEvent);
		Fence->Release();
		CommandQueue->Release();
		CommandList->Release();
		CommandAllocator->Release();
		Device->Release();
	}
}

// �`�抮����҂�
void WaitDrawDone()
{
	//���݂�Fence�l���R�}���h�I�����Fence�ɏ������܂��悤�ɂ���
	UINT64 fvalue = FenceValue;
	CommandQueue->Signal(Fence, fvalue);
	FenceValue++;

	//�܂��R�}���h�L���[���I�����Ă��Ȃ����Ƃ��m�F����
	if (Fence->GetCompletedValue() < fvalue)
	{
		//����Fence�ɂ����āAfvalue �̒l�ɂȂ�����C�x���g�𔭐�������
		Fence->SetEventOnCompletion(fvalue, FenceEvent);
		//�C�x���g����������܂ő҂�
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
}