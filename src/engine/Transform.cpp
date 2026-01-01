#include "Transform.h"
PrismaEngine::Vector3 Transform::GetPosition() const {
    return position;
}
void Transform::UpdateMatrix() {
    matrix = GetMatrix(); // Changed from matrix = worldMatrix; to avoid const qualifier issues
}