#include"WinMain.h"

INT WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ INT)
{
	//ウィンドウ
	{
		//ウィンドウクラス登録
		WNDCLASSEX windowClass = {};
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WndProc;
		windowClass.hInstance = GetModuleHandle(0);
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		windowClass.lpszClassName = L"GAME_WINDOW";
		RegisterClassEx(&windowClass);
		//表示位置、ウィンドウの大きさ調整
		RECT windowRect = { 0, 0, ClientWidth, ClientHeight };
		AdjustWindowRect(&windowRect, WindowStyle, FALSE);
		int windowPosX = ClientPosX + windowRect.left;
		int windowPosY = ClientPosY + windowRect.top;
		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;
		//ウィンドウをつくる
		HWnd = CreateWindowEx(
			NULL,
			L"GAME_WINDOW",
			WindowTitle,
			WindowStyle,
			windowPosX,
			windowPosY,
			windowWidth,
			windowHeight,
			NULL,		//親ウィンドウなし
			NULL,		//メニューなし
			GetModuleHandle(0),
			NULL);		//複数ウィンドウなし
		//ウィンドウ表示
		ShowWindow(HWnd, SW_SHOW);
	}
	//デバイス
	{
#ifdef _DEBUG
		//デバッグモードでは、デバッグレイヤーを有効化する
		ID3D12Debug* debug;
		Hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		assert(SUCCEEDED(Hr));
		debug->EnableDebugLayer();
		debug->Release();
#endif
		//デバイスをつくる(簡易バージョン)
		{
			Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
			assert(SUCCEEDED(Hr));
		}
		//コマンド
		{
			//コマンドアロケータをつくる
			Hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&CommandAllocator));
			assert(SUCCEEDED(Hr));

			//コマンドリストをつくる
			Hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
				CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
			assert(SUCCEEDED(Hr));

			//コマンドキューをつくる
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		//GPUタイムアウトが有効
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;		//直接コマンドキュー
			Hr = Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&CommandQueue));
			assert(SUCCEEDED(Hr));
		}
		//フェンス
		{
			//GPUの処理完了をチェックするフェンスをつくる
			Hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
			assert(SUCCEEDED(Hr));
			FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
			assert(FenceEvent != nullptr);
			FenceValue = 1;
		}
	}
	//リソース		
	{
		//レンダーターゲット
		{
			//スワップチェインをつくる(ここにバックバッファが含まれている)
			{
				//DXGIファクトリをつくる
				IDXGIFactory4* dxgiFactory;
				Hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
				assert(SUCCEEDED(Hr));

				//スワップチェインをつくる
				DXGI_SWAP_CHAIN_DESC1 desc = {};
				desc.BufferCount = 2; //バックバッファ2枚
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

				//IDXGISwapChain4インターフェイスをサポートしているか尋ねる
				Hr = swapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain));
				assert(SUCCEEDED(Hr));

				swapChain1->Release();
				dxgiFactory->Release();
			}
			//バックバッファ「ビュー」の入れ物である「ディスクリプタヒープ」をつくる
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 2;//バックバッファビュー２つ
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//RenderTargetView
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//シェーダからアクセスしないのでNONEでOK
				Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&BbvHeap));
				assert(SUCCEEDED(Hr));
			}
			//バックバッファ「ビュー」を「ディスクリプタヒープ」につくる
			{
				D3D12_CPU_DESCRIPTOR_HANDLE hBbvHeap
					= BbvHeap->GetCPUDescriptorHandleForHeapStart();

				BbvHeapSize
					= Device->GetDescriptorHandleIncrementSize(
						D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

				for (UINT idx = 0; idx < 2; idx++) {
					//バックバッファを取り出す
					Hr = SwapChain->GetBuffer(idx, IID_PPV_ARGS(&BackBufs[idx]));
					assert(SUCCEEDED(Hr));
					//バックバッファのビューをヒープにつくる
					hBbvHeap.ptr += idx * BbvHeapSize;
					Device->CreateRenderTargetView(BackBufs[idx], nullptr, hBbvHeap);
				}
			}
			//デプスステンシルバッファをつくる
			{
				D3D12_HEAP_PROPERTIES prop = {};
				prop.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULTだから後はUNKNOWNでよし
				prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				D3D12_RESOURCE_DESC desc = {};
				desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元のテクスチャデータとして
				desc.Width = ClientWidth;//幅と高さはレンダーターゲットと同じ
				desc.Height = ClientHeight;//上に同じ
				desc.DepthOrArraySize = 1;//テクスチャ配列でもないし3Dテクスチャでもない
				desc.Format = DXGI_FORMAT_D32_FLOAT;//深度値書き込み用フォーマット
				desc.SampleDesc.Count = 1;//サンプルは1ピクセル当たり1つ
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//このバッファは深度ステンシルとして使用します
				desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				desc.MipLevels = 1;
				//デプスステンシルバッファをクリアする値
				D3D12_CLEAR_VALUE depthClearValue = {};
				depthClearValue.DepthStencil.Depth = 1.0f;//深さ１(最大値)でクリア
				depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32bit深度値としてクリア
				//デプスステンシルバッファを作る
				Hr = Device->CreateCommittedResource(
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
					&depthClearValue,
					IID_PPV_ARGS(&DepthStencilBuf));
				assert(SUCCEEDED(Hr));
			}
			//デプスステンシルバッファ「ビュー」の入れ物である「デスクリプタヒープ」をつくる
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};//深度に使うよという事がわかればいい
				desc.NumDescriptors = 1;//深度ビュー1つのみ
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DsvHeap));
				assert(SUCCEEDED(Hr));
			}
			//デプスステンシルバッファ「ビュー」を「ディスクリプタヒープ」につくる
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_D32_FLOAT;//デプス値に32bit使用
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
				desc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし
				D3D12_CPU_DESCRIPTOR_HANDLE hDsvHeap
					= DsvHeap->GetCPUDescriptorHandleForHeapStart();
				Device->CreateDepthStencilView(DepthStencilBuf, &desc, hDsvHeap);
			}
		}


		//３Dモデルのテキストデータを開く
		std::ifstream file("assets\\plane\\plane.txt");
		assert(!file.fail());

		std::string dataType;
		int numVertices = 0;
		int numElements = 0;
		UINT size = 0;

		//頂点バッファ、位置。
		{
			//位置の生データ
			file >> dataType;
			assert(dataType == "positions");
			file >> numVertices;//頂点数
			numElements = numVertices * 3;//要素数；
			size = sizeof(float) * numElements;//全バイト数
			std::vector<float>positions(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> positions[i];
			}

			NumVertices = numVertices;//インデックスを使用しない描画の時に、これを使用するので取っておく

			//位置のバッファをつくる
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//全バイト数
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

			//位置バッファに生データをコピー
			float* mappedBuf;
			Hr = PositionBuf->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuf));
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, positions.data(), size);
			//std::copy(positions.begin(), positions.end(), mappedBuf);
			PositionBuf->Unmap(0, nullptr);

			//位置バッファのビューを初期化しておく。（ディスクリプタヒープに作らなくてよい）
			PositionBufView.BufferLocation = PositionBuf->GetGPUVirtualAddress();
			PositionBufView.SizeInBytes = size;
			PositionBufView.StrideInBytes = sizeof(float) * 3;//１頂点のバイト数
		}
		//頂点バッファ、テクスチャ座標。
		{
			//テクスチャ座標の生データ
			file >> dataType;
			assert(dataType == "texcoords");
			file >> numVertices;//頂点数
			numElements = numVertices * 2;//要素数；
			size = sizeof(float) * numElements;//全バイト数
			std::vector<float> texcoords(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> texcoords[i];
			}

			//テクスチャ座標バッファをつくる
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//全バイト数
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

			//テクスチャ座標バッファに生データをコピー
			float* mappedBuf;
			Hr = TexcoordBuf->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuf));
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, texcoords.data(), size);
			//std::copy(texcoords.begin(), texcoords.end(), mappedBuf);
			TexcoordBuf->Unmap(0, nullptr);

			//テクスチャ座標バッファのビューを初期化しておく。（ディスクリプタヒープに作らなくてよい）
			TexcoordBufView.BufferLocation = TexcoordBuf->GetGPUVirtualAddress();
			TexcoordBufView.SizeInBytes = size;//全バイト数
			TexcoordBufView.StrideInBytes = sizeof(float) * 2;//1頂点のバイト数
		}
		//頂点インデックスバッファ
		{
			//インデックスの生データ
			file >> dataType;
			assert(dataType == "indices");
			file >> numElements;//インデックスはこれが要素数；
			size = sizeof(UINT16) * numElements;//全バイト数
			std::vector<UINT16> indices(numElements);
			for (int i = 0; i < numElements; i++) {
				file >> indices[i];
			}

			NumIndices = numElements;//インデックスを使用する描画の時に使用するので取っておく

			//インデックスバッファをつくる
			D3D12_HEAP_PROPERTIES prop = {};
			prop.Type = D3D12_HEAP_TYPE_UPLOAD;
			prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask = 1;
			prop.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Alignment = 0;
			desc.Width = size;//全バイト数
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

			//作ったバッファにデータをコピー
			UINT16* mappedBuf = nullptr;
			Hr = IndexBuf->Map(0, nullptr, (void**)&mappedBuf);
			assert(SUCCEEDED(Hr));
			memcpy(mappedBuf, indices.data(), size);
			//std::copy(indices.begin(), indices.end(), mappedBuf);
			IndexBuf->Unmap(0, nullptr);

			//インデックスバッファビューをつくる
			IndexBufView.BufferLocation = IndexBuf->GetGPUVirtualAddress();
			IndexBufView.SizeInBytes = size;
			IndexBufView.Format = DXGI_FORMAT_R16_UINT;
		}
		//コンスタントバッファとテクスチャバッファ ⇒ ディスクリプタヒープ
		{
			//コンスタントバッファ０をつくる
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
				desc.Width = (sizeof(CONST_BUF0) + 255) & ~255;//サイズを256の倍数にする
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

				//マップしておいて、メインループ中で更新する。
				Hr = ConstBuf0->Map(0, nullptr, (void**)&MapConstBuf0);
				assert(SUCCEEDED(Hr));
			}
			//コンスタントバッファ１をつくる
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
				desc.Width = (sizeof(CONST_BUF1) + 255) & ~255;//サイズを256の倍数にする
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

				//マップしておいて、メインループ中で更新する。
				Hr = ConstBuf1->Map(0, nullptr, (void**)&MapConstBuf1);
				assert(SUCCEEDED(Hr));
			}
			//テクスチャバッファをつくる(Microsoft推奨 Direct Memory Accessバージョン)
			{
				//ファイル名を読み込む
				file >> dataType;
				assert(dataType == "texture");
				std::string filename;
				file >> filename;

				//ファイルを読み込み、生データを取り出す
				unsigned char* pixels = nullptr;
				int width = 0, height = 0, bytePerPixel = 4;
				pixels = stbi_load(filename.c_str(), &width, &height, nullptr, bytePerPixel);
				assert(pixels != nullptr);

				//１行のピッチを256の倍数にしておく(バッファサイズは256の倍数でなければいけない)
				const UINT64 alignedRowPitch = (width * bytePerPixel + 0xff) & ~0xff;

				//アップロード用中間バッファをつくり、生データをコピーしておく
				ID3D12Resource* uploadBuf;
				{
					//テクスチャではなくフツーのバッファとしてつくる
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
					desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//連続したデータですよ
					desc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし
					desc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
					desc.SampleDesc.Quality = 0;
					Hr = Device->CreateCommittedResource(
						&prop,
						D3D12_HEAP_FLAG_NONE,
						&desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&uploadBuf));
					assert(SUCCEEDED(Hr));

					//生データをuploadbuffに一旦コピーします
					uint8_t* mapBuf = nullptr;
					Hr = uploadBuf->Map(0, nullptr, (void**)&mapBuf);//マップ
					auto srcAddress = pixels;
					auto originalRowPitch = width * bytePerPixel;
					for (int y = 0; y < height; ++y) {
						memcpy(mapBuf, srcAddress, originalRowPitch);
						//1行ごとの辻褄を合わせてやる
						srcAddress += originalRowPitch;
						mapBuf += alignedRowPitch;
					}
					uploadBuf->Unmap(0, nullptr);//アンマップ
				}

				//そして、最終コピー先であるテクスチャバッファを作る
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

				//uploadBufからtextureBufへコピーする長い道のりが始まります

				//まずコピー元ロケーションの準備・フットプリント指定
				D3D12_TEXTURE_COPY_LOCATION src = {};
				src.pResource = uploadBuf;
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				src.PlacedFootprint.Footprint.Width = static_cast<UINT>(width);
				src.PlacedFootprint.Footprint.Height = static_cast<UINT>(height);
				src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(1);
				src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(alignedRowPitch);
				src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				//コピー先ロケーションの準備・サブリソースインデックス指定
				D3D12_TEXTURE_COPY_LOCATION dst = {};
				dst.pResource = TextureBuf;
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = 0;

				//コマンドリストでコピーを予約しますよ！！！
				CommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
				//ってことはバリアがいるのです
				D3D12_RESOURCE_BARRIER barrier = {};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = TextureBuf;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				CommandList->ResourceBarrier(1, &barrier);
				//uploadBufアンロード
				CommandList->DiscardResource(uploadBuf, nullptr);
				//コマンドリストを閉じて
				CommandList->Close();
				//実行
				ID3D12CommandList* commandLists[] = { CommandList };
				CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
				//リソースがGPUに転送されるまで待機する
				WaitDrawDone();

				//コマンドアロケータをリセット
				HRESULT Hr = CommandAllocator->Reset();
				assert(SUCCEEDED(Hr));
				//コマンドリストをリセット
				Hr = CommandList->Reset(CommandAllocator, nullptr);
				assert(SUCCEEDED(Hr));

				//開放
				uploadBuf->Release();
				stbi_image_free(pixels);
			}{}
			//ディスクリプタヒープをつくり、そこにビューをつくる
			{
				//「ビュー」の入れ物である「ディスクリプタヒープ」をつくる
				{
					D3D12_DESCRIPTOR_HEAP_DESC desc = {};
					desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
					desc.NumDescriptors = 3;//コンスタントバッファ２つとテクスチャバッファ１つ
					desc.NodeMask = 0;
					desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
					Hr = Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&CbvTbvHeap));
					assert(SUCCEEDED(Hr));
				}

				auto hCbvTbvHeap = CbvTbvHeap->GetCPUDescriptorHandleForHeapStart();
				auto heapSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				//コンスタントバッファ０の「ビュー」を「ディスクリプタヒープ」につくる
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					desc.BufferLocation = ConstBuf0->GetGPUVirtualAddress();
					desc.SizeInBytes = static_cast<UINT>(ConstBuf0->GetDesc().Width);
					Device->CreateConstantBufferView(&desc, hCbvTbvHeap);
				}

				hCbvTbvHeap.ptr += heapSize;

				//コンスタントバッファ１の「ビュー」を「ディスクリプタヒープ」につくる
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					desc.BufferLocation = ConstBuf1->GetGPUVirtualAddress();
					desc.SizeInBytes = static_cast<UINT>(ConstBuf1->GetDesc().Width);
					Device->CreateConstantBufferView(&desc, hCbvTbvHeap);
				}

				hCbvTbvHeap.ptr += heapSize;

				//テクスチャバッファの「ビュー」を「ディスクリプタヒープ」につくる
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
					desc.Format = TextureBuf->GetDesc().Format;
					desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
					desc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1
					Device->CreateShaderResourceView(TextureBuf, &desc, hCbvTbvHeap);
				}
			}
		}

	}{}
	//パイプライン
	{
		//ルートシグネチャ
		{
			//ディスクリプタレンジ。ディスクリプタヒープとシェーダを紐づける役割をもつ。
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

			//ルートパラメタをディスクリプタテーブルとして使用
			D3D12_ROOT_PARAMETER rootParam[1] = {};
			rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParam[0].DescriptorTable.pDescriptorRanges = range;
			rootParam[0].DescriptorTable.NumDescriptorRanges = _countof(range);
			rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			//サンプラの記述。このサンプラがシェーダーの s0 にセットされる
			D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
			samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//補間しない(ニアレストネイバー)
			samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し
			samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し
			samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し
			samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
			samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
			samplerDesc[0].MinLOD = 0.0f;//ミップマップ最小値
			samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//オーバーサンプリングの際リサンプリングしない？
			samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ可視

			//ルートシグニチャの記述
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			desc.pParameters = rootParam;
			desc.NumParameters = _countof(rootParam);
			desc.pStaticSamplers = samplerDesc;//サンプラーの先頭アドレス
			desc.NumStaticSamplers = _countof(samplerDesc);//サンプラー数

			//ルートシグネチャをシリアライズ⇒blob(塊)をつくる。
			ID3DBlob* blob;
			Hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
			assert(SUCCEEDED(Hr));

			//ルートシグネチャをつくる
			Hr = Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
				IID_PPV_ARGS(&RootSignature));
			assert(SUCCEEDED(Hr));
			blob->Release();
		}

		//シェーダ読み込み
		BIN_FILE12 vs("assets\\cso\\VertexShader.cso");
		assert(vs.succeeded());
		BIN_FILE12 ps("assets\\cso\\PixelShader.cso");
		assert(ps.succeeded());

		//以下、各種記述

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
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
		depthStencilDesc.StencilEnable = false;//ステンシルバッファは使わない


		//ここまでの記述をまとめてパイプラインステートオブジェクトをつくる
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

		//出力領域を設定
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = ClientWidth;
		Viewport.Height = ClientHeight;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;

		//切り取り矩形を設定
		ScissorRect.left = 0;
		ScissorRect.top = 0;
		ScissorRect.right = ClientWidth;
		ScissorRect.bottom = ClientHeight;
	}
	//メインループ
	while (true)
	{
		//ウィンドウメッセージの取得、送出
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
		//コンスタントバッファ更新
		{
			//回転用ラジアン
			static float radian = 0;
			float radius = 0.4f;
			radian += 0.01f;

			//ワールドマトリックス
			XMMATRIX world = XMMatrixRotationY(radian);
			//ビューマトリックス
			XMVECTOR eye = { 0, 3, -10 }, focus = { 0, 3, 0 }, up = { 0, 1, 0 };
			XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
			//プロジェクションマトリックス
			XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, Aspect, 1.0f, 21.0f);

			//コンスタントバッファ０更新
			MapConstBuf0->mat = world * view * proj;
			//コンスタントバッファ１更新
			MapConstBuf1->diffuse = { 1.0f,1.0f,1.0f,1.0f };
		}
		//バックバッファをクリア
		{
			//現在のバックバッファのインデックスを取得。このプログラムの場合0 or 1になる。
			BackBufIdx = SwapChain->GetCurrentBackBufferIndex();

			//バリアでバックバッファを描画ターゲットに切り替える
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//このバリアは状態遷移タイプ
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = BackBufs[BackBufIdx];//リソースはバックバッファ
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;//遷移前はPresent
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;//遷移後は描画ターゲット
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			CommandList->ResourceBarrier(1, &barrier);

			//バックバッファの場所を指すディスクリプタヒープハンドルを用意する
			auto hBbvHeap = BbvHeap->GetCPUDescriptorHandleForHeapStart();
			hBbvHeap.ptr += BackBufIdx * BbvHeapSize;
			//デプスステンシルバッファのディスクリプタハンドルを用意する
			auto hDsvHeap = DsvHeap->GetCPUDescriptorHandleForHeapStart();
			//バックバッファとデプスステンシルバッファを描画ターゲットとして設定する
			CommandList->OMSetRenderTargets(1, &hBbvHeap, false, &hDsvHeap);

			//描画ターゲットをクリアする
			const float clearColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };
			CommandList->ClearRenderTargetView(hBbvHeap, clearColor, 0, nullptr);

			//デプスステンシルバッファをクリアする
			CommandList->ClearDepthStencilView(hDsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			//ビューポートとシザー矩形を設定
			CommandList->RSSetViewports(1, &Viewport);
			CommandList->RSSetScissorRects(1, &ScissorRect);
		}
		//バックバッファに描画
		{
			//パイプラインステートをセット
			CommandList->SetPipelineState(PipelineState);
			//ルートシグニチャをセット
			CommandList->SetGraphicsRootSignature(RootSignature);

			//頂点をセット
			D3D12_VERTEX_BUFFER_VIEW vertexBufViews[] = { PositionBufView, TexcoordBufView, };
			CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CommandList->IASetVertexBuffers(0, _countof(vertexBufViews), vertexBufViews);

			//ディスクリプタヒープをGPUにセット。まだ、シェーダと紐づいていない。
			CommandList->SetDescriptorHeaps(1, &CbvTbvHeap);
			//ディスクリプタヒープをディスクリプタテーブルにセット。シェーダと紐づく。
			CommandList->SetGraphicsRootDescriptorTable(0, CbvTbvHeap->GetGPUDescriptorHandleForHeapStart());
#if 1
			//描画。インデックスを使用しない
			CommandList->DrawInstanced(NumVertices, 1, 0, 0);
#else
			//描画。インデックスを使用する
			CommandList->IASetIndexBuffer(&IndexBufView);
			CommandList->DrawIndexedInstanced(NumIndices, 1, 0, 0, 0);
#endif
		}
		//バックバッファを表示
		{
			//バリアでバックバッファを表示用に切り替える
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//このバリアは状態遷移タイプ
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = BackBufs[BackBufIdx];//リソースはバックバッファ
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;//遷移前は描画ターゲット
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;//遷移後はPresent
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			CommandList->ResourceBarrier(1, &barrier);

			//コマンドリストをクローズする
			CommandList->Close();
			//コマンドリストを実行する
			ID3D12CommandList* commandLists[] = { CommandList };
			CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
			//描画完了を待つ
			WaitDrawDone();

			//バックバッファを表示
			SwapChain->Present(1, 0);

			//コマンドアロケータをリセット
			Hr = CommandAllocator->Reset();
			assert(SUCCEEDED(Hr));
			//コマンドリストをリセット
			Hr = CommandList->Reset(CommandAllocator, nullptr);
			assert(SUCCEEDED(Hr));
		}
	}{}
	//解放
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

// 描画完了を待つ
void WaitDrawDone()
{
	//現在のFence値がコマンド終了後にFenceに書き込まれるようにする
	UINT64 fvalue = FenceValue;
	CommandQueue->Signal(Fence, fvalue);
	FenceValue++;

	//まだコマンドキューが終了していないことを確認する
	if (Fence->GetCompletedValue() < fvalue)
	{
		//このFenceにおいて、fvalue の値になったらイベントを発生させる
		Fence->SetEventOnCompletion(fvalue, FenceEvent);
		//イベントが発生するまで待つ
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