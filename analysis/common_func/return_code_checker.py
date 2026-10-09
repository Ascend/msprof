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

import json
import logging

from common_func.common import call_sys_exit
from common_func.common import print_msg
from common_func.ms_constant.number_constant import NumberConstant


class ReturnCodeCheck:
    """
    check interface return code
    """

    RETURN_CODE_KEY = "status"

    @classmethod
    def print_and_return_status(cls: any, json_dump: any) -> None:
        """
        print and return error code
        """
        print_msg(json_dump)
        cls.finish_with_error_code(json_dump)

    @classmethod
    def finish_with_error_code(cls: any, json_dump: any) -> None:
        """
        finish with the return code
        """
        try:
            call_sys_exit(json.loads(json_dump).get(cls.RETURN_CODE_KEY, NumberConstant.SUCCESS))
        except (ValueError, TypeError) as err:
            logging.error(err)
            call_sys_exit(NumberConstant.ERROR)
        finally:
            pass
