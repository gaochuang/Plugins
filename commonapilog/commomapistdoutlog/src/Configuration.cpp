#include "Configuration.hpp"
#include "Utils.hpp"
#include "AttributeParser.hpp"

#include <unordered_map>
#include <optional>

namespace commonapistdoutlogger
{

    SyslogLevels getSyslogLevels()
    {
        return SyslogLevels{{LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG}};
    }

    SyslogFacilities getSyslogFacilities()
    {
        return SyslogFacilities{{LOG_AUTH, LOG_AUTHPRIV, LOG_CRON, LOG_DAEMON, LOG_FTP, LOG_LOCAL0, LOG_LOCAL1, LOG_LOCAL2, LOG_LOCAL3, LOG_LOCAL4,
                                LOG_LOCAL5, LOG_LOCAL6, LOG_LOCAL7, LOG_LPR, LOG_MAIL, LOG_NEWS, LOG_SYSLOG, LOG_USER, LOG_UUCP}};
    }

    const std::unordered_map<std::string, int> syslogLevelNames = 
    {
        {"emerg", LOG_EMERG}, 	  /* system is unusable */
        {"emergency",LOG_EMERG}, /* system is unusable */
        {"alert", LOG_ALERT},    /* action must be taken immediately */
        {"critical", LOG_CRIT},  /* critical conditions */
        {"err", LOG_ERR},        /*error conditions */
        {"error", LOG_ERR},      /*error conditions */
        {"warn", LOG_WARNING},    /* warning conditions */
        {"warning", LOG_WARNING},  /* warning conditions */
        {"notice", LOG_NOTICE},  /* normal but significant condition */
        {"info", LOG_INFO}, 	/* informational */
        {"debug", LOG_DEBUG}   /* debug-level messages */ 
    };

    const std::unordered_map<std::string, int> syslogFacilityName = 
    {
        {"auth", LOG_AUTH}, /* security/authorization messages */
        {"security", LOG_AUTH}, /* security/authorization messages */
        {"authpriv", LOG_AUTHPRIV}, /* security/authorization messages (private) */
        {"cron", LOG_CONS}, /* log on the console if errors in sending */
        {"daemon", LOG_DAEMON}, /* system daemons */
        {"local0", LOG_LOCAL0}, /* reserved for local use */
        {"local1", LOG_LOCAL1}, /* reserved for local use */
        {"local2", LOG_LOCAL2}, /* reserved for local use */
        {"local3", LOG_LOCAL3}, /* reserved for local use */
        {"local4", LOG_LOCAL4}, /* reserved for local use */
        {"local5", LOG_LOCAL5}, /* reserved for local use */
        {"local6", LOG_LOCAL6}, /* reserved for local use */
        {"local7", LOG_LOCAL7}, /* reserved for local use */
        {"ftp",    LOG_FTP},    /* ftp daemon */
        {"lpr",    LOG_LPR},    /* line printer subsystem */
        {"mail",   LOG_MAIL},   /* mail system */
        {"news",   LOG_NEWS},   /* network news subsystem */
        {"syslog", LOG_SYSLOG}, /* messages generated internally by syslogd */
        {"user",   LOG_USER},   /* random user-level messages */
        {"uucp",   LOG_UUCP}    /* UUCP subsystem */
    };

    std::optional<int> calculateLevel(const std::string& str) noexcept
    {
        int level(0);

        if((stringToInt(str, level)) && (level >= 0) && (level <=7))
        {
            return level;
        }

        return std::nullopt;
    }

    std::optional<int> calculateFacility(const std::string& str) noexcept
    {
        int facility(0);

        if(stringToInt(str, facility) && (facility >= 0 ) && (facility <= 23))
        {
            return facility << 3;
        }

        return std::nullopt;
    }

    Configuration getConfiguration(std::ostream& errors)
    {
        auto configStr = ::getenv("COMMON_API_STDOUT_LOGGER_ATTRS");
        if(nullptr == configStr)
        {
            return {};
        }

        ValueSet<int> syslogLevels{"level", syslogLevelNames};
        syslogLevels.setExtraEvaluator(calculateLevel);

        ValueSet<int> syslogFacilities{"facility", syslogFacilityName};
        syslogFacilities.setExtraEvaluator(calculateFacility);

        ValueSet<int> excludedSyslogFacilities{"excludeFacility", syslogFacilityName};
        excludedSyslogFacilities.setExtraEvaluator(calculateFacility);

        OneOf<int> minErrLevel{"minErrLevel", syslogLevelNames};

        Parser parser(errors);

        parser.addAttribute(&syslogLevels);
        parser.addAttribute(&syslogFacilities);
        parser.addAttribute(&excludedSyslogFacilities);
        parser.addAttribute(&minErrLevel);

        parser.parse(configStr);

        int errLevel = LOG_ERR;
        
        const auto& optMinErrLevel = minErrLevel.get();
        if(optMinErrLevel)
        {
            errLevel = *optMinErrLevel;
        }

        std::vector<int> facilityList;
        if(!excludedSyslogFacilities.given())
        {
            facilityList = syslogFacilities.getValues();
        }else
        {
            auto includeList = syslogFacilities.getValues();
            auto excludeList = excludedSyslogFacilities.getValues();
            std::set_difference(includeList.begin(), includeList.end(), excludeList.begin(), excludeList.end(), std::back_inserter(facilityList));
        }

        return {syslogLevels.getValues(), facilityList, errLevel};
    }
}
