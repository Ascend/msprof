# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

# pylint: disable=duplicate-code

from msconfig.meta_config import MetaConfig


class TsCPUConfig(MetaConfig):
    DATA = {
        'events': [
            ('0x00', 'SW_INCR'),
            ('0x01', 'L1I_CACHE_REFILL'),
            ('0x02', 'L1I_TLB_REFILL'),
            ('0x03', 'L1D_CACHE_REFILL'),
            ('0x04', 'L1D_CACHE'),
            ('0x05', 'L1D_TLB_REFILL'),
            ('0x06', 'LD_RETIRED'),
            ('0x07', 'ST_RETIRED'),
            ('0x08', 'INST_RETIRED'),
            ('0x09', 'EXC_TAKEN'),
            ('0x0A', 'EXC_RETURN'),
            ('0x0B', 'CID_WRITE_RETIRED'),
            ('0x0C', 'PC_WRITE_RETIRED'),
            ('0x0D', 'BR_IMMED_RETIRED'),
            ('0x0E', 'BR_RETURN_RETIRED'),
            ('0x0F', 'UNALIGNED_LDST_RETIRED'),
            ('0x10', 'BR_MIS_PRED'),
            ('0x11', 'CPU_CYCLES'),
            ('0x12', 'BR_PRED'),
            ('0x13', 'MEM_ACCESS'),
            ('0x14', 'L1I_CACHE'),
            ('0x15', 'L1D_CACHE_WB'),
            ('0x19', 'BUS_ACCESS'),
            ('0x1A', 'MEMORY_ERROR'),
            ('0x1B', 'INST_SPEC'),
            ('0x1C', 'TTBR_WRITE_RETIRED'),
            ('0x1D', 'BUS_CYCLES'),
            ('0x1E', 'CHAIN'),
            ('0x21', 'BR_RETIRED'),
            ('0x22', 'BR__MIS_PRED_RETIRED'),
            ('0x23', 'STALL_FRONTEND'),
            ('0x24', 'STALL_BACKEND'),
            ('0x25', 'L1D_TLB'),
            ('0x26', 'L1I_TLB'),
            ('0x34', 'DTLB_WALK'),
            ('0x35', 'ITLB_WALK'),
            ('0x36', 'LL_CACHE_RD'),
            ('0x37', 'LL_CACHE_MISS_RD'),
            ('0x38', 'REMOTE_ACCESS_RD'),
            ('0x40', 'L1D_CACHE_LD'),
            ('0x41', 'L1D_CACHE_ST'),
            ('0x42', 'L1D_CACHE_REFILL_LD'),
            ('0x43', 'L1D_CACHE_REFILL_ST'),
            ('0x44', 'L1D_CACHE_REFILL_INNER'),
            ('0x45', 'L1D_CACHE_REFILL_OUTER'),
            ('0x60', 'BUS_ACCESS_LD'),
            ('0x61', 'BUS_ACCESS_ST'),
            ('0x66', 'MEM_ACCESS_LD'),
            ('0x67', 'MEM_ACCESS_ST'),
            ('0x70', 'LD_SPEC'),
            ('0x71', 'ST_SPEC'),
            ('0x72', 'LDST_SPEC'),
            ('0x73', 'DP_SPEC'),
            ('0x74', 'ASE_SPEC'),
            ('0x75', 'VFP_SPEC'),
            ('0x76', 'PC_WRITE_SPEC'),
            ('0x77', 'CRYPTO_SPEC'),
            ('0x78', 'BR_IMMED_SPEC'),
            ('0x79', 'BR_RETURN_SPEC'),
            ('0x7A', 'BR_INDIRECT_SPEC'),
            ('0x86', 'EXC_IRQ'),
            ('0x87', 'EXC_FIQ'),
            ('0xC2', 'L1D_CACHE_REFILL_PREFETCH'),
            ('0xC4', 'L1D_WS_MODE_ENTRY'),
            ('0xC5', 'L1D_WS_MODE'),
            ('0xC6', 'PREDECODE_ERROR'),
            ('0xC9', 'BR_COND_PRED'),
            ('0xCA', 'BR_INDIRECT_MIS_PRED'),
            ('0xCB', 'BR_INDIRECT_ADDR_MIS_PRED'),
            ('0xCC', 'BR_COND_MIS_PRED'),
            ('0xCD', 'BR_INDIRECT_ADDR_PRED'),
            ('0xCE', 'BR_RETURN_ADDR_PRED'),
            ('0xCF', 'BR_RETURN_ADDR_MIS_PRED'),
            ('0xE1', 'STALL_FRONTEND_CACHE'),
            ('0xE2', 'STALL_FRONTEND_TLB'),
            ('0xE3', 'STALL_FRONTEND_PDERR'),
            ('0xE4', 'STALL_BACKEND_ILOCK'),
            ('0xE5', 'STALL_BACKEND_ILOCK_AGU'),
            ('0xE6', 'STALL_BACKEND_ILOCK_FPU'),
            ('0xE7', 'STALL_BACKEND_LD'),
            ('0xE8', 'STALL_BACKEND_ST'),
            ('0xE9', 'STALL_BACKEND_LD_CACHE'),
            ('0xEA', 'STALL_BACKEND_LD_TLB'),
            ('0xEB', 'STALL_BACKEND_ST_STB'),
            ('0xEC', 'STALL_BACKEND_ST_TLB'),
        ]
    }
