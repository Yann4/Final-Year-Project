#pragma once
#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <DirectXCollision.h>
#include <vector>
#include <algorithm>
#include <stack>
#include <cmath>
#include <random>

#include "Node.h"

class Graph
{
private:
	std::vector<Node*> initialState;
	std::vector<Node*> graphNodes;
	std::vector<Node*> busyBuffer;

	bool graphUpToDate;
	bool colourConnectionsRed;

	ID3D11DeviceContext* context;
	ID3D11Device* device;
	ID3D11Buffer* constBuffer;
	ID3D11Buffer* objBuffer;
	MeshData* nodeMesh;
	ID3D11InputLayout* splineInputLayout;

	unsigned int top_id;

	bool graphBusy;
	bool addedWhileBusy;

public:
	Graph();
	Graph(ID3D11DeviceContext* context, ID3D11Device* device, ID3D11Buffer* constBuffer, ID3D11Buffer* objBuffer, MeshData* mesh, ID3D11InputLayout* splineInputLayout);
	~Graph();

	void giveNode(DirectX::XMFLOAT3 position);

	void calculateGraph(std::vector<DirectX::BoundingBox>& objects);

	void DrawGraph(ID3D11PixelShader* ConnectionPShader, ID3D11VertexShader* ConnectionVShader, ID3D11PixelShader* NodePShader, ID3D11VertexShader* NodeVShader, Frustum& frustum, Camera& cam);

	inline bool needsRecalculating() { return !graphUpToDate || addedWhileBusy; }

	inline void flipColouringConnections() 
	{
		colourConnectionsRed = !colourConnectionsRed; 
		trimConnections(); 
		if (!colourConnectionsRed)
		{
			for (auto n : graphNodes)
			{
				std::vector<Connection*>* neighs = n->getNeighboursRef();
				for (unsigned int i = 0; i < neighs->size(); i++)
				{
					neighs->at(i)->setColour(DirectX::XMFLOAT3(0, 0, 0));
				}
			}
		}
	}

	inline std::stack<DirectX::XMFLOAT3> findPath(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end)
	{ 
		return aStar(start, end);
	}

	inline bool isBusy() 
	{ 
		return graphBusy; 
	}
	inline void flipBusy() 
	{ 
		graphBusy = !graphBusy;
	}

	std::vector<DirectX::XMFLOAT3> getUnvisitedLocations();
	std::vector<Node*> getUnvisitedNodes();
	DirectX::XMFLOAT3 getNearestUnvisitedLocation(DirectX::XMFLOAT3 currentPos);
	DirectX::XMFLOAT3 getRandomUnvisitedLocation();

	void setGraphUnvisited();
	inline bool fullyVisited()
	{
		if (graphBusy)
		{
			return false;
		}

		for (unsigned int i = 0; i < graphNodes.size(); i++)
		{
			if (!graphNodes.at(i)->Visited())
			{
				return false;
			}
		}
		return true;
	}

	void visitLocation(DirectX::XMFLOAT3 location);

	void resetGraph();
private:
	Node* getNearestNode(DirectX::XMFLOAT3 position);

	std::stack<DirectX::XMFLOAT3> aStar(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end);

	float heuristic(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b);

	float euclideanHeuristic(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b);
	float euclideanHeuristicSq(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b);

	//Combines nodes that are too close together
	//Also, removes nodes that are inside blocks
	void trimNodeList(std::vector<DirectX::BoundingBox>& objects);

	void trimConnections();
};