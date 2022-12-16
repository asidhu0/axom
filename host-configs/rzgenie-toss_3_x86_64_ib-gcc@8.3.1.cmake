#------------------------------------------------------------------------------
# !!!! This is a generated file, edit at own risk !!!!
#------------------------------------------------------------------------------
# CMake executable path: /usr/tce/packages/cmake/cmake-3.16.8/bin/cmake
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Compilers
#------------------------------------------------------------------------------
# Compiler Spec: gcc@8.3.1
#------------------------------------------------------------------------------
if(DEFINED ENV{SPACK_CC})

  set(CMAKE_C_COMPILER "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/spack/lib/spack/env/gcc/gcc" CACHE PATH "")

  set(CMAKE_CXX_COMPILER "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/spack/lib/spack/env/gcc/g++" CACHE PATH "")

  set(CMAKE_Fortran_COMPILER "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/spack/lib/spack/env/gcc/gfortran" CACHE PATH "")

else()

  set(CMAKE_C_COMPILER "/usr/tce/packages/gcc/gcc-8.3.1/bin/gcc" CACHE PATH "")

  set(CMAKE_CXX_COMPILER "/usr/tce/packages/gcc/gcc-8.3.1/bin/g++" CACHE PATH "")

  set(CMAKE_Fortran_COMPILER "/usr/tce/packages/gcc/gcc-8.3.1/bin/gfortran" CACHE PATH "")

endif()

set(ENABLE_FORTRAN ON CACHE BOOL "")

#------------------------------------------------------------------------------
# MPI
#------------------------------------------------------------------------------

set(MPI_C_COMPILER "/usr/tce/packages/mvapich2/mvapich2-2.3-clang-10.0.0/bin/mpicc" CACHE PATH "")

set(MPI_CXX_COMPILER "/usr/tce/packages/mvapich2/mvapich2-2.3-clang-10.0.0/bin/mpicxx" CACHE PATH "")

set(MPI_Fortran_COMPILER "/usr/tce/packages/mvapich2/mvapich2-2.3-clang-10.0.0/bin/mpif90" CACHE PATH "")

set(MPIEXEC_EXECUTABLE "/usr/bin/srun" CACHE PATH "")

set(MPIEXEC_NUMPROC_FLAG "-n" CACHE STRING "")

set(ENABLE_MPI ON CACHE BOOL "")

#------------------------------------------------------------------------------
# Hardware
#------------------------------------------------------------------------------

#------------------------------------------------
# Hardware Specifics
#------------------------------------------------

set(ENABLE_OPENMP ON CACHE BOOL "")

set(ENABLE_GTEST_DEATH_TESTS ON CACHE BOOL "")

#------------------------------------------------------------------------------
# TPLs
#------------------------------------------------------------------------------

# Root directory for generated TPLs

set(TPL_ROOT "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/gcc-8.3.1" CACHE PATH "")

set(CONDUIT_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/conduit-0.8.3" CACHE PATH "")

set(C2C_DIR "${TPL_ROOT}/c2c-1.3.0" CACHE PATH "")

set(MFEM_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/mfem-4.5.0" CACHE PATH "")

set(HDF5_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/hdf5-1.8.22" CACHE PATH "")

set(LUA_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-9.0.0/lua-5.4.4" CACHE PATH "")

set(RAJA_DIR "${TPL_ROOT}/raja-2022.03.0" CACHE PATH "")

set(UMPIRE_DIR "${TPL_ROOT}/umpire-2022.03.1" CACHE PATH "")

set(CAMP_DIR "${TPL_ROOT}/camp-2022.03.2" CACHE PATH "")

set(SCR_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/scr-3.0.1" CACHE PATH "")

set(KVTREE_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/kvtree-1.3.0" CACHE PATH "")

set(DTCMP_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/dtcmp-1.1.4" CACHE PATH "")

set(SPATH_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/spath-0.2.0" CACHE PATH "")

set(AXL_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/axl-0.7.1" CACHE PATH "")

set(LWGRP_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/lwgrp-1.0.5" CACHE PATH "")

set(ER_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/er-0.2.0" CACHE PATH "")

set(RANKSTR_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/rankstr-0.1.0" CACHE PATH "")

set(REDSET_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/redset-0.2.0" CACHE PATH "")

set(SHUFFILE_DIR "/usr/WS1/axom/libs/toss_3_x86_64_ib/2022_12_08_21_32_28/clang-10.0.0/shuffile-0.2.0" CACHE PATH "")

set(LIBYOGRT_DIR "/usr" CACHE PATH "")

#------------------------------------------------------------------------------
# Devtools
#------------------------------------------------------------------------------

# Root directory for generated developer tools

set(DEVTOOLS_ROOT "/collab/usr/gapps/axom/devtools/toss_3_x86_64_ib/2022_07_12_16_37_49/gcc-8.1.0" CACHE PATH "")

set(CLANGFORMAT_EXECUTABLE "/usr/tce/packages/clang/clang-10.0.0/bin/clang-format" CACHE PATH "")

set(PYTHON_EXECUTABLE "${DEVTOOLS_ROOT}/python-3.9.13/bin/python3.9" CACHE PATH "")

set(ENABLE_DOCS ON CACHE BOOL "")

set(SPHINX_EXECUTABLE "${DEVTOOLS_ROOT}/py-sphinx-5.0.2/bin/sphinx-build" CACHE PATH "")

set(SHROUD_EXECUTABLE "${DEVTOOLS_ROOT}/py-shroud-0.12.2/bin/shroud" CACHE PATH "")

set(CPPCHECK_EXECUTABLE "${DEVTOOLS_ROOT}/cppcheck-2.8/bin/cppcheck" CACHE PATH "")

set(DOXYGEN_EXECUTABLE "${DEVTOOLS_ROOT}/doxygen-1.8.14/bin/doxygen" CACHE PATH "")


