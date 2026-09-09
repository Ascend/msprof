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

#ifndef ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H
#define ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H

#include <Python.h>
namespace Analysis
{
namespace Interface
{
// 该方法供注册python接口使用，用于收集PyMethodDef供注册时挂载module
// 挂载接口格式如下： Get{模块名}Methods
PyMethodDef *GetParserMethods();

// 解析落盘CANN数据功能的外层包装，解析python侧传入的路径后调用KernelParserWorker启动解析流程，获取返回状态码后返回python侧
PyObject *WrapDumpCANNTrace(PyObject *self, PyObject *args);
// 解析Device侧数据功能的外层包装，解析python侧传入的路径后调用DeviceContextEntry启动解析流程，获取返回状态码后返回python侧
PyObject *WrapDumpDeviceData(PyObject *self, PyObject *args);
// 统一db入口功能的外层包装，解析python侧传入的路径后调用unifiedDbManager启动导出流程，获取返回状态码后返回python侧
PyObject *WrapExportUnifiedDB(PyObject *self, PyObject *args);
// timeline导出入口的外层包装，解析Python侧传入路径后调用exportManager启动导出流程，获取返回状态码后返回python侧
PyObject *WrapExportTimeline(PyObject *self, PyObject *args);
// op_summary导出入口的外层包装，解析Python侧传入路径后调用exportSummary启动导出流程，获取返回状态码后返回Python侧
PyObject *WrapExportSummary(PyObject *self, PyObject *args);
// 一键串联C化处理流程，按flags顺序执行host、device及各导出阶段并返回状态码
PyObject *WrapRunPipeline(PyObject *self, PyObject *args);
}  // namespace Interface
}  // namespace Analysis

#endif  // ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H
