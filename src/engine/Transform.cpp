#include "Transform.h"
PrismaEngine::Vector3 PrismaEngine::Transform::GetPosition() const {
    return position;
}
PrismaEngine::Vector3 PrismaEngine::Transform::GetForward() const {
    return rotation * Vector3(0.0f, 0.0f, 1.0f);
}
void PrismaEngine::Transform::UpdateMatrix() {
    matrix = GetMatrix();  // Changed from matrix = worldMatrix; to avoid const qualifier issues
}
