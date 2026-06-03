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
import triton
import triton.language as tl


@triton.jit
def triton_kernel_add(out_ptr0, in_ptr0, in_ptr1, XS: tl.constexpr):
    idx = tl.arange(0, XS)
    tmp0 = tl.load(in_ptr0 + idx)
    tmp1 = tl.load(in_ptr1 + idx)
    tmp2 = tmp0 + tmp1
    tl.store(out_ptr0 + idx, tmp2)


if __name__ == "__main__":
    N = 1024
    x0 = torch.rand((N,), dtype=torch.float32).npu()
    x1 = torch.rand((N,), dtype=torch.float32).npu()
    y0 = torch.empty_like(x0)
    s = torch_npu.npu.Stream()
    with torch_npu.npu.stream(s):
        g = torch_npu.npu.NPUGraph()
        g.capture_begin()
        triton_kernel_add[1, 1, 1](y0, x0, x1, N)
        g.capture_end()
    g.replay()
