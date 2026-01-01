#include "Transform.h"
PrismaEngine::Vector3 PrismaEngine::Transform::GetPosition() const {
    return position;
}
void PrismaEngine::Transform::UpdateMatrix() {
    matrix = GetMatrix();  // Changed from matrix = worldMatrix; to avoid const qualifier issues
}
