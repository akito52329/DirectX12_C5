#include <Windows.h>
#include<tchar.h>
#include<d3d12.h> //Chapter3_2_2 
#include<dxgi1_6.h>//Chapter3_2_2 
#include<vector>//Chapter3_2_2 
#include <DirectXMath.h>//Chapter4_2_1 P103
#include<d3dcompiler.h>							//Chapter4_6_2 P118
#pragma comment(lib, "d3dcompiler.lib")		//Chapter4_6_2 P118
#define _USE_MATH_DEFINES
#include <math.h>

using namespace DirectX;//Chapter4_2_1 P104

//#define DEF_TEST

#ifdef _DEBUG
#include <iostream>
#endif
using namespace std;
// @brief コンソール画面にフォーマット付き文字列を表示
// @param formatフォーマット（%dとか%fとかの）
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG    
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif 
}
//Chapter3_2_2 P65
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
//

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//Windowが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1080;
const unsigned int window_height = 600;

//Chapter3_2_2 P66
ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;	//Chapter3_3_2 P72
ID3D12GraphicsCommandList* _cmdList = nullptr;			//Chapter3_3_2 P72
ID3D12CommandQueue* _cmdQueue = nullptr;	// Chapter3_3_3 P74
IDXGISwapChain4* _swapchain = nullptr;		// Chapter3_3_3

//Chapter3_4
void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}
//



#ifdef _DEBUG 
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

#ifdef DEF_TEST
	cout << "defined DEF_TEST" << endl;
#endif
	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);	//※追加
	// ウィンドウクラスの生成＆登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;			// コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");						// アプリケーションクラス名（適当でよい）
	w.hInstance = GetModuleHandle(nullptr);					// ハンドルの取得
	RegisterClassEx(&w);													// アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）
	RECT wrc = { 0, 0, window_width, window_height };	// ウィンドウサイズを決める	

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName, // クラス名指定    
		_T("DX12テスト"),      // タイトルバーの文字    
		WS_OVERLAPPEDWINDOW,    // タイトルバーと境界線があるウィンドウ    
		CW_USEDEFAULT,          // 表示x座標はOSにお任せ    
		CW_USEDEFAULT,          // 表示y座標はOSにお任せ    
		wrc.right - wrc.left,   // ウィンドウ幅    
		wrc.bottom - wrc.top,   // ウィンドウ高    
		nullptr,                // 親ウィンドウハンドル    
		nullptr,                // メニューハンドル    
		w.hInstance,            // 呼び出しアプリケーションハンドル    
		nullptr);               // 追加パラメーター
	// ウィンドウ表示

	//Chapter3_4
#ifdef _DEBUG    //デバッグレイヤーをオンに
	EnableDebugLayer();
#endif
	//


//Chapter3_4 ←Chapter3_2_2 
// DirectX12の導入
//  1.IDXGIFactory6を生成
	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)))) {
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&_dxgiFactory)))) {
			DebugOutputFormatString("FAILED CreateDXGIFactory2");
			return -1;
		}
	}
	//  2.VGAアダプタIDXGIAdapterの配列をIDXGIFactory6から取り出す
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}

	//3.使いたいアダプタをVGAのメーカーで選ぶ
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); // アダプターの説明オブジェクト取得
		std::wstring strDesc = adesc.Description;    // 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	//4.ID3D12Deviceを選んだアダプタを用いて初期化し生成する
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = l;
			break;
		}
	}

	//Chapter3_3_2  
	//コマンド周りの準備
	// コマンドアロケーターID3D12CommandAllocatorを生成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	if (result != S_OK) {
		DebugOutputFormatString("FAILED CreateCommandAllocator");
		return -1;
	}
	// コマンドリストID3D12GraphicsCommandListを生成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	if (result != S_OK) {
		DebugOutputFormatString("FAILED CreateCommandList");
		return -1;
	}
	//Chapter3_3_3 P74  
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.NodeMask = 0; // プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// キュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

	//Chapter3_3_3 P79  
	//スワップチェーン
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	// バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	// フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// ウィンドウ⇔フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);
	if (result != S_OK) {
		DebugOutputFormatString("FAILED CreateSwapChainForHwnd");
		return -1;
	}
	// Chapter3_3_5
	// ディスクリプターヒープの生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));//バッファの位置のハンドルを取り出す
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle); //RTVをディスクリプターヒープに作成
		// ディスクリプタの先頭アドレスをRTVのサイズ分、後ろへずらず
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//

	// Chapter3_4_2 P92
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);

	//Chapter5_1 P155
	// 頂点データ構造体
	struct Vertex {
		XMFLOAT3 pos; // xyz座標    
		XMFLOAT2 uv;  // uv座標
	};

	float r = (float)window_width / window_height;


	/*Vertex  vertices[] = {
		{{-0.4f, -0.7f, 0.0f},	{0.0f, 1.0f}}, // 左下
		{{-0.4f,  0.7f, 0.0f},	{0.0f, 0.0f}}, // 左上 
		{{ 0.4f, -0.7f, 0.0f},	{1.0f, 1.0f}}, // 右下
		{{ 0.4f,  0.7f, 0.0f},	{1.0f, 0.0f}}, // 右上
	};*/

	Vertex  vertices[] = {
		{{0.0f, 0.5f ,0.0f},	{1.0f, 1.0f}}, // 0
		{{0.5f / r, 0.25f,0.0f},	{0.0f, 1.0f}}, // 1
		{{0.5f / r, -0.25f,0.0f},	{0.0f, 0.0f}}, // 2
		{{0.0f, -0.5f,0.0f},	{1.0f, 0.0f}}, // 3
		{{-0.5f / r, -0.25f,0.0f},	{0.0f, 0.0f}}, // 4
		{{-0.5f / r, 0.25f,0.0f},	{0.0f, 1.0f}}, //5
		{{0.7f, 0.0f,0.0f},	{1.0f, 0.0f}}, // 6
		{{0.7f, -0.7f,0.0f},	{1.0f, 1.0f}}, // 7
		{{1.0f , -0.7f,0.0f},	{0.0f, 0.0f}}, // 8
	};

	/*XMFLOAT3 vertices[] =
	{
			{0.0f, 0.5f ,0.0f} ,//0
			{0.5f / r, 0.25f,0.0f} ,//1
			{0.5f / r, -0.25f,0.0f} ,//2
			{0.0f, -0.5f,0.0f} ,//3
			{-0.5f / r, -0.25f,0.0f} ,	//4
			{-0.5f / r, 0.25f,0.0f},//5
			{0.7f, 0.0f,0.0f},//6
			{0.7f, -0.7f,0.0f},//7
			{1.0f , -0.7f,0.0f},//8
	};*/

	//XMFLOAT3 vertices[] = {	//TRIANGLELIST
	//	//　左下の三角形
	//	{-0.4f,-0.7f,0.0f} ,	//左下#0
	//	{-0.4f,0.7f,0.0f} ,	//左上#1
	//	{0.4f,-0.7f,0.0f} ,	//右下#2
	//	//　右上の三角形
	//	{-0.4f,0.7f,0.0f} ,	//左上#3
	//	{0.4f,0.7f,0.0f} ,	//右上#4
	//	{0.4f,-0.7f,0.0f},	//右下#5
	//};

	//XMFLOAT3 vertices[] = {	//TRIANGLESTRIP
	//{-0.4f,-0.7f,0.0f} ,//左下
	//{-0.4f,0.7f,0.0f} ,//左上
	//{0.4f,-0.7f,0.0f} ,//右下
	//{0.4f,0.7f,0.0f} ,//右上
	//{0.9f,0.0f,0.0f} ,//右中
	//};



	//Chapter4_3_4 P112
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//UPLOAD(確保は可能)
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	//Chapter5_1 P156
	Vertex* vertMap = nullptr; // 型をVertexに変更
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	//Chapter4_4_2 P114
	// 頂点バッファビューを用意
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);      // 全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]); // 1頂点あたりのバイト数

	//Chapter4_11_2 P150
	//unsigned short indices[] = { 0, 1, 2,    2, 1, 3 };
	unsigned short indices[] = { 0,1,2, 0,2,3, 0,3,4, 0,4,5, 6,7,8 };
	ID3D12Resource* idxBuff = nullptr;
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);






	//Chapter4_6_1 P118
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl", // シェーダー名
		nullptr, // define はなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicVS", "vs_5_0", // 関数は BasicVS、対象シェーダーは vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用および最適化なし
		0,
		&_vsBlob, &errorBlob); // エラー時は errorBlob にメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA(" ファイルが見当たりません ");
			return 0; // exit() などに適宜置き換えるほうがよい
		}
		else
		{
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl", // シェーダー名
		nullptr, // define はなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicPS", "ps_5_0", // 関数は BasicPS、対象シェーダーは ps_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用および最適化なし
		0,
		&_psBlob, &errorBlob); // エラー時は errorBlob にメッセージが入る
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA(" ファイルが見当たりません ");
			return 0; // exit() などに適宜置き換えるほうがよい
		}
		else
		{
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
	}

	// Chapter5_3 P157
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		 { // uv（追加）        
			 "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
			 0, D3D12_APPEND_ALIGNED_ELEMENT,
			 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		 },
	};
	//

	// Chapter4_8_3 P130
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr; // あとで設定する
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// Chapter4_8_4 P130
	// デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true; // 深度方向のクリッピングは有効に
	//残り
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Chapter4_8_5 P133
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.DepthStencilState.DepthEnable = false;
	gpipeline.DepthStencilState.StencilEnable = false;

	// Chapher4_8_6 P137
	gpipeline.InputLayout.pInputElementDescs = inputLayout; // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数
	// Chapher4_8_6 P138
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成


	// Chapher4_8_7 P139
	gpipeline.NumRenderTargets = 1; // 今は 1 つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0 ～ 1 に正規化された RGBA

	// Chapter4_8_8
	gpipeline.SampleDesc.Count = 1; // サンプリングは 1 ピクセルにつき 1
	gpipeline.SampleDesc.Quality = 0; // クオリティは最低

	// Chapter4_9_2 P141
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange = {};
	descTblRange.NumDescriptors = 1;//テクスチャひとつ
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange.BaseShaderRegister = 0;//0番スロットから
	descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;//デスクリプタレンジのアドレス
	rootparam.DescriptorTable.NumDescriptorRanges = 1;//デスクリプタレンジ数
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダから見える

	rootSignatureDesc.pParameters = &rootparam; // ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = 1;        // ルートパラメーター数	

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 奥行きの繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間
	// 補間しない（ニアレストネイバー法：最近傍補間）
	// samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
	samplerDesc.MinLOD = 0.0f; // ミップマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // リサンプリングしない

	rootSignatureDesc.pStaticSamplers = &samplerDesc; 
	rootSignatureDesc.NumStaticSamplers = 1;


	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
		&rootSigBlob, // シェーダーを作ったときと同じ
		&errorBlob); // エラー処理も同じ

	// Chapter4_9_2 P142
	result = _dev->CreateRootSignature(
		0, // nodemask。0 でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同様
		rootSigBlob->GetBufferSize(), // シェーダーのときと同様
		IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release(); // 不要になったので解放
	gpipeline.pRootSignature = rootsignature;

	// Chapter4_8_9 P139
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(
		&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// Chapter4_10_1 P143
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width; // 出力先の幅（ピクセル数）
	viewport.Height = window_height; // 出力先の高さ（ピクセル数）
	viewport.TopLeftX = 0; // 出力先の左上座標 X
	viewport.TopLeftY = 0; // 出力先の左上座標 Y
	viewport.MaxDepth = 1.0f; // 深度最大値
	viewport.MinDepth = 0.0f; // 深度最小値
	// Chapter4_10_2 P143
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0; // 切り抜き上座標
	scissorrect.left = 0; // 切り抜き左座標
	scissorrect.right = scissorrect.left + window_width; // 切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height; // 切り抜き下座標


	// Chapter5_4 P159													  
	//ノイズテクスチャの作成
	struct TexRGBA {
		unsigned char R, G, B, A;
	};
	std::vector<TexRGBA> texturedata(256 * 256);
	//for (auto& rgba : texturedata) {
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255;//アルファは1.0という事にします。
	//}
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			if ((i /16 % 2)%2==0) {
				texturedata[j * 256 + i].R = 255;
				texturedata[j * 256 + i].G = 0;
				texturedata[j * 256 + i].B = 0;
			}
			else {
				texturedata[j * 256 + i].R = 255;
				texturedata[j * 256 + i].G = 255;
				texturedata[j * 256 + i].B = 255;
			}
			texturedata[j * 256 + i].A = 1;
		}
	}

	//WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
	resDesc.Width = 256;  // 幅
	resDesc.Height = 256; // 高さ
	resDesc.DepthOrArraySize = 1;   // 2Dで配列でもないので１
	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;	// ミップマップしないのでミップ数は1つ
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//テクスチャ用(ピクセルシェーダから見る用)
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	result = texbuff->WriteToSubresource(0,
		nullptr,//全領域へコピー
		texturedata.data(),    // 元データアドレス
		sizeof(TexRGBA) * 256, // 1ラインサイズ    
		sizeof(TexRGBA) * texturedata.size() // 全サイズ
	);

	// Chapter5_7_1 P163
	ID3D12DescriptorHeap* texDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {}; // シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // マスクは0 
	descHeapDesc.NodeMask = 0; // ビューは今のところ1つだけ
	descHeapDesc.NumDescriptors = 1; // シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // 生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));
	// Chapter5_7_2 P164
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // RGBA（0.0f～1.0fに正規化）
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;      // 後述
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので1
	_dev->CreateShaderResourceView(texbuff,  // ビューと関連付けるバッファー
		&srvDesc, // 先ほど設定したテクスチャ設定情報    
		texDescHeap->GetCPUDescriptorHandleForHeapStart() // ヒープのどこに割り当てるか
	);



	MSG	msg = {};
	int g = 0;
	float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; //黄色

	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリケーションが終わるときにmessageがWM_QUITになる    
		if (msg.message == WM_QUIT) {
			break;
		}

		g++;
		clearColor[0] = sin((g % 300) / 300.0f * M_PI);
		clearColor[1] = sin((g % 200) / 200.0f * M_PI);
		clearColor[2] = sin((g % 100) / 100.0f * M_PI);

		// Chapter3_3_6
		// スワップチェーンを動作
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
		//// Chapter3_4_3　 リソースバリア
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		////

		// Chapter4_10_3 P144
		_cmdList->SetPipelineState(_pipelinestate);

		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);


		// Chapter4_10_3 P145 改造
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		//Chapter4_11_2 P151
		_cmdList->IASetIndexBuffer(&ibView);

		// Chapter5_9_2 P171
		_cmdList->SetDescriptorHeaps(1, &texDescHeap);
		// Chapter5_9_3 P171
		_cmdList->SetGraphicsRootDescriptorTable(0, // ルートパラメーターインデックス    
			texDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス


		//_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
		_cmdList->DrawIndexedInstanced(15, 1, 0, 0, 0);

		//// Chapter3_4_3　 リソースバリア
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		////

		// 命令のクローズ
		_cmdList->Close();
		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		//// Chapter3_4_2 P93
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		////

		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備
		//フリップ
		_swapchain->Present(1, 0);
		//

	}
	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	//getchar();
	return 0;
}