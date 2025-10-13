#pragma once

#include <initializer_list>
#include <string>
#include <functional>
#include <format>
#include <iostream>

enum class ParseResults {
    kOkay,
    kNoArguments,
    kHelpUsed,
    kUnregisteredOption,
    kUnfulfilledOptions,
};

enum class OptionType {
    kHelper,
    kOptional,
    kRequired
};

class ArgumentParser {
public:
    ArgumentParser() = default;

    ArgumentParser(ArgumentParser&&) = delete;
    ArgumentParser(const ArgumentParser&) = delete;
    void operator=(ArgumentParser&&) = delete;
    void operator=(const ArgumentParser&) = delete;

    void registerHandler(std::initializer_list<std::string>, std::function<void(std::string&)>&&, OptionType = OptionType::kOptional) noexcept;

    ParseResults parse(char** argv, int argc) noexcept;

private:
    struct CallbackDescriptor {
        CallbackDescriptor() = default;
        CallbackDescriptor(std::function<void(std::string& args)>&&, OptionType);
        std::function<void(std::string& args)> callback;
        OptionType optionType;
        bool consumed;
    };

    // Maps options to functional callbacks
    std::unordered_map<std::string, std::size_t> callbackMapper;

    std::vector<CallbackDescriptor> callbackDescriptors;

    std::size_t nRequiredOptions;
};