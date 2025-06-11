/*
 * Copyright (c) 2021 QUALCOMM Technologies Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include "remote.h"

domain supported_domains[] = {
    {ADSP_DOMAIN_ID, ADSP_DOMAIN},
    {MDSP_DOMAIN_ID, MDSP_DOMAIN},
    {SDSP_DOMAIN_ID, SDSP_DOMAIN},
    {CDSP_DOMAIN_ID, CDSP_DOMAIN},
    {CDSP1_DOMAIN_ID, CDSP1_DOMAIN}
};

bool is_CDSP(int domain_id) {
    return (domain_id == CDSP_DOMAIN_ID || domain_id == CDSP1_DOMAIN_ID);
}
