#ifndef COMMON_ATTRIBUTE_PARSER_HPP_
#define COMMON_ATTRIBUTE_PARSER_HPP_

#include <map>
#include <string>
#include <algorithm>
#include <strings.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <functional>
#include <optional>

#include "Utils.hpp"

namespace commonapistdoutlogger
{

    struct CaselessCompare
    {
        bool operator()(const std::string& left, const std::string& right) const noexcept
        {
            return (::strcasecmp(left.c_str(), right.c_str()) < 0 );
        }
    };

    std::vector<std::string> extractTokens(const std::string& input, const std::string& separators);
    std::vector<std::string> extractNameAndTokens(const std::string& input, const std::string& separators);

    class Parser
    {
    public:
        class Attribute;
        using Attributes = std::map<std::string, Attribute* const, CaselessCompare>;

        Parser(std::ostream& errorStream);
        void addAttribute(Attribute* attribute);
        void parse(const std::string& inputString);
    private:
        std::ostream& errors;
        Attributes attributes;
    
        std::string trim(const std::string& str);
        std::vector<std::string> splitAndUnquote(const std::string& input);
    };

    class Parser::Attribute
    {
    public:
        explicit Attribute(const std::string& name):name(name){};
        const std::string& getName() const {return name;}
        virtual bool parse(const std::string& input, std::ostream& errors) = 0;
        virtual ~Attribute() = default;

        Attribute(const Attribute&) = delete;
        Attribute(Attribute&&) = delete;
        Attribute& operator=(const Attribute&) = delete;
        Attribute& operator=(Attribute&&) = delete;
    private:
       std::string name;
    };

    template <typename T>
    class ValueSet : public Parser::Attribute
    {
    public:
        using ValueMap = std::unordered_map<std::string, T>;
        ValueSet(const std::string& name, const ValueMap& values): Parser::Attribute(name),values(values),valuesParsed(false){}

        void setExtraEvaluator(const std::function<std::optional<T>(const std::string&)>& evaluator)
        {
            extraEvaluator = evaluator;
        }

        std::optional<T> getValue(const std::string& token) const
        {
            for(const auto& it : values)
            {
                if(isEquals(token, it.first))
                {
                    return it.second;
                }
                if(extraEvaluator)
                {
                    return extraEvaluator(token);
                }else
                {
                    return std::nullopt;
                }
            }
        }

        bool nameExists(const std::string& name) const
        {
            return (values.find(name) != values.end());
        }

        bool given() const
        {
            return valuesParsed;
        }

        std::vector<T> getDefaultSet() const
        {
            std::vector<int> defaultValues;
            for(const auto& it : values)
            {
                defaultValues.push_back(it.second);
            }

            return removeDuplicates(sortValues(defaultValues));
        }

        std::vector<T> getValue() const
        {
            if(!valuesParsed)
            {
                return getDefaultSet();
            }

            return parsedValues;
        }

        
        bool parse(const std::string& input, std::ostream& errors) override
        {
            auto tokens = extractNameAndTokens(input, "|");
            if(tokens.empty() || !isEquals(tokens[0], const_cast<std::string&>(getName())))
            {
                return false;
            }

            if(valuesParsed)
            {
                errors << "ignoring second definition for attribute: " << getName() << " in string: " << input << std::endl;
                return false;
            }

            valuesParsed = true;

            for(size_t i = 0; i < tokens.size(); i++)
            {
                auto val = getValue(tokens[i]);
                if(val)
                {
                    parsedValues.push_back(*val);
                    std::cout << "Added " << tokens[i] << " to attribute " << getName() << " set " << std::endl;
                }else
                {
                    auto range = extractTokens(tokens[i], "-");
                    if (2U != range.size())
                    {
                        errors << "ignoring unexpected token " << tokens[i] << " for attribute " << getName() << std::endl;
                        continue;
                    }

                    auto begin = getValue(range.front());
                    auto end = getValue(range.end());
                    if((begin) && (end))
                    {
                        if(nameExists(range.front()) && nameExists(range.back()))
                        {
                            const auto numericBegin = static_cast<ssize_t>(*begin);
                            const auto numericEnd = static_cast<ssize_t>(*end);
                            for(const auto& it : values)
                            {
                                const auto val(static_cast<ssize_t>(it.second));
                                if ((val >= numericBegin && val <= numericEnd) || (val <= numericBegin && val >= numericEnd))
                                {
                                    parsedValues.push_back(it.second);
                                }
                            }
                            std::cout << "added range " << range.front() << " -> " << range.back() << " to attribute " << getName() << " set" << std::endl;
                        }
                        else
                        {
                            std::cout << "adding raw range " << range.front() << " -> " << range.back()  << " to attribute " << getName() << " set" << std::endl;
                            long x(static_cast<long>(*begin));
                            long y(static_cast<long>(*end));
                            long numericBegin((x < y) ? x : y);
                            long numericEnd((x < y) ? y : x);

                            for(long i = numericBegin; i <= numericEnd; ++i)
                            {
                                parsedValues.push_back(static_cast<T>(i));
                            } 
                        }
                    }else
                    {
                        errors << "ignoring invalid attribute range for attribute " << getName() << " :";
                        if (!begin)
                            errors << " Unknown token " << range.front() << " as range begin.";
                        if (!end)
                            errors << " Unknown token " << range.back() << " as range end.";
                        errors << std::endl;
                    }

                }
            }
            
            parsedValues = removeDuplicates(sortValues(parsedValues));
            return true;
        }

        std::vector<T> getValues() const
        {
            if (!valuesParsed)
                return getDefaultSet();

            return parsedValues;
        }

    private:
        std::vector<T>& sortValues(std::vector<T>& values) const
        {
            std::sort(values.begin(), values.end(), [](const T& left, const T& right)
                      {
                          return (static_cast<ssize_t>(left) < static_cast<ssize_t>(right));
                      });
            return values;
        }

        std::vector<T> removeDuplicates(const std::vector<T>& values) const
        {
            std::vector<T> temp;
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (i == 0 || values[i] != values[i - 1])
                {
                    temp.push_back(values[i]);
                }
            }

            return temp;
        }

        ValueMap values;
        bool valuesParsed;
        std::vector<T> parsedValues;
        std::function<std::optional<T>(const std::string&)> extraEvaluator;
    };

    template<typename T>
    class OneOf : public ValueSet<T>
    {
    public:
      using ValueMap = std::unordered_map<std::string, T>;
      OneOf(const std::string& name, const ValueMap& values):ValueSet<T>(name, values) {}
      bool parse(const std::string& input, std::ostream& errors) override
      {
          if (!(ValueSet<T>::parse(input, errors)))
          {
            return false;
          }

          if(value)
          {
              errors << "duplicate definition for attribute " << ValueSet<T>::getName() << std::endl;
              return false;
          }

          auto tokens = ValueSet<T>::getValues();
          if(1U != tokens.size())
          {
              errors << "definition for " << ValueSet<T>::getName() << " must specify exactly one token: error in " << input << std::endl;
          }else
          {
            value = tokens.front();
          }
          return true;
      }

      const std::optional<T>& get() const noexcept
      {
        return value;
      }

    private:
        std::optional<T> value;
    };
}

#endif
