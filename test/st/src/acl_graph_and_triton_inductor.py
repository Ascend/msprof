# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import torch
import torch_npu


def torch_add(a, b):
    return a + b


if __name__ == "__main__":
    inductor_add = torch.compile(torch_add)
    N = 1024
    x0 = torch.rand((N,), dtype=torch.float32).npu()
    x1 = torch.rand((N,), dtype=torch.float32).npu()
    inductor_add(x0, x1)
    s = torch_npu.npu.Stream()
    with torch_npu.npu.stream(s):
        g = torch_npu.npu.NPUGraph()
        g.capture_begin()
        inductor_add(x0, x1)
        g.capture_end()
    g.replay()
