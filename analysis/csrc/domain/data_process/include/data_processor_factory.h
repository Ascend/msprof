/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H
#define ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H

#include <memory>
#include <new>
#include <string>
#include <typeindex>

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_struct.h"
#include "analysis/csrc/infrastructure/process/include/topo_graph.h"

namespace Analysis
{
namespace Domain
{
class DataProcessorProcess final : public Infra::Process
{
   public:
    DataProcessorProcess(std::shared_ptr<DataProcessor> processor, std::string name)
        : processor_(std::move(processor)), name_(std::move(name))
    {
    }

    const std::shared_ptr<DataProcessor>& GetProcessor() const { return processor_; }

   private:
    uint32_t ProcessEntry(Infra::DataInventory& inventory, const Infra::Context&) override;

    std::shared_ptr<DataProcessor> processor_;
    std::string name_;
};

template <typename Processor>
Application::TopoNodeCreatorFactory CreateDataProcessorFactory(const std::string& processName)
{
    return [processName](const Application::TopoBuildContext& context)
    {
        const std::string profPath = context.profPath;
        return [processName, profPath]() -> std::unique_ptr<Infra::Process>
        {
            std::shared_ptr<DataProcessor> processor(new (std::nothrow) Processor(profPath));
            if (processor == nullptr)
            {
                return nullptr;
            }
            return std::unique_ptr<Infra::Process>(new (std::nothrow) DataProcessorProcess(processor, processName));
        };
    };
}

}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H
