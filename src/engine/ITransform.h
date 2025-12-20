#pragma once
#include "Component.h"
#include "math/MathTypes.h"

using namespace std;

class ITransform:public Component
{
public:
	PrismaMath::vec3 position;
};

