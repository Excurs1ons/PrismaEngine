#pragma once
#include "Component.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace std;

class ITransform:public Component
{
public:
	XMVECTOR position;
};

