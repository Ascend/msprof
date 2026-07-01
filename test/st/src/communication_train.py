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
import os
import socket
from itertools import product

import torch
import torch_npu
from torch import distributed as dist
from torch import multiprocessing as mp

MASTER_PORT = '25000'
DEV_NUM = 2
SERVER_NUM = 1
SERVER_INDEX = 0
DATA_LIST = [1024]
DTYPE = 'int8'
OPTYPE = 'sum'
MUL_NUM = 10


DTYPE_MAP = {
    'int8': [torch.int8],
    'int32': [torch.int32],
    'float32': [torch.float32],
    'float16': [torch.float16],
    'int64': [torch.int64],
    'bfloat16': [torch.bfloat16],
    'all': [torch.int8, torch.int32, torch.float32, torch.float16, torch.int64, torch.bfloat16],
}

OP_MAP = {
    'sum': [dist.ReduceOp.SUM],
    'max': [dist.ReduceOp.MAX],
    'min': [dist.ReduceOp.MIN],
    'prod': [dist.ReduceOp.PRODUCT],
    'all': [dist.ReduceOp.SUM, dist.ReduceOp.MAX, dist.ReduceOp.MIN, dist.ReduceOp.PRODUCT],
}


def get_host_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    finally:
        s.close()


def check_tensor(tensor, op, world_size, data, dtype):
    tensor = tensor.cpu()
    if op == dist.ReduceOp.SUM:
        expected = torch.full(
            (1, data), 2 * world_size if dtype == torch.bfloat16 else sum(range(1, world_size + 1)), dtype=dtype
        )
    elif op == dist.ReduceOp.MAX:
        expected = torch.full((1, data), world_size, dtype=dtype)
    elif op == dist.ReduceOp.MIN:
        expected = torch.full((1, data), 1, dtype=dtype)
    elif op == dist.ReduceOp.PRODUCT:
        expected = torch.full((1, data), __import__('math').prod(range(1, world_size + 1)), dtype=dtype)
    else:
        return
    assert (tensor == expected).all() or (torch.isinf(expected).all() and torch.isinf(tensor).all()), (
        f"{op} check failed: got {tensor}"
    )


def run_single_case(rank, data, dtype, op, loop):
    if op == dist.ReduceOp.PRODUCT and dtype == torch.bfloat16:
        return None
    stream = torch_npu.npu.Stream()
    with torch_npu.npu.stream(stream):
        val = 2 if (op == dist.ReduceOp.SUM and dtype == torch.bfloat16) else rank + 1
        tensor = torch.full((1, data), val, dtype=dtype).npu()
        graph = torch_npu.npu.NPUGraph()
        graph.capture_begin()
        dist.all_reduce(tensor, op=op)
        graph.capture_end()
        graph.replay()
        return tensor


def run_hccl(rank, master_ip):
    torch_npu.npu.set_device(rank)
    rank += DEV_NUM * SERVER_INDEX
    world_size = SERVER_NUM * DEV_NUM
    dist.init_process_group(
        backend="hccl", rank=rank, world_size=world_size, init_method=f'tcp://{master_ip}:{MASTER_PORT}'
    )
    try:
        cases = product(DTYPE_MAP.get(DTYPE, []), DATA_LIST, OP_MAP.get(OPTYPE, []), range(MUL_NUM))

        for dtype, data, op, loop in cases:
            tensor = run_single_case(rank, data, dtype, op, loop)
            if tensor is not None:
                check_tensor(tensor, op, world_size, data, dtype)
    except Exception as e:
        print(f'ERROR: {e}')


def train():
    master_ip = get_host_ip()
    print(
        "\n".join(
            f"{k}: {v}"
            for k, v in {
                'master_ip': master_ip,
                'master_port': MASTER_PORT,
                'dev_num': DEV_NUM,
                'server_num': SERVER_NUM,
                'server_index': SERVER_INDEX,
                'world_size': SERVER_NUM * DEV_NUM,
                'data_list': DATA_LIST,
                'dtype': DTYPE,
                'optype': OPTYPE,
                'mul_num': MUL_NUM,
                'main_pid': os.getpid(),
            }.items()
        )
    )
    mp.spawn(run_hccl, args=(master_ip,), nprocs=DEV_NUM)
