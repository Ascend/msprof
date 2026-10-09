#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


class ClassMock:
    def __init__(self: any, owner: any, instance: any) -> None:
        self.owner = owner
        self.instance = instance

    def __enter__(self: any) -> None:
        self.owner.__new__ = self.instance

    def __exit__(self: any, exc_type, exc_val, exc_tb) -> None:
        self.owner.__new__ = ClassMock.origin_new_function

    @staticmethod
    def origin_new_function(cls: any, *args, **kwargs) -> any:  # pylint: disable=bad-staticmethod-argument
        return object().__new__(cls)  # pylint: disable=no-value-for-parameter
