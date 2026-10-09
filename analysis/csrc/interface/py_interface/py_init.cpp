/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <dictobject.h>

#include <string>

#include "analysis/csrc/interface/py_interface/py_init_parser.h"

#ifdef ASCEND_CI_LIMITED_PY37
#undef PyCFunction_NewEx
#endif

namespace Analysis
{
namespace Interface
{
static PyMethodDef g_moduleMethods[] = {{nullptr, nullptr, METH_VARARGS, ""}};

static PyModuleDef g_mylibMethods = {
    PyModuleDef_HEAD_INIT,
    "msprof_analysis",  // name of module
    "",                 // module documentation, may be NULL
    -1,  // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    g_moduleMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr};

static void AddSubModule(PyObject *root, const char *name, PyMethodDef *methods)
{
    PyObject *d = PyModule_GetDict(root);
    std::string subModuleName = name;
    std::string moduleName = "msprof_analysis." + subModuleName;
    PyObject *subModule = PyDict_GetItemString(d, name);
    if (subModule == nullptr)
    {
        subModule = PyImport_AddModule(moduleName.c_str());
        PyDict_SetItemString(d, name, subModule);
        Py_XDECREF(subModule);
    }
    // populate module's dict
    d = PyModule_GetDict(subModule);
    for (PyMethodDef *m = methods; m->ml_name != nullptr; ++m)
    {
        PyObject *methodObj = PyCFunction_NewEx(m, nullptr, nullptr);
        PyDict_SetItemString(d, m->ml_name, methodObj);
        Py_XDECREF(methodObj);
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
    PyMODINIT_FUNC PyInit_msprof_analysis(void)
    {
        PyObject *m = PyModule_Create(&g_mylibMethods);
        AddSubModule(m, "parser", GetParserMethods());
        return m;
    }
    __attribute__((visibility("default"))) PyObject *PyInit_msprof_analysis(void);
#ifdef __cplusplus
}
#endif
}  // namespace Interface
}  // namespace Analysis
