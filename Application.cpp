#include "Application.h"

using namespace DirectX;
using std::vector;

//This is a static variable that's been declared in Application.h
std::queue<Event> Application::inputEventQueue;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
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

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;

	_depthStencilView = nullptr;
	_depthStencilBuffer = nullptr;

	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	
	skyboxPS = nullptr;
	skyboxVS = nullptr;
	
	lineVS = nullptr;
	linePS = nullptr;
	
	_pVertexLayout = nullptr;
	basicVertexLayout = nullptr;

	_wireFrame = nullptr;
	_solid = nullptr;
	wfRender = false;

	samplerLinear = nullptr;

	frameConstantBuffer = nullptr;
	objectConstantBuffer = nullptr;

	cubeMesh = new MeshData();
	pipeMesh = new MeshData();
	grassMesh = new MeshData();
	agentMesh = new MeshData();

	cameraMoveSpeed = 0.1f;
	cameraPanSpeed = 0.25f;
	lastMousePos = XMFLOAT2(0, 0);
	objects = vector<GameObject>();
	
	playerSpawn = XMFLOAT3(0, 0, 0);
	agents = vector<Agent>();
	agentSplines = vector<Spline>(numAgents);
	
	splines = vector<Spline>();
	obstacleBoxes = vector<BoundingBox>();
	flashlightOn = false;
	renderGraph = false;

	currentCamera = nullptr;

	graphYPosition = 1.0f;

	blackboard = Blackboard(numAgents);
	exit = ExitPoint();

	timeLastFrame = GetTickCount64();
	timeStarted = timeLastFrame;
}

Application::Application(const Application& other)
{
	lastMousePos =  other.lastMousePos;
	cameraPanSpeed = other.cameraPanSpeed;
	cameraMoveSpeed = other.cameraMoveSpeed;

	flashlightOn = other.flashlightOn;
	inputEventQueue, other.inputEventQueue;
	input, other.input;

	graphYPosition, other.graphYPosition;
	renderGraph, other.renderGraph;
	navGraph, other.navGraph;

	skybox, other.skybox;

	blackboard = other.blackboard;
	agents = other.agents;
	player = other.player;

	obstacleBoxes = other.obstacleBoxes;
	splines = other.splines;
	agentSplines = other.agentSplines;
	objects = other.objects;

	agentMaterial = other.agentMaterial;
	agentMesh = other.agentMesh;

	grassMaterial = other.grassMaterial;
	grassMesh = other.grassMesh;

	pipeMaterials = other.pipeMaterials;
	pipeMesh = other.pipeMesh;

	cubeMesh = other.cubeMesh;

	viewFrustum = other.viewFrustum;

	playerPerspective = other.playerPerspective;
	currentCamera = other.currentCamera;
	camera = other.camera;

	perFrameCB = other.perFrameCB;

	frameConstantBuffer = other.frameConstantBuffer;
	frameConstantBuffer->AddRef();
	objectConstantBuffer = other.objectConstantBuffer;
	objectConstantBuffer->AddRef();

	samplerLinear = other.samplerLinear;
	samplerLinear->AddRef();

	wfRender = other.wfRender;
	_wireFrame = other._wireFrame;
	_wireFrame->AddRef();
	_solid = other._solid;
	_solid->AddRef();

	basicVertexLayout = other.basicVertexLayout;
	basicVertexLayout->AddRef();
	_pVertexLayout = other._pVertexLayout;
	_pVertexLayout->AddRef();

	lineVS = other.lineVS;
	lineVS->AddRef();
	linePS = other.linePS;
	linePS->AddRef();

	skyboxVS = other.skyboxVS;
	skyboxVS->AddRef();
	skyboxPS = other.skyboxPS;
	skyboxPS->AddRef();

	_pVertexShader = other._pVertexShader;
	_pVertexShader->AddRef();
	_pPixelShader = other._pPixelShader;
	_pPixelShader->AddRef();

	_depthStencilView = other._depthStencilView;
	_depthStencilView->AddRef();
	_depthStencilBuffer = other._depthStencilBuffer;
	_depthStencilBuffer->AddRef();

	_pd3dDevice = other._pd3dDevice;
	_pd3dDevice->AddRef();
	_pImmediateContext = other._pImmediateContext;
	_pImmediateContext->AddRef();
	_pSwapChain = other._pSwapChain;
	_pSwapChain->AddRef();
	_pRenderTargetView = other._pRenderTargetView;
	_pRenderTargetView->AddRef();

	_driverType = other._driverType;
	_featureLevel = other._featureLevel;

	_hWnd = other._hWnd;
	_hInst = other._hInst;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;

	ID3DBlob* skyboxVSBuffer = nullptr;
	ID3DBlob* skyboxPSBuffer = nullptr;

	//Skybox
	hr = CompileShaderFromFile(L"SkyBox.fx", "SKYMAP_VS", "vs_4_0", &skyboxVSBuffer);
	hr = CompileShaderFromFile(L"SkyBox.fx", "SKYMAP_PS", "ps_4_0", &skyboxPSBuffer);

	hr = _pd3dDevice->CreateVertexShader(skyboxVSBuffer->GetBufferPointer(), skyboxVSBuffer->GetBufferSize(), NULL, &skyboxVS);
	hr = _pd3dDevice->CreatePixelShader(skyboxPSBuffer->GetBufferPointer(), skyboxPSBuffer->GetBufferSize(), NULL, &skyboxPS);

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

	ID3DBlob* lineVSBuffer = nullptr;
	ID3DBlob* linePSBuffer = nullptr;

	//Skybox
	hr = CompileShaderFromFile(L"LineShader.fx", "lineVertexShader", "vs_4_0", &lineVSBuffer);
	hr = CompileShaderFromFile(L"LineShader.fx", "linePixelShader", "ps_4_0", &linePSBuffer);

	hr = _pd3dDevice->CreateVertexShader(lineVSBuffer->GetBufferPointer(), lineVSBuffer->GetBufferSize(), NULL, &lineVS);
	hr = _pd3dDevice->CreatePixelShader(linePSBuffer->GetBufferPointer(), linePSBuffer->GetBufferSize(), NULL, &linePS);

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC lineLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	numElements = ARRAYSIZE(lineLayout);

	// Create the input layout
	hr = _pd3dDevice->CreateInputLayout(lineLayout, numElements, lineVSBuffer->GetBufferPointer(),
		lineVSBuffer->GetBufferSize(), &basicVertexLayout);
	lineVSBuffer->Release();

	if (FAILED(hr))
		return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::initialiseCube()
{
	cubeMesh->material.diffuse = XMFLOAT4(0.8f, 0.7f, 0.5f, 1.0f);
	cubeMesh->material.specular = XMFLOAT4(0.4f, 0.35f, 0.25f, 1.0f);
	cubeMesh->material.ambient = XMFLOAT4(0.4, 0.35, 0.25, 0.2);

	CreateDDSTextureFromFile(_pd3dDevice, L"Crate_COLOR.dds", nullptr, &(cubeMesh->textureRV));
	CreateDDSTextureFromFile(_pd3dDevice, L"Crate_SPEC.dds", nullptr, &(cubeMesh->specularRV));
	CreateDDSTextureFromFile(_pd3dDevice, L"Crate_NRM.dds", nullptr, &(cubeMesh->normalMapRV));

	Parser p;
	p.readObjFile(_pd3dDevice, "box.obj", cubeMesh, nullptr);
	return S_OK;
}

HRESULT Application::initialiseGrass()
{
	*grassMesh = *cubeMesh;
	CreateDDSTextureFromFile(_pd3dDevice, L"grass.dds", nullptr, &(grassMesh->textureRV));
	CreateDDSTextureFromFile(_pd3dDevice, L"specularWhite.dds", nullptr, &(grassMesh->specularRV));
	CreateDDSTextureFromFile(_pd3dDevice, L"normalUp.dds", nullptr, &(grassMesh->normalMapRV));	
	return S_OK;
}

HRESULT Application::initialisePipe()
{
	Parser p;
	p.readObjFile(_pd3dDevice, "pipe.obj", pipeMesh, &pipeMaterials);
	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 640, 480};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
		if (pErrorBlob != nullptr)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _WindowWidth;
    sd.BufferDesc.Height = _WindowHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

	D3D11_TEXTURE2D_DESC ds_desc;
	ds_desc.Width = _WindowWidth;
	ds_desc.Height = _WindowHeight;
	ds_desc.MipLevels = 1;
	ds_desc.ArraySize = 1;
	ds_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ds_desc.SampleDesc.Count = 1;
	ds_desc.SampleDesc.Quality = 0;
	ds_desc.Usage = D3D11_USAGE_DEFAULT;
	ds_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ds_desc.CPUAccessFlags = 0;
	ds_desc.MiscFlags = 0;

	_pd3dDevice->CreateTexture2D(&ds_desc, nullptr, &_depthStencilBuffer);
	_pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_WindowWidth;
    vp.Height = (FLOAT)_WindowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(frameCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &frameConstantBuffer);

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(objectCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &objectConstantBuffer);

	D3D11_RASTERIZER_DESC wf;
	ZeroMemory(&wf, sizeof(D3D11_RASTERIZER_DESC));
	wf.FillMode = D3D11_FILL_WIREFRAME;
	wf.CullMode = D3D11_CULL_BACK;
	_pd3dDevice->CreateRasterizerState(&wf, &_wireFrame);

	D3D11_RASTERIZER_DESC sol;
	ZeroMemory(&sol, sizeof(D3D11_RASTERIZER_DESC));
	sol.FillMode = D3D11_FILL_SOLID;
	sol.CullMode = D3D11_CULL_BACK;
	sol.MultisampleEnable = true;
	_pd3dDevice->CreateRasterizerState(&sol, &_solid);

	_pImmediateContext->RSSetState(_solid);
	wfRender = false;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	_pd3dDevice->CreateSamplerState(&sampDesc, &samplerLinear);
	_pImmediateContext->PSSetSamplers(0, 1, &samplerLinear);

	initObjects();

    if (FAILED(hr))
        return hr;

    return S_OK;
}

GameObject* Application::placeCrate(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT3 rotation)
{
	if (!graphMutex.try_lock())
	{
		return nullptr;
	}
	graphMutex.unlock();

	GameObject temp = GameObject(_pImmediateContext, frameConstantBuffer, objectConstantBuffer, cubeMesh, position);
	temp.setScale(scale.x, scale.y, scale.z);
	temp.setRotation(rotation.x, rotation.y, rotation.z);
	temp.UpdateMatrix();
	temp.setCollider(true);

	objects.push_back(temp);
	obstacleBoxes.push_back(temp.getBoundingBox());

	/*
	from top down:
	* _ *
	 |_|
	*   *
	*/

	navGraph.giveNode(XMFLOAT3(position.x - scale.x - 0.45f, graphYPosition, position.z - scale.z - 0.45f));
	navGraph.giveNode(XMFLOAT3(position.x - scale.x - 0.45f, graphYPosition, position.z + scale.z + 0.45f));
	navGraph.giveNode(XMFLOAT3(position.x + scale.x + 0.45f, graphYPosition, position.z - scale.z - 0.45f));
	navGraph.giveNode(XMFLOAT3(position.x + scale.x + 0.45f, graphYPosition, position.z + scale.z + 0.45f));

	return &objects.back();
}

void Application::readInitFile(std::string fileName)
{
	/*
	Reads world data from file.
	The syntax for the file is:
	Lines starting with a # is treated as a comment
	Other than that, each line should be an isolated piece of information that gives all of the data needed to instantiate the object in the format:
	MESHNAME LOCATION SCALE ROTATION
	or
	SPLINE CONTROLPOINT CONTROLPOINT ...
	e.g. CRATE (0,0,0) (1,1,1) (0,0,0)

	The list of valid meshnames are:
	CRATE
	SPLINE
	PIPE
	EXIT
	*/

	std::fstream worldFile;
	worldFile.open(fileName, std::fstream::in);

	if (!worldFile.is_open())
	{
		throw std::exception::exception("Error opening file");
	}

	std::string line;
	std::regex const matcher("([[:upper:]]+) \\((-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*)\\) \\((-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*)\\) \\((-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*)\\)");
	std::regex const splineMatch("(SPLINE)");
	std::regex const endSplineMatch("(END)");
	std::regex const posMatch("\\((-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*)\\)");
	std::regex const playerMatch("(PLAYER) \\((-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*),(-*[[:digit:]]+.*[[:digit:]]*)\\)");

	while (std::getline(worldFile, line))
	{
		std::smatch captures;
		if (line.find('#') != std::string::npos)
		{
			continue;
		}

		if (std::regex_match(line, captures, matcher))
		{
			std::string name = captures[1];
			XMFLOAT3 position(std::stof(captures[2]), std::stof(captures[3]), std::stof(captures[4]));
			XMFLOAT3 scale(std::stof(captures[5]), std::stof(captures[6]), std::stof(captures[7]));
			XMFLOAT3 rotation(std::stof(captures[8]), std::stof(captures[9]), std::stof(captures[10]));

			if (name == "CRATE")
			{
				placeCrate(position, scale, rotation);
			}
			else if (name == "PIPE")
			{
				GameObject temp = GameObject(_pImmediateContext, frameConstantBuffer, objectConstantBuffer, pipeMesh, position);
				temp.setScale(scale.x, scale.y, scale.z);
				temp.setRotation(rotation.x, rotation.y, rotation.z);
				temp.UpdateMatrix();
				objects.push_back(temp);
			}
			else if (name == "GRASS")
			{
				GameObject temp = GameObject(_pImmediateContext, frameConstantBuffer, objectConstantBuffer, grassMesh, position);
				temp.setScale(scale.x, scale.y, scale.z);
				temp.setRotation(rotation.x, rotation.y, rotation.z);
				temp.UpdateMatrix();
				temp.setCullState(false);
				temp.setCollider(true);
				temp.setMovementState(false);
				temp.setIsGround(true);
				objects.push_back(temp);
			}
			else if (name == "EXIT")
			{
				GameObject temp = GameObject(_pImmediateContext, frameConstantBuffer, objectConstantBuffer, cubeMesh, position);
				temp.setScale(scale.x, scale.y, scale.z);
				temp.setRotation(rotation.x, rotation.y, rotation.z);
				temp.UpdateMatrix();
				temp.setCullState(false);
				temp.setCollider(false);
				temp.setMovementState(true);
				temp.setIsGround(false);
				exit = ExitPoint(temp, 1.0f, 0.4f, 0.2f);
				blackboard.setExitLocation(position);
			}
		}
		else if (std::regex_match(line, captures, splineMatch))
		{
			std::vector<XMFLOAT3> controlPoints;
			while(!std::regex_match(line, captures, endSplineMatch))
			{
				std::getline(worldFile, line);
				if (std::regex_match(line, captures, posMatch))
				{
					XMFLOAT3 position(std::stof(captures[1]), std::stof(captures[2]), std::stof(captures[3]));
					controlPoints.push_back(position);
				}
			}
			splines.push_back(Spline(controlPoints, _pImmediateContext, _pd3dDevice, basicVertexLayout));			
		}
		else if (std::regex_match(line, captures, playerMatch))
		{
			XMFLOAT3 position(std::stof(captures[2]), std::stof(captures[3]), std::stof(captures[4]));
			playerSpawn = position;
		}
	}
}

void Application::initialiseAgents(XMFLOAT2 floorSize)
{
	*agentMesh = *cubeMesh;

	std::random_device device;
	std::mt19937 engine(device());
	std::uniform_real_distribution<float> distrX(-floorSize.x / 2.0f, floorSize.x / 2.0f);
	std::uniform_real_distribution<float> distrZ(-floorSize.y / 2.0f, floorSize.y / 2.0f);

	XMFLOAT3 position = XMFLOAT3(distrX(engine), 1.0f, distrZ(engine));
	
	const float agentSize = agentMesh->size.x * 0.2f;
	const float radius = agentSize * numAgents;

	for (unsigned int i = 0; i < obstacleBoxes.size(); i++)
	{
		if (obstacleBoxes.at(i).Contains(BoundingSphere(position, radius)))
		{
			position = XMFLOAT3(distrX(engine), 1.0f, distrZ(engine));
			i = 0;
		}
	}

	XMVECTOR centre = XMLoadFloat3(&position);
	XMVECTOR direction = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	float rotationAngle = XM_2PI / numAgents;

	XMVECTOR rotationVector = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationAngle);

	for (unsigned int i = 0; i < numAgents; i++)
	{
		XMStoreFloat3(&position, centre + direction);
		direction = XMVector3Rotate(direction, rotationVector);

		agents.push_back(Agent(_pImmediateContext, frameConstantBuffer, objectConstantBuffer, agentMesh, &navGraph, &blackboard, position, i));
		agents.back().setScale(0.2f, 0.5f, 0.2f);
		agents.back().UpdateMatrix();
	}
}

void Application::initObjects()
{
	initialiseCube();
	initialisePipe();
	initialiseGrass();

	camera = Camera(XM_PIDIV2, _WindowWidth / (FLOAT)_WindowHeight, 0.00001f, 100.0f);
	camera.setPosition(XMFLOAT4(0.0f, 15.0f, 0.0f, 1.0f));
	camera.Pitch(XM_PIDIV2);

	input = Input("input_map.txt");

	navGraph = Graph(_pImmediateContext, _pd3dDevice, frameConstantBuffer, objectConstantBuffer, cubeMesh, basicVertexLayout);

	readInitFile("worldData.txt");

	player = Player(playerSpawn, _WindowWidth, _WindowHeight);

	currentCamera = player.getCamera();
	playerPerspective = true;

	skybox = Skybox();
	skybox.init(_pImmediateContext, _pd3dDevice, L"snowcube.dds");

	XMFLOAT2 floorSize = XMFLOAT2(0, 0);

	for (unsigned int i = 0; i < objects.size(); i++)
	{
		if (objects.at(i).getIsGround())
		{
			floorSize.x += objects.at(i).Size().x;
			floorSize.y += objects.at(i).Size().z;
		}
	}

	viewFrustum = Frustum();
	initialiseAgents(floorSize);

	//Lights
	perFrameCB.dirLight = DirectionalLight(XMFLOAT4(0.5, 0.5, 0.5, 1.0), XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f), XMFLOAT4(0.5, 0.5, 0.5, 1.0), XMFLOAT3(0.115f, -0.57735f, -0.94f));
	perFrameCB.pointLight = PointLight(XMFLOAT4(0.7, 0.7, 0.7, 10.0), XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f), XMFLOAT3(0,0,0), 5, XMFLOAT3(0.0f, 1.0f, 0.0f));
	perFrameCB.spotLight = SpotLight(XMFLOAT4(0.5, 0.5, 0.5, 10.0), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), XMFLOAT4(0.5, 0.5, 0.5, 1.0), XMFLOAT3(0,0,0), 5, XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0), 96);
}

void Application::resetMap()
{
	player.moveTo(playerSpawn);
	
	if (numBoxesFired > 0)
	{
		objects.erase(objects.end() - numBoxesFired, objects.end());
		obstacleBoxes.erase(obstacleBoxes.end() - numBoxesFired, obstacleBoxes.end());
	}

	navGraph.resetGraph();

	XMFLOAT2 floorSize = XMFLOAT2(0, 0);

	for (unsigned int i = 0; i < objects.size(); i++)
	{
		if (objects.at(i).getIsGround())
		{
			floorSize.x += objects.at(i).Size().x;
			floorSize.y += objects.at(i).Size().z;
		}
	}

	std::random_device device;
	std::mt19937 engine(device());
	std::uniform_real_distribution<float> distrX(-floorSize.x / 2.0f, floorSize.x / 2.0f);
	std::uniform_real_distribution<float> distrZ(-floorSize.y / 2.0f, floorSize.y / 2.0f);

	XMFLOAT3 position = XMFLOAT3(distrX(engine), 1.0f, distrZ(engine));

	const float agentSize = agentMesh->size.x * 0.2f;
	const float radius = agentSize * numAgents;

	for (unsigned int i = 0; i < obstacleBoxes.size(); i++)
	{
		if (obstacleBoxes.at(i).Contains(BoundingSphere(position, radius)))
		{
			position = XMFLOAT3(distrX(engine), 1.0f, distrZ(engine));
			i = 0;
		}
	}

	XMVECTOR centre = XMLoadFloat3(&position);
	XMVECTOR direction = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	float rotationAngle = XM_2PI / numAgents;

	XMVECTOR rotationVector = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationAngle);

	for (unsigned int i = 0; i < numAgents; i++)
	{
		XMStoreFloat3(&position, centre + direction);
		direction = XMVector3Rotate(direction, rotationVector);

		agents.at(i).moveTo(position.x, position.y, position.z);
		agents.back().UpdateMatrix();
	}
}

void Application::Cleanup()
{
	if (_pd3dDevice) _pd3dDevice->Release();
    if (_pImmediateContext) _pImmediateContext->ClearState();
	if (_pImmediateContext) _pImmediateContext->Release();
	if (_pSwapChain) _pSwapChain->Release();
	if (_pRenderTargetView) _pRenderTargetView->Release();

	if (_depthStencilView) _depthStencilView->Release();
	if (_depthStencilBuffer) _depthStencilBuffer->Release();

	if (_pVertexShader) _pVertexShader->Release();
	if (_pPixelShader) _pPixelShader->Release();

	if (skyboxPS) skyboxPS->Release();
	if (skyboxVS) skyboxVS->Release();

	if (lineVS) lineVS->Release();
	if (linePS) linePS->Release();

	if (_pVertexLayout) _pVertexLayout->Release();
	if (basicVertexLayout) basicVertexLayout->Release();

	if (_wireFrame) _wireFrame->Release();
	if (_solid) _solid->Release();

	if (samplerLinear) samplerLinear->Release();

    if (objectConstantBuffer) objectConstantBuffer->Release();
	if (frameConstantBuffer) frameConstantBuffer->Release();
	
	delete cubeMesh;
	delete pipeMesh;
	delete grassMesh;
	delete agentMesh;
}

void Application::fireBox()
{
	XMFLOAT3 cameraPos = player.Position();
	XMFLOAT3 cameraForwards = player.getCamera()->getForwards();

	XMVECTOR posVect = XMLoadFloat3(&cameraPos);
	XMVECTOR forwardsVector = XMLoadFloat3(&cameraForwards);
	forwardsVector = XMVector3Normalize(forwardsVector);
	XMStoreFloat3(&cameraForwards, forwardsVector);

	std::vector<BoundingBox> groundBoxes;
	for (auto object : objects)
	{
		if (object.getIsGround())
		{
			groundBoxes.push_back(object.getBoundingBox());
		}
	}

	float distance;
	for (BoundingBox box : groundBoxes)
	{
		if (box.Intersects(posVect, forwardsVector, distance))
		{
			posVect = XMVector3Transform(posVect, XMMatrixTranslation(cameraForwards.x * distance, cameraForwards.y * distance, cameraForwards.z * distance));
			XMStoreFloat3(&cameraPos, posVect);
			cameraPos.y = graphYPosition;
			numBoxesFired++;
			Logger::Instance() << "[Box fired]" << Logger::endl;
			GameObject* fired = placeCrate(cameraPos, XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(0, 0, 0));
			BoundingBox firedBB = fired->getBoundingBox();
			firedBB.Extents.x *= 2.0f;
			firedBB.Extents.y *= 2.0f;
			firedBB.Extents.z *= 2.0f;

			blackboard.noiseMade(cameraPos, 50);

			for (unsigned int i = 0; i < agents.size(); i++)
			{
				if (firedBB.Intersects(agents.at(i).getBoundingBox()))
				{
					agents.at(i).stun();
				}
			}

			break;
		}
	}
}

void Application::onMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(cameraPanSpeed * (x - lastMousePos.x));
		float dy = XMConvertToRadians(cameraPanSpeed * (y - lastMousePos.y));

		player.getCamera()->Pitch(dy);
		player.getCamera()->Yaw(dx);
	}

	lastMousePos.x = (float)x;
	lastMousePos.y = (float)y;
	player.getCamera()->Update();
}

void Application::pushEvent(Event toPush)
{
	inputEventQueue.push(toPush);
}

void Application::handleMessages(double delta)
{
	vector<Event> playerEvents = vector<Event>();
	while (!inputEventQueue.empty())
	{
		Event next = inputEventQueue.front();

		switch (next)
		{
		case WALK_FORWARDS:
			playerEvents.push_back(next);
			break;
		case WALK_BACKWARDS:
			playerEvents.push_back(next);
			break;
		case STRAFE_LEFT:
			playerEvents.push_back(next);
			break;
		case STRAFE_RIGHT:
			playerEvents.push_back(next);
			break;
		case PLACE_CRATE:
			fireBox();
			break;
		case TOGGLE_PERSPECTIVE:
			if (playerPerspective)
			{
				currentCamera = &camera;
			}
			else
			{
				currentCamera = player.getCamera();
			}
			playerPerspective = !playerPerspective;
			break;
		case TOGGLE_FLASHLIGHT:
			resetMap();
			break;
		case TOGGLE_GRAPH_RENDER:
			renderGraph = !renderGraph;
			break;
		case TOGGLE_GRAPH_COLOURING:
			navGraph.flipColouringConnections();
			break;
		case NO_SUCH_EVENT:
		default:
			break;
		}
		inputEventQueue.pop();
	}

	player.inputUpdate(playerEvents, delta);
}

void Application::updateGraph(std::vector<BoundingBox>& objectsBBs)
{
	graphMutex.lock();
	navGraph.calculateGraph(objectsBBs);
	graphMutex.unlock();
}

void Application::handleCollisions()
{
	Collision::AABB playerBB = Collision::AABB(player.Position(), player.Size());
	Collision::MTV mtv;
	for (unsigned int i = 0; i < objects.size(); i++)
	{
		if (!objects.at(i).getCollider())
		{
			continue;
		}

		XMFLOAT3 size = objects.at(i).Size();

		Collision::AABB objAABB = Collision::AABB(objects.at(i).Pos(), objects.at(i).Size());
		

		if (Collision::boundingBoxCollision(playerBB, objAABB, mtv))
		{
			player.moveFromCollision(XMFLOAT3(mtv.axis.x * mtv.magnitude, mtv.axis.y * mtv.magnitude, mtv.axis.z * mtv.magnitude));
		}

		for (unsigned int j = 0; j < objects.size(); j++)
		{
			if (i == j || !objects.at(j).getCollider())
			{
				continue;
			}

			Collision::AABB closeAABB = Collision::AABB(objects.at(j).Pos(), objects.at(j).Size());

			if (Collision::boundingBoxCollision(objAABB, closeAABB, mtv))
			{
				if (objects.at(i).getIsGround())
				{
					objects.at(j).setOnGround(true);
				}
				if (objects.at(j).getIsGround())
				{
					objects.at(i).setOnGround(true);
				}
				objects.at(i).moveFromCollision(mtv.axis.x * mtv.magnitude, mtv.axis.y * mtv.magnitude, mtv.axis.z * mtv.magnitude);
			}
		}

		for (unsigned int j = 0; j < agents.size(); j++)
		{
			Collision::AABB agentAABB = Collision::AABB(agents.at(j).Pos(), agents.at(j).Size());

			if (Collision::boundingBoxCollision(agentAABB, objAABB, mtv))
			{
				if (objects.at(i).getIsGround())
				{
					agents.at(j).setOnGround(true);
				}

				agents.at(j).moveFromCollision(mtv.axis.x * mtv.magnitude, mtv.axis.y * mtv.magnitude, mtv.axis.z * mtv.magnitude);
			}
		}
	}

	if (exit.collidesWith(player.Position(), 0.5f))
	{
		resetMap();
	}
}

void Application::Update()
{
    // Update our time
	double deltaTime = GetTickCount64() - timeLastFrame;
	deltaTime /= 1000.0;
	timeLastFrame = GetTickCount64();

	//Handle object update
	for (unsigned int i = 0; i < objects.size(); i++)
	{
		objects.at(i).Update(deltaTime);
	}

	//Deal with input
	input.handleInput(&Application::pushEvent);
	handleMessages(deltaTime);

	//Update the player and the camera
	player.Update();
	
	if (!playerPerspective)
	{
		camera.Update();
	}

	//Construct view frustum for culling
	viewFrustum.constructFrustum(currentCamera->viewDistance() * 2, currentCamera->fov() * 2, currentCamera->aspectRatio() * 2, 0, currentCamera->zFar() * 2, currentCamera->getView());

	skybox.Update(currentCamera);

	//Update navGraph if it's needed
	if (graphMutex.try_lock())
	{
		graphMutex.unlock();
		if (navGraph.needsRecalculating())
		{
			/*Update the navigation graph*/
			std::thread(&Application::updateGraph, this, obstacleBoxes).detach();
		}
	}

	exit.Update(deltaTime);

	//Blackboard update for deprication of confidence etc.
	blackboard.Update(deltaTime);

	//Clear splines so they can be updated
	agentSplines.clear();
	vector<XMFLOAT3> cp = vector<XMFLOAT3>(2);

	//Update agents, and recreate the splines facing forwards
	for (unsigned int i = 0; i < agents.size(); i++)
	{
		auto splineIterator = agentSplines.begin() + i;
		agents.at(i).Update(deltaTime, obstacleBoxes);
		XMFLOAT3 f = agents.at(i).getFacing();
		XMFLOAT3 p = agents.at(i).Pos();

		XMVECTOR fPos = XMLoadFloat3(&p);
		fPos += XMLoadFloat3(&f);
		XMStoreFloat3(&f, fPos);

		cp.at(0) = f;
		cp.at(1) = p;
		
		if (agents.at(i).canSeePlayer(player.Position(), obstacleBoxes))
		{
			agentSplines.emplace(splineIterator, cp, _pImmediateContext, _pd3dDevice, basicVertexLayout, XMFLOAT4(1, 0, 0, 1));
		}else
		{
			agentSplines.emplace(splineIterator, cp, _pImmediateContext, _pd3dDevice, basicVertexLayout, XMFLOAT4(0, 1, 0, 1));
		}
	}

	handleCollisions();
}

void Application::Draw()
{
    //
    // Clear the back buffer
    //
    float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f};
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
	_pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Update the constant buffer used for this frame
	perFrameCB.eyePos = XMFLOAT3(currentCamera->getPosition().x, currentCamera->getPosition().y, currentCamera->getPosition().z);
	perFrameCB.pointLight.position = perFrameCB.eyePos;

	if (flashlightOn)
	{
		perFrameCB.spotLight.range = 15;
		perFrameCB.spotLight.direction = currentCamera->getForwards();
		perFrameCB.spotLight.position = perFrameCB.eyePos;
	}
	else
	{
		perFrameCB.spotLight.range = 0;
	}

	_pImmediateContext->UpdateSubresource(frameConstantBuffer, 0, nullptr, &perFrameCB, 0, 0);

	//Begin drawing objects in the world
	for (unsigned int i = 0; i < objects.size(); i++)
	{
		objects.at(i).Draw(_pPixelShader, _pVertexShader, viewFrustum, *currentCamera);
	}

	exit.Draw(_pPixelShader, _pVertexShader, viewFrustum, *currentCamera);

	for (unsigned int i = 0; i < agents.size(); i++)
	{
		agents.at(i).Draw(_pPixelShader, _pVertexShader, viewFrustum, *currentCamera);
	}

	//If the graph should be rendered and it is available, render it
	if (renderGraph && graphMutex.try_lock())
	{
		graphMutex.unlock();
		navGraph.DrawGraph(linePS, lineVS, _pPixelShader, _pVertexShader, viewFrustum, *currentCamera);
	}

	//Draw any and all splines within the world after setting the layout
	_pImmediateContext->IASetInputLayout(basicVertexLayout);
	
	for (unsigned int i = 0; i < splines.size(); i++)
	{
		splines.at(i).Draw(linePS, lineVS, *currentCamera, false);
	}

	for (unsigned int i = 0; i < agentSplines.size(); i++)
	{
		agentSplines.at(i).Draw(linePS, lineVS, *currentCamera, false);
	}
	//Don't forget to set the layout back to the regular vertex layout
	_pImmediateContext->IASetInputLayout(_pVertexLayout);

	//Draw the skybox last because it is the most likely to be drawn over
	skybox.Draw(skyboxVS, skyboxPS, currentCamera);

    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}