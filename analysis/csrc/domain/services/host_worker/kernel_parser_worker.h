#ifndef ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H
#define ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H

#include <atomic>
#include <string>

namespace Analysis
{
namespace Domain
{
// 解析流程控制类，传入host data所在路径，启动采集流程并返回结果
class KernelParserWorker
{
   public:
    // 初始化传入host路径
    explicit KernelParserWorker(std::string hostFilePath);
    // 启动流程
    int Run();

   private:
    // hashData解析落盘
    void DumpHashData();
    // typeInfo解析落盘
    void DumpTypeInfoData();
    // NPU算子内存解析、计算及落盘
    void ProcessNpuOpMemData();
    // 启动CANN侧数据解析及分析流程
    void LaunchTraceParser();

    std::string hostFilePath_;
    // 用于子线程上报执行结果
    std::atomic<bool> result_;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H
