#include "Blackboard.h"

using namespace DirectX;
using std::vector;

Blackboard::Blackboard()
{
	playerPosition = Data<XMFLOAT3>(XMFLOAT3(0, 0, 0), 0, 0);
	agentLocations = vector<XMFLOAT3>(0);
	scaredAgents = vector<bool>(0);
	agentsGuardingExit = vector<unsigned int>();
	playerTagged = false;
}

Blackboard::Blackboard(const Blackboard& other)
{
	playerPosition = other.playerPosition;
	noises = other.noises;
	agentLocations = other.agentLocations;
	scaredAgents = other.scaredAgents;
	playerTagged = other.playerTagged;

	exitLocation = other.exitLocation;
	agentsGuardingExit = other.agentsGuardingExit;
	subject = other.subject;
}

Blackboard::Blackboard(unsigned int numAgents)
{
	playerPosition = Data<XMFLOAT3>(XMFLOAT3(0, 0, 0), 0, 0);
	agentLocations = vector<XMFLOAT3>(numAgents);
	
	scaredAgents = vector<bool>(numAgents);
	std::fill(scaredAgents.begin(), scaredAgents.end(), false);

	beingExplored = vector<XMFLOAT3>(numAgents);
	
	agentsGuardingExit = vector<unsigned int>();
	playerTagged = false;
}

void Blackboard::Update(double deltaTime)
{
	UpdateDataConfidence(deltaTime);
	UpdateSoundFalloff(deltaTime);
}

void Blackboard::UpdateSoundFalloff(double deltaTime)
{
	for (unsigned int i = 0; i < noises.size(); i++)
	{
		noises.at(i).soundFalloff(deltaTime);
		
		if (noises.at(i).volume <= 0)
		{
			noises.erase(noises.begin() + i);
			i--;
			continue;
		}
	}
}

void Blackboard::UpdateDataConfidence(double deltaTime)
{
	playerPosition.Update(deltaTime);
}

vector<Sound*> Blackboard::getSoundsWithinRange(XMFLOAT3 agentPosition, float hearingRange)
{
	vector<Sound*> noisesInRange = vector<Sound*>();

	XMVECTOR agentPos = XMLoadFloat3(&agentPosition);

	for (unsigned int i = 0; i < noises.size(); i++)
	{
		XMVECTOR soundPos = XMLoadFloat3(&noises.at(i).position);

		if (XMVectorGetX(XMVector3LengthEst(soundPos - agentPos)) < hearingRange + noises.at(i).volume)
		{
			noisesInRange.push_back(&noises.at(i));
		}
	}

	return noisesInRange;
}

bool Blackboard::isAgentAlone(unsigned int agentIndex)
{
	if (agentLocations.size() == 1)
	{
		return false;
	}

	const float aloneRadius = 3.0f;
	
	XMVECTOR examinedPos = XMLoadFloat3(&agentLocations.at(agentIndex));

	for (unsigned int i = 0; i < agentLocations.size(); i++)
	{
		if (i == agentIndex)
		{
			continue;
		}

		XMVECTOR otherPos = XMLoadFloat3(&agentLocations.at(i));

		if (XMVectorGetX(XMVector3LengthSq(otherPos - examinedPos)) < powf(aloneRadius, 2.0f))
		{
			return false;
		}
	}

	return true;
}

vector<XMFLOAT3> Blackboard::agentPositions(int discounting)
{
	vector<XMFLOAT3> positions;

	for (unsigned int i = 0; i < agentLocations.size(); i++)
	{
		if (i != discounting)
		{
			positions.push_back(agentLocations.at(i));
		}
	}
	return positions;
}

bool Blackboard::isAgentScared(unsigned int agentIndex)
{
	return scaredAgents.at(agentIndex);
}

void Blackboard::setAgentScaredState(unsigned int agentIndex, bool isScared)
{
	scaredAgents.at(agentIndex) = isScared;
}

void Blackboard::flipAgentGuarding(unsigned int agentIndex)
{
	auto iterator = std::find(agentsGuardingExit.begin(), agentsGuardingExit.end(), agentIndex);

	if (iterator == agentsGuardingExit.end())
	{
		agentsGuardingExit.push_back(agentIndex);
	}
	else
	{
		agentsGuardingExit.erase(iterator);
	}
}