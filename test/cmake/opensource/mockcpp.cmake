set(BOOST_INCLUDE_DIRS ${TOP_DIR}/test/opensource/boost CACHE PATH "Boost include directory")
set(MOCKCPP_CXXFLAGS "-fPIC -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0 -Wno-unused-parameter")
set(MOCKCPP_LDFLAGS "-Wl,-z,relro,-z,now,-z,noexecstack")
set(MOCKCPP_LINKER_FLAGS "")

ExternalProject_Add(mockcpp_build
    SOURCE_DIR ${TOP_DIR}/test/opensource/mockcpp
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -G ${CMAKE_GENERATOR}
        -DCMAKE_CXX_FLAGS=${MOCKCPP_CXXFLAGS}
        -DCMAKE_SHARED_LINKER_FLAGS=${MOCKCPP_LINKER_FLAGS}
        -DCMAKE_EXE_LINKER_FLAGS=${MOCKCPP_LINKER_FLAGS}
        -DBUILD_32_BIT_TARGET_BY_64_BIT_COMPILER=OFF
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/mockcpp
        -DBOOST_INCLUDE_DIRS=${BOOST_INCLUDE_DIRS}
        <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    EXCLUDE_FROM_ALL TRUE
)
