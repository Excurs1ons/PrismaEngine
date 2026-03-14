#include "Transform.h"
Prisma::Vector3 Prisma::Transform::GetPosition() const {
    return position;
}
Prisma::Vector3 Prisma::Transform::GetForward() const {
    return rotation * Vector3(0.0f, 0.0f, 1.0f);
}
void Prisma::Transform::UpdateMatrix() {
    matrix = GetMatrix();  // Changed from matrix = worldMatrix; to avoid const qualifier issues
}
