#pragma once
#include "CommandLineParser.h"
#include "DynamicLoader.h"
#include "Logger.h"

using InitializeFunc = bool (*)();
using RunFunc        = int (*)(); 
using ShutdownFunc   = void (*)();
using UpdateFunc     = void (*)();