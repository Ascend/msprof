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
# MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging
import struct
from dataclasses import dataclass
from typing import Any
from typing import Callable
from typing import Dict
from typing import Iterator
from typing import Optional
from typing import Tuple

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from msparser.data_struct_size_constant import StructFmt


@dataclass(frozen=True)
class VersionedStructSpec:
    """
    Binary structure description for a collection data version.
    """

    fmt: str
    size: int
    decoder: Optional[Callable[[Tuple], Any]] = None


class VersionedStructParser:
    """
    Select and unpack binary records by collection version.
    """

    DEFAULT_VERSION = "1.0"

    def __init__(self: any, parser_name: str, specs: Dict[str, VersionedStructSpec]) -> None:
        self._parser_name = parser_name
        self._specs = specs
        self._spec = self._select_spec()

    @property
    def spec(self: any) -> VersionedStructSpec:
        return self._spec

    def iter_records(self: any, data: bytes) -> Iterator[Tuple]:
        """
        Iterate unpacked records. Incomplete bytes should already be filtered by OffsetCalculator.
        """
        if not data:
            return
        for index in range(len(data) // self._spec.size):
            start = index * self._spec.size
            one_slice = data[start : start + self._spec.size]
            if not one_slice:
                break
            yield struct.unpack(StructFmt.BYTE_ORDER_CHAR + self._spec.fmt, one_slice)

    def iter_decoded_records(self: any, data: bytes) -> Iterator[Any]:
        """
        Iterate records decoded to parser-defined normalized data.
        """
        for record in self.iter_records(data):
            yield self.decode_record(record)

    def decode_record(self: any, record: Tuple) -> Any:
        if self._spec.decoder:
            return self._spec.decoder(record)
        return record

    def _select_spec(self: any) -> VersionedStructSpec:
        collection_version = InfoConfReader().get_collection_version()
        spec = self._specs.get(collection_version)
        if spec:
            return spec
        default_spec = self._specs.get(self.DEFAULT_VERSION)
        logging.warning(
            "%s collection version is not found, use %s parser.",
            self._parser_name,
            self.DEFAULT_VERSION,
            exc_info=Constant.TRACE_BACK_SWITCH,
        )
        return default_spec
