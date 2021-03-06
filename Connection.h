#pragma once
#include <Windows.h>
#include <d3d11.h>
#include "Node.h"
#include "Spline.h"

class Node;

class Connection
{
private:
	Spline* spline;
public:
	Node* start;
	Node* end;
	int cost;

	Connection() :start(nullptr), end(nullptr), spline(nullptr), cost(-1) {};
	Connection(Node* start, Node* end, int cost, ID3D11DeviceContext* context, ID3D11Device* device, ID3D11InputLayout* splineLayout);
	~Connection();

	void Draw(ID3D11PixelShader* pShader, ID3D11VertexShader* vShader, Camera& cam);
	inline void setColour(DirectX::XMFLOAT3 colour) { spline->changeColour(DirectX::XMFLOAT4(colour.x, colour.y, colour.z, 1.0)); }

	Connection& operator=(Connection other)
	{
		std::swap(spline, other.spline);

		std::swap(start, other.start);

		std::swap(end, other.end);
		return *this;
	}
};