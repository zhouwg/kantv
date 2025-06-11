/*-----------------------------------------------------------------------------
   Copyright (c) 2019-2020 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------------*/

#ifndef HAP_USER_PMU_H_
#define HAP_USER_PMU_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @file HAP_user_pmu.h
 *  @brief HAP user PMU API
 */

/** @defgroup Constants constants
 *  @{
 */

/** Error value for unsupported APIs. */
#define HAP_USER_PMU_READ_NOT_SUPPORTED                           0x80000FFF

/** Error value for PMU read failure. */
#define HAP_USER_PMU_READ_FAILED                                  0xDEADDEAD

/** @}
 */

/** @defgroup Types Data types
 *  @{
 */

/**
 * Input parameter type used when a group of PMU events must be read via
 * HAP_register_pmu_group(), HAP_read_pmu_group() and HAP_deregister_pmu_group().

 * The user must fill in the pmu_events[] array field of this structure with the
 * specified PMU events to track and update the num_events field with the number
 * of events to track. Only four unique PMU events can be tracked.
 */
typedef struct {
    int contextId;
    /**< Return value after registering the PMU group via HAP_register_pmu_group. */

    unsigned int num_events;
    /**< Input parameter specifying the number of PMU events register.*/

    unsigned short pmu_events[4];
    /**< Input parameter specifying the list of PMU events to register.*/

    unsigned int pmu_value[4];
    /**< Output parameter containing values of PMU events registered. */
} HAP_pmu_group_config_t;

/** @}
 */

/**
 * @cond DEV
 */
int __attribute__((weak)) __HAP_register_pmu_group(HAP_pmu_group_config_t* pmu_config);
int __attribute__((weak)) __HAP_deregister_pmu_group(int contextId);
int __attribute__((weak)) __HAP_read_pmu_group(HAP_pmu_group_config_t* pmu_config);
int __attribute__((weak)) __HAP_register_pmu_event(unsigned short pmu_event);
int __attribute__((weak)) __HAP_deregister_pmu_event(unsigned short pmu_event);
unsigned int __attribute__((weak)) __HAP_read_pmu_event(unsigned short pmu_event);

/**
 * @endcond
 */

/** @defgroup GroupFunc API for reading a group of PMUs
 *  These APIs expose a way to register and read an array of PMU events
 *  (maximum of four PMU events) by using the #HAP_pmu_group_config_t structure.
 *  Alternatively, the user can use a different set of APIs explained in the next
 *  section to configure and read a single PMU event.
 *  @{
 */

/**
 * Registers a group of PMU events to read.
 *
 * Call this function from the DSP user process to register a set of PMU events
 * (maximum of four) for tracking. Fill in the pmu_events[] array file of
 * @p pmu_config with the specified PMU events to track (maximum of four) and
 * update the num_events field of @p pmu_config with the number of PMU events
 * written into the pmu_events[] array.
 *
 * @param pmu_config Pointer to HAP_pmu_group_config_t structure with
 *                   pmu_events[] array and num_events fields updated.
 *
 * @return 0 upon success. Updates the contextId field of @p pmu_config.
 * @par
 * The same pmu_config structure should be used for reading the PMU
 * counter values #HAP_read_pmu_group() corresponding to the
 * configured events and for de-registration #HAP_deregister_pmu_group().
 */
static inline int HAP_register_pmu_group(HAP_pmu_group_config_t* pmu_config) {
    if(__HAP_register_pmu_group)
        return __HAP_register_pmu_group(pmu_config);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/**
 * Reads the PMU values of registered PMU events.
 *
 * Call this function after successfully calkling HAP_register_pmu_group() with the
 * same structure pointer, @p pmu_config.
 * This API uses the context_id field of the input @p pmu_config
 * structure, which is set in a successful HAP_register_pmu_group().
 *
 * @param pmu_config Pointer to the #HAP_pmu_group_config_t structure used in
 *                   #HAP_register_pmu_group() call.
 * @return
 * 0 upon success. Updates the pmu_value[] array corresponding to the
 * configured pmu_events[] in the structure pointed to by @p pmu_config.
 * pmu_value[x] is updated to HAP_USER_PMU_READ_FAILED if the corresponding pmu_event[x]
 * configuration has failed or is invalid.
 * @par
 * Other values upon failure. \n
 * @par
 * #HAP_USER_PMU_READ_NOT_SUPPORTED when unsupported.
 */
static inline int HAP_read_pmu_group(HAP_pmu_group_config_t* pmu_config) {
    if(__HAP_read_pmu_group)
        return __HAP_read_pmu_group(pmu_config);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/**
 * De-registers a group of PMU events registered via HAP_register_pmu_group().
 *
 * @param pmu_config Pointer to the #HAP_pmu_group_config_t structure used in the
 *                   HAP_register_pmu_group() call.

  * @return
 * 0 upon success. \n
 * Other values upon failure.
 */
static inline int HAP_deregister_pmu_group(HAP_pmu_group_config_t* pmu_config) {
    if(__HAP_deregister_pmu_group)
        return __HAP_deregister_pmu_group(pmu_config->contextId);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/**
 * @}
 */

/** @defgroup singleFunc API for reading single PMU event
 *  These APIs allow the user to configure and read single PMU events.
 *  PMU event is used as an input in register, read and de-register APIs.
 *  Up to four unique PMU event requests can be served.
 *  @{
 */

/**
 * Registers sa PMU event for read.
 *
 * @param pmu_event PMU event to register.
 *
 * @return
 * 0 upon success. \n
 * Other values upon failure.
 */
static inline int HAP_register_pmu_event(unsigned short pmu_event) {
    if(__HAP_register_pmu_event)
        return __HAP_register_pmu_event(pmu_event);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/**
 * Reads the PMU event registered via HAP_register_pmu_event().
 *
 * @param pmu_event PMU event to read. Should already be registered via
 *                  HAP_register_pmu_event().
 *
 * @return
 * The value of the PMU counter corresponding to the pmu_event. \n
 * - HAP_USER_PMU_READ_NOT_SUPPORTED -- API is unsupported. \n
 * - HAP_USER_PMU_READ_FAILED -- The given @p pmu_event read fails.
 */
static inline unsigned int HAP_read_pmu_event(unsigned short pmu_event) {
    if(__HAP_read_pmu_event)
        return __HAP_read_pmu_event(pmu_event);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/**
 * De-registers the PMU event registered via HAP_register_pmu_event().
 *
 * @param pmu_event PMU event to de-register. It should already be registered
 *                  via #HAP_register_pmu_event().
 *
 * @return
 * 0 upon success. \n
 * Other values upon failure. \n
 * HAP_USER_PMU_READ_NOT_SUPPORTED when not supported.
 */
static inline int HAP_deregister_pmu_event(unsigned short pmu_event) {
    if(__HAP_deregister_pmu_event)
        return __HAP_deregister_pmu_event(pmu_event);

    return HAP_USER_PMU_READ_NOT_SUPPORTED;
}

/** @}
 */

#ifdef __cplusplus
}
#endif
#endif /*HAP_USER_PMU_H_*/
