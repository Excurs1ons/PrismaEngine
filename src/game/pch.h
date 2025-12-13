#pragma once

// 游戏项目的预编译头文件
// 包含游戏常用的头文件和定义

// 引擎核心头文件（总是需要）
#include "../engine/pch.h"

// 注意：游戏特定的头文件（GameObject、Scene等）不在PCH中包含
// 它们会在各自的源文件中被包含

// C++标准库额外包含（游戏常用）
#include <random>
#include <fstream>
#include <filesystem>
#include <sstream>

// 游戏特定类型
using GameObjectID = uint32;
using ComponentID = uint32;
using EntityID = uint32;

// 注意：游戏特定的枚举、常量和函数
// 将在各自的头文件中定义，不在PCH中包含
// 以避免编译错误和循环依赖