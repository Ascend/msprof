#ifndef MSPROF_TEST_MSPROF_CPP_ANALYSIS_UT_DOMAIN_DATA_PROCESS_TEST_RESERVE_MOCK_UTILS_H
#define MSPROF_TEST_MSPROF_CPP_ANALYSIS_UT_DOMAIN_DATA_PROCESS_TEST_RESERVE_MOCK_UTILS_H

#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Test {
template <typename ElemT>
inline void StubReserveFailureForElem()
{
    MOCKER_CPP(&Analysis::Utils::Reserve<ElemT>).stubs().will(returnValue(false));
}

template <typename ElemT>
inline void ResetReserveFailureForElem()
{
    MOCKER_CPP(&Analysis::Utils::Reserve<ElemT>).reset();
}

template <typename VectorT>
inline void StubReserveFailureForVector()
{
    StubReserveFailureForElem<typename VectorT::value_type>();
}

template <typename VectorT>
inline void ResetReserveFailureForVector()
{
    ResetReserveFailureForElem<typename VectorT::value_type>();
}
} // namespace Test
} // namespace Analysis

#endif
