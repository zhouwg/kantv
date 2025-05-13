#ifndef HAP_PROCESS_H
#define HAP_PROCESS_H
/*==============================================================================
  Copyright (c) 2024 Qualcomm Technologies Incorporated.
  All Rights Reserved Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S.
  Government. Diversion contrary to U.S. law prohibited.
==============================================================================*/

/** @defgroup process_type Process type
 *  @{
 */
/** Return values for HAP_get_pd_type
	Returns any one of the below values depending on the type of PD spawned */
enum process_type {
	ROOT_PD				= 0,
	AUDIO_STATIC_PD			= 1,
	SENSOR_STATIC_PD		= 2,
	DYNAMIC_SIGNED_PD		= 3,
	DYNAMIC_UNSIGNED_PD		= 4,
	DYNAMIC_CPZ_PD			= 5,
	SECURE_PD			= 6,
	DYNAMIC_SYS_UNSIGNED_PD		= 7,
	OIS_STATIC_PD			= 8,
	MAX_PD_TYPE			= 9		/**< Maximum number of supported PD types */
};
/**
 * @} // process_type
 */
#endif