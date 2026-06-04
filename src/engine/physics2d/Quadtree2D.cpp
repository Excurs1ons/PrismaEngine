#include "Quadtree2D.h"
#include <algorithm>

namespace Prisma::Physics2D {

Quadtree2D::Quadtree2D(const AABB2D& bounds, uint32_t maxDepth, uint32_t maxElements)
    : m_bounds(bounds)
    , m_maxDepth(maxDepth)
    , m_maxElements(maxElements)
{
    m_nodes.reserve(256);
    m_nodes.push_back({bounds, {}, {0, 0, 0, 0}, true});
}

uint32_t Quadtree2D::CreateNode(const AABB2D& bounds)
{
    uint32_t index = static_cast<uint32_t>(m_nodes.size());
    m_nodes.push_back({bounds, {}, {0, 0, 0, 0}, true});
    return index;
}

void Quadtree2D::SplitNode(uint32_t nodeIndex)
{
    Node& node = m_nodes[nodeIndex];
    if (!node.isLeaf) return;

    const AABB2D& b = node.bounds;
    float midX = (b.minX + b.maxX) * 0.5f;
    float midY = (b.minY + b.maxY) * 0.5f;

    // child[0]: TL, child[1]: TR, child[2]: BL, child[3]: BR
    node.children[0] = CreateNode(AABB2D(b.minX, midY, midX, b.maxY));
    node.children[1] = CreateNode(AABB2D(midX,  midY, b.maxX, b.maxY));
    node.children[2] = CreateNode(AABB2D(b.minX, b.minY, midX, midY));
    node.children[3] = CreateNode(AABB2D(midX,  b.minY, b.maxX, midY));
    node.isLeaf = false;

    std::vector<uint32_t> remaining;
    remaining.reserve(node.elements.size());

    for (uint32_t entityId : node.elements) {
        auto it = m_entityMap.find(entityId);
        if (it == m_entityMap.end()) continue;

        const AABB2D& aabb = it->second;
        int quad = GetQuadrant(b, aabb);
        if (quad >= 0 && quad < 4 && FitsInChild(b, aabb)) {
            m_nodes[node.children[quad]].elements.push_back(entityId);
        } else {
            remaining.push_back(entityId);
        }
    }

    node.elements.swap(remaining);
}

void Quadtree2D::Insert(uint32_t entityId, const AABB2D& aabb)
{
    if (m_entityMap.find(entityId) != m_entityMap.end()) {
        Remove(entityId);
    }
    m_entityMap[entityId] = aabb;
    InsertInternal(0, entityId, aabb, 0);
}

void Quadtree2D::InsertInternal(uint32_t nodeIndex, uint32_t entityId, const AABB2D& aabb, uint32_t depth)
{
    Node& node = m_nodes[nodeIndex];

    if (node.isLeaf) {
        node.elements.push_back(entityId);
        if (node.elements.size() > m_maxElements && depth < m_maxDepth) {
            SplitNode(nodeIndex);
        }
        return;
    }

    int quad = GetQuadrant(node.bounds, aabb);
    if (quad >= 0 && quad < 4 && FitsInChild(node.bounds, aabb)) {
        InsertInternal(node.children[quad], entityId, aabb, depth + 1);
    } else {
        node.elements.push_back(entityId);
    }
}

bool Quadtree2D::RemoveEntityFromNode(uint32_t nodeIndex, uint32_t entityId)
{
    Node& node = m_nodes[nodeIndex];
    bool found = false;

    auto& elements = node.elements;
    for (size_t i = 0; i < elements.size(); ) {
        if (elements[i] == entityId) {
            elements[i] = elements.back();
            elements.pop_back();
            found = true;
            break;
        }
        ++i;
    }

    if (!node.isLeaf) {
        for (int i = 0; i < 4; ++i) {
            if (node.children[i] != 0) {
                found |= RemoveEntityFromNode(node.children[i], entityId);
            }
        }
    }

    return found;
}

void Quadtree2D::Remove(uint32_t entityId)
{
    auto it = m_entityMap.find(entityId);
    if (it == m_entityMap.end()) return;

    RemoveEntityFromNode(0, entityId);
    m_entityMap.erase(it);
}

void Quadtree2D::Update(uint32_t entityId, const AABB2D& newAABB)
{
    Remove(entityId);
    Insert(entityId, newAABB);
}

std::vector<uint32_t> Quadtree2D::Query(const AABB2D& range) const
{
    std::vector<uint32_t> result;
    result.reserve(64);
    QueryInternal(0, range, result);
    return result;
}

void Quadtree2D::QueryInternal(uint32_t nodeIndex, const AABB2D& range, std::vector<uint32_t>& result) const
{
    const Node& node = m_nodes[nodeIndex];
    if (!node.bounds.Intersects(range)) return;

    for (uint32_t entityId : node.elements) {
        auto it = m_entityMap.find(entityId);
        if (it != m_entityMap.end() && it->second.Intersects(range)) {
            result.push_back(entityId);
        }
    }

    if (!node.isLeaf) {
        for (int i = 0; i < 4; ++i) {
            if (node.children[i] != 0) {
                QueryInternal(node.children[i], range, result);
            }
        }
    }
}

int Quadtree2D::GetQuadrant(const AABB2D& nodeBounds, const AABB2D& elementAABB) const
{
    float midX = (nodeBounds.minX + nodeBounds.maxX) * 0.5f;
    float midY = (nodeBounds.minY + nodeBounds.maxY) * 0.5f;

    bool left  = elementAABB.maxX <= midX;
    bool right = elementAABB.minX >= midX;
    bool top   = elementAABB.minY >= midY;
    bool bot   = elementAABB.maxY <= midY;

    if ((left && right) || (top && bot)) return -1;

    if (top  && left)  return 0;
    if (top  && right) return 1;
    if (bot  && left)  return 2;
    if (bot  && right) return 3;

    return -1;
}

bool Quadtree2D::FitsInChild(const AABB2D& nodeBounds, const AABB2D& elementAABB) const
{
    float midX = (nodeBounds.minX + nodeBounds.maxX) * 0.5f;
    float midY = (nodeBounds.minY + nodeBounds.maxY) * 0.5f;

    bool crossesMidX = elementAABB.minX < midX && elementAABB.maxX > midX;
    bool crossesMidY = elementAABB.minY < midY && elementAABB.maxY > midY;

    return !crossesMidX && !crossesMidY;
}

void Quadtree2D::Clear()
{
    m_nodes.clear();
    m_entityMap.clear();
    m_nodes.push_back({m_bounds, {}, {0, 0, 0, 0}, true});
}

uint32_t Quadtree2D::GetTotalElementCount() const
{
    return GetElementCountInternal(0);
}

uint32_t Quadtree2D::GetElementCountInternal(uint32_t nodeIndex) const
{
    const Node& node = m_nodes[nodeIndex];
    uint32_t count = static_cast<uint32_t>(node.elements.size());

    if (!node.isLeaf) {
        for (int i = 0; i < 4; ++i) {
            if (node.children[i] != 0) {
                count += GetElementCountInternal(node.children[i]);
            }
        }
    }

    return count;
}

} // namespace Prisma::Physics2D
