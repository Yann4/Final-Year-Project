#include "TravelToPositionState.h"

using namespace DirectX;
using namespace Steering;

TravelToPositionState::TravelToPositionState(Controller* owner, XMFLOAT3 position) : State(owner), position(position)
{}

void TravelToPositionState::Update(double deltaTime, std::vector<BoundingBox>& objects)
{
	XMFLOAT3 seek = seekForce(owner->position, position);
	XMFLOAT3 oa = obstacleAvoidForce(objects, position, owner->facing);

	XMFLOAT3 force = aggregateForces(seek, XMFLOAT3(0, 0, 0), oa);
	owner->force = force;
}

Priority TravelToPositionState::shouldEnter()
{
	return IMMEDIATE;
}

bool TravelToPositionState::shouldExit()
{
	XMVECTOR pos = XMLoadFloat3(&owner->position);

	XMVECTOR dest = XMLoadFloat3(&position);

	float distance = XMVectorGetX(XMVector3LengthEst(dest - pos));

	if (distance < distThreshold)
	{
		return true;
	}

	return false;
}

Priority TravelToPositionState::Exit(State** toPush)
{
	return NONE;
}