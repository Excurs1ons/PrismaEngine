#pragma once
#include <cstdint>
class Object
{
public:
	Object():m_instanceId(s_nextId++) {
		
	}
	const uint32_t GetInstanceID() const { return m_instanceId; }
private:
	static uint32_t s_nextId;
	const uint32_t m_instanceId;
};

