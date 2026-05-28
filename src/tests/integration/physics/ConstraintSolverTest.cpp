#include <gtest/gtest.h>
#include "physics/ConstraintSolver.h"
#include "physics/Constraint.h"

namespace Prisma {
namespace Physics {
namespace {

// ============================================================================
// ContactConstraint — Default State
// ============================================================================
TEST(ContactConstraintTest, DefaultState) {
    ContactConstraint c;
    EXPECT_EQ(c.bodyA, nullptr);
    EXPECT_EQ(c.bodyB, nullptr);
    EXPECT_FALSE(c.isValid());
    EXPECT_DOUBLE_EQ(c.penetration, 0.0);
    EXPECT_DOUBLE_EQ(c.restitution, 0.0);
    EXPECT_DOUBLE_EQ(c.friction, 0.5);
    EXPECT_DOUBLE_EQ(c.accumulatedNormalImpulse, 0.0);
    EXPECT_DOUBLE_EQ(c.accumulatedTangentImpulse, 0.0);
}

// ============================================================================
// ConstraintSolver — Basic Operations
// ============================================================================
TEST(ConstraintSolverTest, DefaultConstructor) {
    ConstraintSolver solver;
    EXPECT_EQ(solver.m_iterations, 8);
    EXPECT_EQ(solver.getContactCount(), 0);
    EXPECT_EQ(solver.getJointCount(), 0);
}

TEST(ConstraintSolverTest, Clear) {
    ConstraintSolver solver;

    // Add a contact
    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);
    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.5, 0.0);
    contact.penetration = 0.1;
    solver.addContact(contact);
    EXPECT_EQ(solver.getContactCount(), 1);

    solver.clear();
    EXPECT_EQ(solver.getContactCount(), 0);
    EXPECT_EQ(solver.getJointCount(), 0);
}

TEST(ConstraintSolverTest, AddContact) {
    ConstraintSolver solver;
    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);
    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.5, 0.0);
    contact.penetration = 0.1;
    contact.restitution = 0.3;
    solver.addContact(contact);
    EXPECT_EQ(solver.getContactCount(), 1);
}

// ============================================================================
// ConstraintSolver — Basic Solver Execution
// ============================================================================
TEST(ConstraintSolverTest, SolveWithNoContacts) {
    ConstraintSolver solver;
    // Should not crash with empty contacts
    solver.solve(1.0 / 60.0);
    EXPECT_EQ(solver.getContactCount(), 0);
}

TEST(ConstraintSolverTest, SolveWithStaticBodiesOnly) {
    ConstraintSolver solver;
    RigidBody bodyA(RigidBodyType::Static);
    RigidBody bodyB(RigidBodyType::Static);
    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.5, 0.0);
    contact.penetration = 0.1;
    solver.addContact(contact);

    // Should not crash or modify static bodies
    solver.solve(1.0 / 60.0);
    SUCCEED();
}

TEST(ConstraintSolverTest, SolveWithInvalidDt) {
    ConstraintSolver solver;
    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);
    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.5, 0.0);
    contact.penetration = 0.1;
    solver.addContact(contact);

    // dt <= 0 should be handled gracefully
    solver.solve(0.0);
    SUCCEED();
}

// ============================================================================
// ConstraintSolver — Sequential Impulse Convergence
// ============================================================================
TEST(ConstraintSolverTest, SolverConvergesWithContact) {
    ConstraintSolver solver;
    solver.m_iterations = 10;

    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);

    // Body A falls onto body B (ground plane)
    bodyA.setPosition(glm::dvec3(0.0, 5.0, 0.0));
    bodyA.setLinearVelocity(glm::dvec3(0.0, -10.0, 0.0));
    bodyA.setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));

    // Contact normal points from A (dynamic, above) to B (static, ground below) = downward
    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, -1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.0, 0.0);
    contact.penetration = 0.1;
    contact.restitution = 0.0;
    contact.friction = 0.5;

    solver.addContact(contact);
    solver.solve(1.0 / 60.0);

    glm::dvec3 velAfter = bodyA.getLinearVelocity();
    EXPECT_GT(velAfter.y, -10.0) << "Solver should reduce downward velocity";
}

TEST(ConstraintSolverTest, SolverHandlesRestitution) {
    ConstraintSolver solver;
    solver.m_iterations = 10;

    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);

    bodyA.setPosition(glm::dvec3(0.0, 5.0, 0.0));
    bodyA.setLinearVelocity(glm::dvec3(0.0, -10.0, 0.0));

    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, -1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.0, 0.0);
    contact.penetration = 0.1;
    contact.restitution = 0.5;
    contact.friction = 0.5;

    solver.addContact(contact);
    solver.solve(1.0 / 60.0);

    glm::dvec3 velAfter = bodyA.getLinearVelocity();
    EXPECT_GT(velAfter.y, -10.0) << "Solver with restitution should bounce";
}

TEST(ConstraintSolverTest, SolverPreservesTangentialVelocity) {
    ConstraintSolver solver;
    solver.m_iterations = 10;

    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);

    // Body moving diagonally: downward + sideways
    bodyA.setPosition(glm::dvec3(0.0, 5.0, 0.0));
    bodyA.setLinearVelocity(glm::dvec3(3.0, -10.0, 0.0));

    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, -1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.0, 0.0);
    contact.penetration = 0.1;
    contact.restitution = 0.0;
    contact.friction = 0.5;

    solver.addContact(contact);
    solver.solve(1.0 / 60.0);

    glm::dvec3 velAfter = bodyA.getLinearVelocity();
    EXPECT_GT(velAfter.y, -10.0) << "Solver should reduce normal velocity";
    EXPECT_NE(velAfter.x, 0.0) << "Tangential velocity should have remaining component";
}

// ============================================================================
// ConstraintSolver — Multiple Iterations
// ============================================================================
TEST(ConstraintSolverTest, MoreIterationsMoreConvergence) {
    // Test that more iterations converge better (impulse accumulates)
    ConstraintSolver solverHigh;
    solverHigh.m_iterations = 20;

    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);
    bodyA.setLinearVelocity(glm::dvec3(0.0, -10.0, 0.0));

    ContactConstraint contact;
    contact.bodyA = &bodyA;
    contact.bodyB = &bodyB;
    contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact.contactPoint = glm::dvec3(0.0, 0.0, 0.0);
    contact.penetration = 0.1;
    contact.restitution = 0.0;
    contact.friction = 0.5;

    solverHigh.addContact(contact);
    solverHigh.solve(1.0 / 60.0);

    glm::dvec3 velHigh = bodyA.getLinearVelocity();

    // Run a separate solver with fewer iterations
    ConstraintSolver solverLow;
    solverLow.m_iterations = 1;

    RigidBody bodyC(RigidBodyType::Dynamic);
    RigidBody bodyD(RigidBodyType::Static);
    bodyC.setLinearVelocity(glm::dvec3(0.0, -10.0, 0.0));

    ContactConstraint contact2;
    contact2.bodyA = &bodyC;
    contact2.bodyB = &bodyD;
    contact2.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
    contact2.contactPoint = glm::dvec3(0.0, 0.0, 0.0);
    contact2.penetration = 0.1;
    contact2.restitution = 0.0;
    contact2.friction = 0.5;

    solverLow.addContact(contact2);
    solverLow.solve(1.0 / 60.0);

    glm::dvec3 velLow = bodyC.getLinearVelocity();

    // More iterations should produce a larger impulse (less downward velocity)
    EXPECT_GE(velHigh.y, velLow.y) << "More iterations should converge better";
}

// ============================================================================
// Mock Constraint — For testing joint integration
// ============================================================================
class MockConstraint : public IConstraint {
public:
    RigidBody* m_bodyA;
    RigidBody* m_bodyB;
    int solveCount = 0;

    MockConstraint(RigidBody* a, RigidBody* b) : m_bodyA(a), m_bodyB(b) {}

    double Solve(double /*dt*/) override {
        solveCount++;
        return 0.0;
    }

    RigidBody* GetBodyA() const override { return m_bodyA; }
    RigidBody* GetBodyB() const override { return m_bodyB; }
};

TEST(ConstraintSolverTest, SolverCallsJointSolve) {
    ConstraintSolver solver;
    solver.m_iterations = 5;

    RigidBody bodyA(RigidBodyType::Dynamic);
    RigidBody bodyB(RigidBodyType::Static);

    MockConstraint mockJoint(&bodyA, &bodyB);
    solver.addJoint(&mockJoint);

    EXPECT_EQ(solver.getJointCount(), 1);
    EXPECT_EQ(mockJoint.solveCount, 0);

    solver.solve(1.0 / 60.0);

    // Joint solve should be called m_iterations times
    EXPECT_EQ(mockJoint.solveCount, 5);
}

TEST(ConstraintSolverTest, InvalidJointNotAdded) {
    ConstraintSolver solver;

    // Pass nullptr as a joint
    solver.addJoint(nullptr);
    EXPECT_EQ(solver.getJointCount(), 0);
}

} // namespace
} // namespace Physics
} // namespace Prisma
