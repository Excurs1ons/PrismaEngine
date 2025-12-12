#include "TestFramework.h"
#include "ECSTest.cpp"
#include "MathTest.cpp"
#include "ResourceTest.cpp"

int main() {
    return TestFramework::TestRunner::RunAllSuites();
}