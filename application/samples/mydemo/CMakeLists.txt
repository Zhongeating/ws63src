#===============================================================================
# @brief    cmake file
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
#===============================================================================
if(DEFINED CONFIG_SAMPLE_SUPPORT_FOC)
    add_subdirectory_if_exist(foc)
endif()

if(DEFINED CONFIG_SAMPLE_SUPPORT_LFS)
    add_subdirectory_if_exist(lfs)
endif()

set(SOURCES "${SOURCES}" PARENT_SCOPE)