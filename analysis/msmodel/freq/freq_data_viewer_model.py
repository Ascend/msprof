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

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from msmodel.interface.view_model import ViewModel


class FreqDataViewModel(ViewModel):
    '''
    class for query aicore freq.db
    '''

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._iter_range = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_FREQ, [DBNameConstant.TABLE_FREQ_PARSE])

    def get_data(self) -> list:
        '''
        query data from freq.db
        '''
        if not DBManager.check_tables_in_db(
            PathManager.get_db_path(self._result_dir, self.db_name), DBNameConstant.TABLE_FREQ_PARSE
        ):
            return []
        sql = "select syscnt, freq from {}".format(DBNameConstant.TABLE_FREQ_PARSE)
        return DBManager.fetch_all_data(self.cur, sql)
