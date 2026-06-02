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
import triton
import triton.language as tl


def test_add(input0, input1):
    def torch_func(a, b):
        return a + b

    @triton.jit
    def triton_kernel_add(out_ptr0, in_ptr0, in_ptr1, XS: tl.constexpr):
        # pylint: disable=duplicate-code
        idx = tl.arange(0, XS)
        tmp0 = tl.load(in_ptr0 + idx)
        tmp1 = tl.load(in_ptr1 + idx)
        tmp2 = tmp0 + tmp1
        tl.store(out_ptr0 + idx, tmp2)

    def triton_func(a, b):
        n = a.shape[0]
        y0 = torch.empty_like(a)
        triton_kernel_add[(1,)](y0, a, b, n)
        return y0

    torch_ref = torch_func(input0, input1)
    triton_cal = triton_func(input0, input1)
    torch.testing.assert_close(triton_cal, torch_ref)


def test_or(input0, input1):
    def torch_func(a, b):
        return a | b

    @triton.jit
    def triton_kernel_or(out_ptr0, in_ptr0, in_ptr1, XS: tl.constexpr):
        idx = tl.arange(0, XS)
        tmp0 = tl.load(in_ptr0 + idx)
        tmp1 = tl.load(in_ptr1 + idx)
        tmp2 = tmp0 | tmp1
        tl.store(out_ptr0 + idx, tmp2)

    def triton_func(a, b):
        n = a.shape[0]
        y0 = torch.empty_like(a)
        triton_kernel_or[(1,)](y0, a, b, n)
        return y0

    torch_ref = torch_func(input0, input1)
    triton_cal = triton_func(input0, input1)
    torch.testing.assert_close(triton_cal, torch_ref)


def test_inductor_add(a, b):
    def torch_add(x, y):
        res = x + y
        return res

    compiled_func = torch.compile(torch_add)
    compiled_func(a, b)


if __name__ == "__main__":
    test_case_is_inductor = False
    N = 1024
    low = 1
    high = 100

    # float32
    x0_fp32 = torch.rand((N,), dtype=torch.float32).npu()
    x1_fp32 = torch.rand((N,), dtype=torch.float32).npu()

    # float16
    x0_fp16 = torch.rand((N,), dtype=torch.float16).npu()
    x1_fp16 = torch.rand((N,), dtype=torch.float16).npu()

    # bfloat16
    x0_bf16 = torch.rand((N,), dtype=torch.bfloat16).npu()
    x1_bf16 = torch.rand((N,), dtype=torch.bfloat16).npu()

    # int64
    x0_i64 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int64).npu()
    x1_i64 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int64).npu()

    # int32
    x0_i32 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int32).npu()
    x1_i32 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int32).npu()

    # int16
    x0_i16 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int16).npu()
    x1_i16 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int16).npu()

    # int8
    x0_i8 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int8).npu()
    x1_i8 = torch.randint(low=low, high=high, size=(N,), dtype=torch.int8).npu()

    # bool (i1)
    x0_i1 = torch.randint(low=0, high=2, size=(N,)).bool().npu()
    x1_i1 = torch.randint(low=0, high=2, size=(N,)).bool().npu()

    test_cases = [
        ('fp32', x0_fp32, x1_fp32),
        ('fp16', x0_fp16, x1_fp16),
        ('bf16', x0_bf16, x1_bf16),
        ('i64', x0_i64, x1_i64),
        ('i32', x0_i32, x1_i32),
        ('i16', x0_i16, x1_i16),
        ('i8', x0_i8, x1_i8),
        ('i1', x0_i1, x1_i1),
    ]

    for dtype_name, x0, x1 in test_cases:
        print(f"Running test for {dtype_name}...")
        if dtype_name != 'i1':
            test_inductor_add(x0, x1)
            test_add(x0, x1)
        else:
            test_or(x0, x1)
