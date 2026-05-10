#include "Transform.h"

namespace Prisma {

Transform::Data Transform::GetData() const {
    return {{
                m_Position.x, m_Position.y, m_Position.z
            },
            {
                m_Rotation.x, m_Rotation.y, m_Rotation.z, m_Rotation.w
            },
            {
                m_Scale.x, m_Scale.y, m_Scale.z
            }};
}

void Transform::SetData(const Data& d) {
    SetPosition({d.position[0], d.position[1], d.position[2]});
    SetRotation(Quaternion(d.rotation[3], d.rotation[0], d.rotation[1], d.rotation[2]));
    SetScale({d.scale[0], d.scale[1], d.scale[2]});
}

} // namespace Prisma
