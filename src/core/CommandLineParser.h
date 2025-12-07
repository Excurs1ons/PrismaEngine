#pragma once

#include "Singleton.h"
#include <any>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class CommandLineParser :public Singleton<CommandLineParser>
{
	friend class Singleton<CommandLineParser>;
public:
    // 解析结果枚举
    enum class ParseResult {
        Success,          // 解析成功，继续执行程序
        Error,            // 解析出错
        ActionRequested   // 执行了特定动作后应退出（如显示版本信息等）
    };

    struct Option {
        std::string name;
        std::string shortName;
        std::string description;
        bool requiresValue;
        std::function<void(const std::string&)> callback;
        std::function<bool(const std::string&)> actionCallback; // 返回true表示执行后应退出
        bool isSet = false;
    };

    CommandLineParser();
    ~CommandLineParser() = default;

    // 添加普通选项（不导致程序退出）
    void AddOption(const std::string& name, 
                   const std::string& shortName, 
                   const std::string& description, 
                   bool requiresValue,
                   std::function<void(const std::string&)> callback = nullptr);

    // 添加动作选项（执行后程序应退出）
    void AddActionOption(const std::string& name,
                        const std::string& shortName,
                        const std::string& description,
                        bool requiresValue,
                        std::function<bool(const std::string&)> callback);

    // 解析命令行参数
    ParseResult Parse(int argc, char* argv[]);

    // 检查选项是否被设置
    bool IsOptionSet(const std::string& name) const;

    // 获取选项值
    const std::string& GetOptionValue(const std::string& name) const;

    // 显示帮助信息
    void ShowHelp() const;

    // 获取解析后的剩余参数
    const std::vector<std::string>& GetRemainingArgs() const { return remainingArgs_; }

private:
    std::unordered_map<std::string, Option> options_;
    std::unordered_map<std::string, std::string> optionValues_;
    std::vector<std::string> remainingArgs_;
    std::string programName_;

    Option* FindOption(const std::string& name);
};