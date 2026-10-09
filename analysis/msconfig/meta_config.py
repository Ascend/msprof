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


class MetaConfig:
    DATA = {}

    def __init__(self):
        self.support_parser = True
        self.lower_option()

    def lower_option(self):
        if not self.support_parser:
            return
        for section in self.DATA:
            items = self.DATA.get(section, [])
            for i, item in enumerate(items):
                items[i] = (str(item[0]).lower(), str(item[1]))

    def sections(self):
        if not self.support_parser:
            return []
        return self.DATA.keys()

    def has_section(self, section: str):
        if not self.support_parser:
            return False
        return section in self.DATA

    def has_option(self, section: str, option: str):
        if not self.support_parser:
            return False
        options = self.options(section)
        return option in options

    def options(self, section: str):
        if not self.support_parser:
            return []
        values = self.DATA.get(section, [])
        options = [value[0] for value in values]
        return options

    def get(self, section: str, option: str):
        if not self.support_parser:
            return ''
        for value in self.DATA.get(section, []):
            if value[0] == option:
                return value[1]
        return ''

    def items(self, section: str):
        if not self.support_parser:
            return []
        return self.DATA.get(section, [])

    def get_data(self):
        return self.DATA
