#===============================================================================
# @brief    cmake file
# Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
#===============================================================================
if(DEFINED CONFIG_SAMPLE_SUPPORT_FOC)
    add_subdirectory_if_exist(foc)
endif()

set(SOURCES "${SOURCES}" PARENT_SCOPE)