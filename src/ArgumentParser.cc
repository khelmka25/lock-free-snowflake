#include "ArgumentParser.h"

void ArgumentParser::registerHandler(std::initializer_list<std::string> options, std::function<void(std::string&)>&& callback, OptionType optionType) noexcept {
    for (auto& item : options) {
        callbackMapper[item] = callbackDescriptors.size();
    }

    callbackDescriptors.emplace_back(std::move(callback), optionType);

    if (optionType == OptionType::kRequired) {
        nRequiredOptions++;
    }
}

ParseResults ArgumentParser::parse(char** argv, int argc) noexcept {
    if (argc <= 1) {
        std::cout << "Invalid usage. Use -h for help" << std::endl;
        return ParseResults::kNoArguments;
    }
    
    std::vector<std::string> tokens;
    // ignore first argument
    for (int i = 1; i < argc; i++) {
        tokens.emplace_back(argv[i]);
    }

    // parse tokens: flags, optional of argument
    std::unordered_map<std::string, std::string> commands;
    std::string command{};

    bool commandHasArguments = true;

    for (std::size_t i = 0; i < tokens.size(); i++) {
        std::string& token = tokens[i];

        std::size_t start = token.find_first_of('-');
        if (start != std::string::npos) /*This is an option*/ {
            if (!commandHasArguments) {
                commands[command] = "";
            }
            // this must be the start of an argument
            command = token;
            commandHasArguments = false;
        }
        else /*This is an arg*/ {
            commandHasArguments = true;
            // must be an argument
            commands[command] = token;
        }
    }

    if (!commandHasArguments) {
        commands[command] = "";
    }

    for (auto& [option, args] : commands) {
        if (callbackMapper.find(option) != callbackMapper.end()) {
            std::size_t index = callbackMapper[option];
            auto& descriptor = callbackDescriptors.at(index);

            /*Already consumed option?*/
            if (descriptor.consumed) {
                std::cout << std::format("Duplicate option: {}, args: {}", option, args) << std::endl;
                continue;
            }

            descriptor.callback(args);
            descriptor.consumed = true;

            if (descriptor.optionType == OptionType::kHelper) {
                return ParseResults::kHelpUsed;
            }

            if (descriptor.optionType == OptionType::kRequired) {
                nRequiredOptions--;
            }
        }
        else {
            std::cout << std::format("Invalid or unregistered option: {}, args: {}", option, args) << std::endl;
            return ParseResults::kUnregisteredOption;
        }
    }

    if (nRequiredOptions != 0ull) {
        std::cout << std::format("Unfilled required options:") << std::endl;
        for (auto& [option, args] : commands) {
            if (callbackMapper.find(option) != callbackMapper.end()) {
                std::size_t index = callbackMapper[option];
                auto& descriptor = callbackDescriptors.at(index);
                if (descriptor.optionType == OptionType::kRequired && !descriptor.consumed) {
                    std::cout << std::format("option: {}, args: {}", option, args) << std::endl;
                }
            }
        }

        return ParseResults::kUnfulfilledOptions;
    }

    return ParseResults::kOkay;
}

ArgumentParser::CallbackDescriptor::CallbackDescriptor(std::function<void(std::string& args)>&& t_callback, OptionType t_optionType) {
    callback = std::move(t_callback);
    optionType = t_optionType;
}


