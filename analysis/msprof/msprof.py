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

import importlib
import os
import sys

if __name__ == '__main__':
    sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))
    MODEL_PATH = "msinterface.msprof_entrance"
    MSPROF_ENTRANCE_CLASS = "MsprofEntrance"
    os.umask(0o027)
    msprof_entrance_module = importlib.import_module(MODEL_PATH)
    if hasattr(msprof_entrance_module, MSPROF_ENTRANCE_CLASS):
        getattr(msprof_entrance_module, MSPROF_ENTRANCE_CLASS)().main()
