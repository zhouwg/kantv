#ifndef HAP_PD_DTOR_H
#define HAP_PD_DTOR_H
/*==============================================================================
  Copyright (c) 2015 Qualcomm Technologies Incorporated.
  All Rights Reserved Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S.
  Government. Diversion contrary to U.S. law prohibited.
==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type is used to provide the qdi driver the register address, and the
 * bits of the register to clear.
 *
 * @param register_addr,  The register address whose value needs to modified on process exit
 * @param register_mask, A mask that indicates which bits of the register need to be set.
 * @param register_val, The value that needs to be applied to the unmasked bits.
 */
typedef struct {
    uintptr_t   register_addr;
    uint32      register_mask;
    uint32      register_value;
}HAP_register_t;

/**
 * A fastrpc process can call this method and provide a list of register addresses
 * and their desired values. When the fastrpc process exits, a previous call to this
 * method will ensure that the bits (defined in the bitmask) for the provided
 * registers are cleared.

 * @param num_registers, Number of registers that need to be modified on process exit
 * @param registers, The register address and masks.
 */
int HAP_clear_registers(unsigned int num_registers, HAP_register_t* registers);

/**
 * This method is used by the kernel to free any memory that
 * a fastrpc client might have line-locked
*/
int HAP_free_linelocked_memory(unsigned int asid);

#ifdef __cplusplus
}
#endif

#endif /*HAP_PD_DTOR_H */

