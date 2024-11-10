#ifndef COMMON_API_CONFIGURATION_HPP_
#define COMMON_API_CONFIGURATION_HPP_

#include <vector>
#include <sstream>
#include <syslog.h>

namespace commonapistdoutlogger
{
   
   using SyslogLevels = std::vector<int>;
   using SyslogFacilities = std::vector<int>;

   SyslogLevels getSyslogLevels();
   SyslogFacilities getSyslogFacilities();

   struct Configuration
   {
      SyslogLevels includeLevels;
      SyslogFacilities includeFacilities;

      int minErrLevel;

      Configuration(): includeLevels(getSyslogLevels()), includeFacilities(getSyslogFacilities()), minErrLevel(LOG_ERR)
      {
      }

      Configuration(const SyslogLevels& levels, const SyslogFacilities& facilities, int minErrLevel):
                   includeLevels(levels), includeFacilities(facilities), minErrLevel(minErrLevel)
      {

      }

   };

   Configuration getConfiguration(std::ostream& errors);                            
}

#endif
