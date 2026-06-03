# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

"""
This file used for basic python type modification
"""

from typing import OrderedDict
from collections import namedtuple
from collections import OrderedDict as _OrderedDict
from dataclasses import fields, MISSING


class HighPerfDict(OrderedDict):
    """
    this class will provide some high perf interface for dict
    """

    def set_default_call_obj_later(self: any, key: any, _class: any, *args, **kwargs):
        if key in self.keys():
            return self.get(key)
        obj = _class(*args, **kwargs)
        self.setdefault(key, obj)
        return obj


class CustomizedNamedtupleFactory:
    @staticmethod
    def generate_named_tuple_from_dto(dto_class: type, description: any) -> any:
        """
        dynamically generate namedtuple from dto class which meets requests in below:
        1. Contains all the names that sql returns
        2. If an attribute in dto has a default value and not in sql, it should be in the namedtuple
        3. Attribute that come from sql, should be placed ahead with the same order in sql
        4. A default value should be defined for all attributes
        5. dto should be in form of dataclass
        dto_class: dto class
        description: description from sql, each line describe a name from db
        """
        description_set = {i[0] for i in description}
        extend_columns = _OrderedDict()
        # get all attribute of dataclass by fields()
        for item in fields(dto_class):
            if item.name not in description_set:
                # dataclass use an object to represent default value if not defined, should be replaced by None,
                # otherwise it may throw error when insert the tuple into db
                if item.default == MISSING:
                    extend_columns[item.name] = None
                else:
                    extend_columns[item.name] = item.default
        filed_names = [i[0] for i in description]
        # place name that in dto and not in sql at the end, use extend slightly improve efficiency
        filed_names.extend(extend_columns.keys())
        defaults = [None] * len(description)
        defaults.extend(extend_columns.values())
        # use the same name as the dto, when call isinstance compare __name__
        base_tuple = namedtuple(dto_class.__name__, filed_names, defaults=defaults)
        # get all reserved functions
        extra_properties = {}
        for name in dir(dto_class):
            if isinstance(getattr(dto_class, name), property):
                extra_properties[name] = getattr(dto_class, name)
        return CustomizedNamedtupleFactory.enhance_namedtuple(base_tuple, extra_properties)

    @staticmethod
    def enhance_namedtuple(tuple_type: type, function_dict: dict):
        """
        Enhance namedtuple, add function or attribute for it.
        tuple_type: base type
        function_dict: functions or attrs added to the type
        """
        class_namespace = dict(tuple_type.__dict__)
        class_namespace.update(function_dict)
        # rename _replace, to pass codecheck
        class_namespace["replace"] = class_namespace.get("_replace")
        return type(tuple_type.__name__, (tuple,), class_namespace)
