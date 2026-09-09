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

import logging
import multiprocessing

from common_func.cpp_pipeline_decision import CppPipelineDecisionRequest
from common_func.cpp_pipeline_decision import decide_cpp_pipeline
from common_func.msprof_common import prepare_log
from common_func.msprof_exception import ProfException
from msconfig.cpp_pipeline_capability_config import PipelineCommand
from msinterface import msprof_c_interface


class CppPipelineRunner:
    """Translate an approved request to the native parser.run_pipeline interface."""

    def run(self, request: CppPipelineDecisionRequest) -> None:
        """Isolate native context per collection and propagate execution failure."""
        process = multiprocessing.Process(target=self._run, args=(request,))
        process.start()
        process.join()
        if process.exitcode != 0:
            raise ProfException(
                ProfException.PROF_INVALID_DATA_ERROR,
                "Full C pipeline failed for %s (process exit code %s)." % (request.collection_path, process.exitcode),
            )

    def _run(self, request: CppPipelineDecisionRequest) -> None:
        flags = msprof_c_interface.MSPROF_PIPELINE_CANN_TRACE if request.host_path else 0
        if request.device_paths:
            flags |= msprof_c_interface.MSPROF_PIPELINE_DEVICE_DATA
        if request.command == PipelineCommand.EXPORT:
            flags |= {
                "db": msprof_c_interface.MSPROF_PIPELINE_DB,
                "timeline": msprof_c_interface.MSPROF_PIPELINE_TIMELINE,
                "summary": msprof_c_interface.MSPROF_PIPELINE_SUMMARY,
            }[request.command_type]
        # DeviceContext consumes the parent PROF directory of this result path.
        device_path = request.device_paths[0] if request.device_paths else None
        status = msprof_c_interface.run_pipeline(
            request.collection_path,
            flags,
            request.host_path or None,
            device_path,
            request.reports_path or None,
        )
        if status != msprof_c_interface.MSPROF_OK:
            raise ProfException(
                ProfException.PROF_INVALID_DATA_ERROR,
                "Full C pipeline failed for %s (status %s)." % (request.collection_path, status),
            )


def try_full_cpp_pipeline(request: CppPipelineDecisionRequest) -> bool:
    """Return False only for preflight rejection; raise on native execution failure."""
    result_path = request.host_path or (request.device_paths[0] if request.device_paths else "")
    if result_path:
        # Reuse one existing result log for the collection-wide decision.
        prepare_log(result_path)
    decision = decide_cpp_pipeline(request)
    if not decision.can_run_in_cpp:
        logging.info(
            "Full C pipeline is unavailable for %s: %s",
            request.collection_path,
            "; ".join(issue.reason_code for issue in decision.issues),
        )
        return False
    logging.info("Run full C pipeline for %s.", request.collection_path)
    try:
        CppPipelineRunner().run(request)
    except (OSError, ProfException):
        logging.exception("Full C pipeline failed for %s.", request.collection_path)
        raise
    logging.info("Full C pipeline completed for %s.", request.collection_path)
    return True
