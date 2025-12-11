#include "CommandLineParser.h"
#include "pch.h"
#include <algorithm>
#include <cstdio>
#include <iostream>

CommandLineParser::CommandLineParser() {
    AddActionOption("help", "h", "显示帮助信息", false, [](const std::string&) {
        CommandLineParser::GetInstance().ShowHelp();
        return true;  // 显示帮助后退出
    });
}

void CommandLineParser::AddOption(const std::string& name,
                                 const std::string& shortName,
                                 const std::string& description,
                                 bool requiresValue,
                                 std::function<void(const std::string&)> callback) {
    Option opt;
    opt.name = name;
    opt.shortName = shortName;
    opt.description = description;
    opt.requiresValue = requiresValue;
    opt.callback = callback;
    opt.actionCallback = nullptr;
    opt.isSet = false;

    options_[name] = opt;
    if (!shortName.empty()) {
        options_[shortName] = opt;
    }
}

void CommandLineParser::AddActionOption(const std::string& name,
                                      const std::string& shortName,
                                      const std::string& description,
                                      bool requiresValue,
                                      std::function<bool(const std::string&)> callback) {
    Option opt;
    opt.name = name;
    opt.shortName = shortName;
    opt.description = description;
    opt.requiresValue = requiresValue;
    opt.callback = nullptr;
    opt.actionCallback = callback;
    opt.isSet = false;

    options_[name] = opt;
    if (!shortName.empty()) {
        options_[shortName] = opt;
    }
}

CommandLineParser::ParseResult CommandLineParser::Parse(int argc, char* argv[]) {
    if (argc <= 0) return ParseResult::Success;

    programName_ = argv[0];
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg.substr(0, 2) == "--") {
            // 处理长选项
            std::string optionName = arg.substr(2);
            std::string value;
            
            // 检查是否有等号分隔的值
            size_t equalPos = optionName.find('=');
            if (equalPos != std::string::npos) {
                value = optionName.substr(equalPos + 1);
                optionName = optionName.substr(0, equalPos);
            }
            
            Option* opt = FindOption(optionName);
            if (!opt) {
                std::cerr << "未知选项: --" << optionName << std::endl;
                return ParseResult::Error;
            }
            
            opt->isSet = true;
            optionValues_[opt->name] = value;
            
            if (opt->requiresValue && value.empty()) {
                // 值应该在下一个参数中
                if (i + 1 >= argc || argv[i+1][0] == '-') {
                    std::cerr << "选项 --" << optionName << " 需要一个值" << std::endl;
                    return ParseResult::Error;
                }
                value = argv[++i];
                optionValues_[opt->name] = value;
            }
            
            // 执行回调
            if (opt->callback) {
                opt->callback(value);
            }
            
            // 执行动作回调（如果有）
            if (opt->actionCallback) {
                bool shouldExit = opt->actionCallback(value);
                if (shouldExit) {
                    return ParseResult::ActionRequested;
                }
            }
        }
        else if (arg.substr(0, 1) == "-" && arg.length() > 1) {
            // 处理短选项
            std::string optionName = arg.substr(1);
            
            Option* opt = FindOption(optionName);
            if (!opt) {
                std::cerr << "未知选项: -" << optionName << std::endl;
                return ParseResult::Error;
            }
            
            opt->isSet = true;
            
            std::string value;
            if (opt->requiresValue) {
                if (i + 1 >= argc || argv[i+1][0] == '-') {
                    std::cerr << "选项 -" << optionName << " 需要一个值" << std::endl;
                    return ParseResult::Error;
                }
                value = argv[++i];
                optionValues_[opt->name] = value;
            } else {
                optionValues_[opt->name] = "";
            }
            
            // 执行回调
            if (opt->callback) {
                opt->callback(value);
            }
            
            // 执行动作回调（如果有）
            if (opt->actionCallback) {
                bool shouldExit = opt->actionCallback(value);
                if (shouldExit) {
                    return ParseResult::ActionRequested;
                }
            }
        }
        else {
            // 剩余参数
            remainingArgs_.push_back(arg);
        }
    }
    return ParseResult::Success;
}

bool CommandLineParser::IsOptionSet(const std::string& name) const {
    auto it = options_.find(name);
    if (it != options_.end()) {
        return it->second.isSet;
    }
    return false;
}

const std::string& CommandLineParser::GetOptionValue(const std::string& name) const {
    static const std::string emptyString;
    auto it = optionValues_.find(name);
    if (it != optionValues_.end()) {
        return it->second;
    }
    return emptyString;
}

void CommandLineParser::ShowHelp() const {
    std::cout << "用法: " << programName_ << " [选项...] [参数...]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    
    // 计算最大选项长度以进行对齐（仅计算长名为键的唯一条目）
    size_t maxOptionLength = 0;
    for (const auto& pair : options_) {
        const Option& opt = pair.second;
        // 只在主键（长选项名）处统计，避免短名重复
        if (pair.first != opt.name) continue;

        std::string optionDisplay;
        if (!opt.shortName.empty()) {
            optionDisplay = "  -" + opt.shortName + ", --" + opt.name;
        } else {
            optionDisplay = "  --" + opt.name;
        }
        maxOptionLength = std::max(maxOptionLength, optionDisplay.length());
    }
    
    for (const auto& pair : options_) {
        const Option& opt = pair.second;
        // 只在主键（长选项名）处打印，避免短名重复
        if (pair.first != opt.name) { // 避免重复显示
            continue;
        }
        
        std::string optionDisplay;
        if (!opt.shortName.empty()) {
            optionDisplay = "  -" + opt.shortName + ", --" + opt.name;
        } else {
            optionDisplay = "  --" + opt.name;
        }
        
        // 对齐描述
        printf("%-*s  %s\n", static_cast<int>(maxOptionLength + 2), optionDisplay.c_str(), opt.description.c_str());
    }
}

CommandLineParser::Option* CommandLineParser::FindOption(const std::string& name) {
    auto it = options_.find(name);
    if (it != options_.end()) {
        return &(it->second);
    }
    return nullptr;
}