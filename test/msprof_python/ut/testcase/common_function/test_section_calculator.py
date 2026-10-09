#!/usr/bin/python3
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
import types
import unittest

from common_func.section_calculator import SectionCalculator
from common_func.msprof_object import CustomizedNamedtupleFactory
from profiling_bean.db_dto.time_section_dto import TimeSectionDto, CommunicationTimeSection

TimeSectionDtoTuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TimeSectionDto, [])


class TestSectionCalculator(unittest.TestCase):

    def test_merge_continuous_intervals_when_empty_input(self):
        self.assertEqual(SectionCalculator.merge_continuous_intervals([]), [])

    def test_merge_continuous_intervals_when_continuous(self):
        section = [
            TimeSectionDtoTuple(start_time=2, end_time=4), TimeSectionDtoTuple(start_time=1, end_time=5)
        ]
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 1)

    def test_merge_continuous_intervals_when_intervals(self):
        section = [TimeSectionDtoTuple(start_time=7, end_time=9), TimeSectionDtoTuple(start_time=1, end_time=3)]
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 2)

    def test_compute_overlap_time(self):
        master_section = [
            TimeSectionDtoTuple(start_time=4, end_time=9),
            TimeSectionDtoTuple(start_time=14, end_time=15),
            TimeSectionDtoTuple(start_time=17, end_time=18)
        ]
        slave_section = [
            TimeSectionDtoTuple(start_time=1, end_time=2), TimeSectionDtoTuple(start_time=3, end_time=5),
            TimeSectionDtoTuple(start_time=6, end_time=7), TimeSectionDtoTuple(start_time=8, end_time=10),
            TimeSectionDtoTuple(start_time=11, end_time=12),
            TimeSectionDtoTuple(start_time=13, end_time=16), TimeSectionDtoTuple(start_time=19, end_time=20)
        ]
        result = SectionCalculator.compute_overlap_time(master_section, slave_section)
        self.assertEqual(result[0].overlap_time, 3)
        self.assertEqual(result[1].overlap_time, 1)
        self.assertEqual(result[2].overlap_time, 0)

    def test_compute_pipeline_overlap_should_return_empty_when_no_time_sections(self):
        communication_section = []
        compute_section = []
        latest_time = 1000
        earliest_time = 0

        pure_communication_section, free_time_section = SectionCalculator.compute_pipeline_overlap(
            communication_section, compute_section, latest_time, earliest_time)

        self.assertEqual(pure_communication_section, [])
        self.assertEqual(free_time_section, [])

    def test_compute_pipeline_overlap_should_return_free_time_when_max_end_time_less_than_latest_time(self):
        communication_section = [SectionCalculator._generate_time_section(100, 200, CommunicationTimeSection)]
        compute_section = [SectionCalculator._generate_time_section(300, 400)]
        latest_time = 500
        earliest_time = 0

        pure_communication_section, free_time_section = SectionCalculator.compute_pipeline_overlap(
            communication_section, compute_section, latest_time, earliest_time)

        self.assertEqual(len(pure_communication_section), 1)
        self.assertEqual(len(free_time_section), 3)
        self.assertEqual(free_time_section[0].start_time, 200)
        self.assertEqual(free_time_section[0].end_time, 300)
        self.assertEqual(free_time_section[1].start_time, 400)
        self.assertEqual(free_time_section[1].end_time, 500)
        self.assertEqual(free_time_section[2].start_time, 0)
        self.assertEqual(free_time_section[2].end_time, 100)