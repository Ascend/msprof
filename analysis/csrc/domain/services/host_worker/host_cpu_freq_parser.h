#ifndef ANALYSIS_DOMAIN_HOST_CPU_FREQ_PARSER_H
#define ANALYSIS_DOMAIN_HOST_CPU_FREQ_PARSER_H

#include <cstdint>
#include <string>

namespace Analysis
{
namespace Domain
{
class HostCpuFreqParser
{
   public:
    explicit HostCpuFreqParser(const std::string& hostPath) : hostPath_(hostPath) {}
    uint32_t Run() const;

   private:
    std::string hostPath_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_CPU_FREQ_PARSER_H
