#ifndef COMMON_API_ATTRIBUTE_PARSER_HPP_
#define COMMON_API_ATTRIBUTE_PARSER_HPP_

#include "AttributeParser.hpp"

#include <algorithm>


using namespace commonapistdoutlogger;

namespace commonapistdoutlogger
{

    //// 分割函数：用于按照分隔符分割字符串
    std::vector<std::string> extractTokens(const std::string& input, const std::string& separators)
    {
        std::vector<std::string> tokens;
        std::string::size_type start(0);

        auto end = input.find_first_of(separators);

        while (end != std::string::npos)
        {
            if(end > start)
            {
                tokens.push_back(input.substr(start, end - start));
            }

            start = end + 1; //更新开始位置，跳过当前的分隔符
            end = input.find_first_of(separators, start);
        }

        if(start < input.length())
        {
            tokens.push_back(input.substr(start)); //添加最后一个子字符串
        }

        return tokens;
    }

    std::vector<std::string> extractNameAndTokens(const std::string& input, const std::string& separators)
    {
        std::vector<std::string> nameAndTokens;
        if(input.empty())
        {
            return nameAndTokens;
        }

        auto pos = input.find_first_of('=');
        if(std::string::npos == pos)
        {
            std::string trimmedInput = input;
            trimmedInput.erase(trimmedInput.begin(), std::find_if(trimmedInput.begin(), trimmedInput.end(), [](unsigned char c) { return !std::isspace(c); })); // 去除开头空白
            trimmedInput.erase(std::find_if(trimmedInput.rbegin(), trimmedInput.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), trimmedInput.end()); // 去除结尾空白
            nameAndTokens.push_back(trimmedInput);
            return nameAndTokens;
        }

        // 提取 name，去除前后的空白
        std::string name = input.substr(0, pos);
        name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](unsigned char c) { return !std::isspace(c); })); // 去除开头空白
        name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), name.end()); // 去除结尾空白
        nameAndTokens.push_back(name);

        if(pos < input.size() - 1)
        {
            std::string remaining = input.substr(pos + 1); // 获取 '=' 后面的内容
            if(separators.empty())
            {
                // 如果没有指定分隔符，直接添加去空格后的内容
                remaining.erase(remaining.begin(), std::find_if(remaining.begin(), remaining.end(), [](unsigned char c) { return !std::isspace(c); })); // 去除开头空白
                remaining.erase(std::find_if(remaining.rbegin(), remaining.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), remaining.end()); // 去除结尾空白
                nameAndTokens.push_back(remaining);
            }else
            {
                for (const auto& token : extractTokens(remaining, separators))
                {
                    std::string trimmedToken = token;
                    trimmedToken.erase(trimmedToken.begin(), std::find_if(trimmedToken.begin(), trimmedToken.end(), [](unsigned char c) { return !std::isspace(c); })); // 去除开头空白
                    trimmedToken.erase(std::find_if(trimmedToken.rbegin(), trimmedToken.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), trimmedToken.end()); // 去除结尾空白
                    nameAndTokens.push_back(trimmedToken);
                }
            }
        }

        return nameAndTokens;
    }
}

Parser::Parser(std::ostream& errorStream):errors(errorStream)
{
}

void Parser::addAttribute(Attribute* attribute)
{
    const auto& name = attribute->getName();
    auto ret = attributes.insert(std::make_pair(name, attribute));
    if (!ret.second)
    {
        std::ostringstream oss;
        oss << "duplicate attribute name: " << name << std::endl;
        throw std::runtime_error(oss.str());
    }
}

//去除字符串中空格
std::string Parser::trim(const std::string& str) 
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) start++;
    auto end = str.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

// 解析带引号的字符串或普通字符串
std::vector<std::string> Parser::splitAndUnquote(const std::string& input) 
{
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    char ch;

    while (ss >> std::ws) {
        if (ss.peek() == '\'' || ss.peek() == '"') {
            char quote = ss.get();  // 读取引号
            token.clear();
            while (ss.get(ch)) {
                if (ch == '\\') {  // 处理转义字符
                    if (ss.peek() == quote) {
                        token += ss.get();  // 转义引号
                    } else {
                        token += ch;  // 普通的转义
                        token += ss.get();
                    }
                } else if (ch == quote) {
                    break;  // 结束当前引号的字符串
                } else {
                    token += ch;
                }
            }
            tokens.push_back(token);
        } else {
            std::getline(ss, token, ',');  // 普通逗号分隔符
            tokens.push_back(trim(token));  // 去除前后空格
        }
    }
    return tokens;
}

void Parser::parse(const std::string& input)
{
    auto tokens = splitAndUnquote(input); // 分割和去除引号的字符串

    for(const auto& it : tokens)
    {
        bool found = false;
        for (auto& attr : attributes)
        {
            const std::string trimmedInput = trim(it);
            if(attr.second->parse(trimmedInput, errors))
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            errors << "Ignoring unknown attribute " << it << std::endl;
        }
    }
}

#endif
