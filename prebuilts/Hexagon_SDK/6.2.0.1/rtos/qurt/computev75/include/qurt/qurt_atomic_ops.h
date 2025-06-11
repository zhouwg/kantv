#ifndef QURT_ATOMIC_OPS_H
#define QURT_ATOMIC_OPS_H
/**
  @file qurt_atomic_ops.h 
  @brief  Prototypes of kernel atomic operations API.

  EXTERNAL FUNCTIONS
   None.

   INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2021, 2022  by Qualcomm Technologies, Inc.  All Rights Reserved
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

/*
 * Australian Public Licence B (OZPLB)
 *
 * Version 1-0
 *
 * Copyright (c) 2007, Open Kernel Labs, Inc.
 *
 * All rights reserved. 
 *
 * Developed by: Embedded, Real-time and Operating Systems Program (ERTOS)
 *               National ICT Australia
 *               http://www.ertos.nicta.com.au
 *
 * Permission is granted by National ICT Australia, free of charge, to
 * any person obtaining a copy of this software and any associated
 * documentation files (the "Software") to deal with the Software without
 * restriction, including (without limitation) the rights to use, copy,
 * modify, adapt, merge, publish, distribute, communicate to the public,
 * sublicense, and/or sell, lend or rent out copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimers in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of National ICT Australia, nor the names of its
 *       contributors, may be used to endorse or promote products derived
 *       from this Software without specific prior written permission.
 *
 * EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
 * PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND
 * NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,
 * WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS
 * REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF
 * ERRORS, WHETHER OR NOT DISCOVERABLE.
 *
 * TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL
 * THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER
 * LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR
 * OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS
 * OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR
 * OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,
 * CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN
 * CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS
 * CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,
 * DAMAGES OR OTHER LIABILITY.
 *
 * If applicable legislation implies representations, warranties, or
 * conditions, or imposes obligations or liability on National ICT
 * Australia or one of its contributors in respect of the Software that
 * cannot be wholly or partly excluded, restricted or modified, the
 * liability of National ICT Australia or the contributor is limited, to
 * the full extent permitted by the applicable legislation, at its
 * option, to:
 * a.  in the case of goods, any one or more of the following:
 * i.  the replacement of the goods or the supply of equivalent goods;
 * ii.  the repair of the goods;
 * iii. the payment of the cost of replacing the goods or of acquiring
 *  equivalent goods;
 * iv.  the payment of the cost of having the goods repaired; or
 * b.  in the case of services:
 * i.  the supplying of the services again; or
 * ii.  the payment of the cost of having the services supplied again.
 *
 * The construction, validity and performance of this licence is governed
 * by the laws in force in New South Wales, Australia.
 */

/*
 * Author: Malcolm Purvis <malcolmp@ok-labs.com>
 *
 * This file is only included by the main atomic_ops.h, so all of that
 * file's definitions are available.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/

///* Sanity check to ensure the smp flag is set in machines.py */
//#if defined(__ATOMIC_OPS_IN_KERNEL__) && !defined(MACHINE_SMP) && CONFIG_NUM_UNITS > 1
//#error CONFIG_NUM_UNITS > 1 but smp not defined in machines.py.
//#endif
#define QURT_INLINE  __attribute__((always_inline))

/*=============================================================================
                        FUNCTIONS
=============================================================================*/
/**@ingroup func_qurt_atomic_set
  Sets the atomic variable with the specified value.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      value  Value to set.
  
  @return
  Value successfuly set.

  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_set(unsigned int* target, unsigned int value)
{
    unsigned long tmp;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       memw_locked(%2, p0) = %3\n"
        "       if !p0 jump 1b\n"
        : "=&r" (tmp),"+m" (*target)
        : "r" (target), "r" (value)
        : "p0");
    return value;
}

/**@ingroup func_qurt_atomic_and
  Bitwise AND operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise AND. 

  @return
  None
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_and(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = and(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target),"r" (mask)
        : "p0");
}

/**@ingroup func_qurt_atomic_and_return
  Bitwise AND operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise AND. 

  @return
  AND result of atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_and_return(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = and(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_or
  Bitwise OR operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise OR. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_or(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = or(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");
}

/**@ingroup func_qurt_atomic_or_return
  Bitwise OR operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise OR. 

  @return
  Returns the OR result of the atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_or_return(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = or(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_xor
  Bitwise XOR operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise XOR.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_xor(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = xor(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");
}

/**@ingroup func_qurt_atomic_xor_return
  Bitwise XOR operation of the atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   Mask for bitwise XOR. 

  @return
  XOR result of atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_xor_return(unsigned int* target, unsigned int mask)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = xor(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_set_bit
  Sets a bit in the atomic variable at a specified position.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to set. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_set_bit(unsigned int *target, unsigned int bit)
{
    unsigned int result;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit % ((unsigned int)sizeof(unsigned int) * 8U);
    unsigned int *wtarget= (unsigned int *)&target[aword];

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = setbit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget), "r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic_clear_bit
  Clears a bit in the atomic variable at a specified position. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to clear.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_clear_bit(unsigned int *target, unsigned int bit)
{
    unsigned int result;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit % ((unsigned int)sizeof(unsigned int) * 8U);
    unsigned int *wtarget= (unsigned int *)&target[aword];

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = clrbit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget), "r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic_change_bit
  Toggles a bit in a atomic variable at a bit position. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to toggle. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_change_bit(unsigned int *target, unsigned int bit)
{
    unsigned int result;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit & 0x1fU;
    unsigned int *wtarget= (unsigned int *)&target[aword];

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = togglebit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget),"r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic_add
  Adds an integer to atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      Integer value to add. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_add(unsigned int *target, unsigned int v)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");
}

/**@ingroup func_qurt_atomic_add_return
  Adds an integer to atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      Integer value to add. 

  @return
  Result of arithmetic sum.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_add_return(unsigned int *target, unsigned int v)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_add_unless
  Adds the delta value to an atomic variable unless the current value in the target 
  matches the unless variable.

  @note1hang The function retries until load lock and store conditional
             are successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      delta  Value to add to the current value.
  @param[in]      unless Perform the addition only when the current value is not 
                         equal to this unless value.
  @return
  TRUE  -- 1 - Addition was performed. \n
  FALSE -- 0 - Addition was not done.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_add_unless(unsigned int* target,
                       unsigned int delta,
                       unsigned int unless)
{
    unsigned int current_val;
    unsigned int new_val;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%3)\n"
        "       p0 = cmp.eq(%0, %5)\n"
        "       if p0 jump 2f\n"
        "       %1 = add(%0, %4)\n"
        "       memw_locked(%3, p0) = %1\n"
        "       if !p0 jump 1b\n"
        "2:\n"
        : "=&r" (current_val),"=&r" (new_val),"+m" (*target)
        : "r" (target), "r" (delta), "r" (unless)
        : "p0");

    return (unsigned int)(current_val != unless);
}

/**@ingroup func_qurt_atomic_sub
  Subtracts an integer from an atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      Integer value to subtract. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_sub(unsigned int *target, unsigned int v)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = sub(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");
}

/**@ingroup func_qurt_atomic_sub_return
  Subtracts an integer from an atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      Integer value to subtract. 

  @return
  Result of arithmetic subtraction.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_sub_return(unsigned int *target, unsigned int v)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = sub(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_inc
  Increments an atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_inc(unsigned int *target)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, #1)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target)
        : "p0");
}

/**@ingroup func_qurt_atomic_inc_return
  Increments an atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  Incremented value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_inc_return(unsigned int *target)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, #1)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_dec
  Decrements an atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_dec(unsigned int *target)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, #-1)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target)
        : "p0");
}

/**@ingroup func_qurt_atomic_dec_return
  Decrements an atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  Decremented value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_dec_return(unsigned int *target)
{
    unsigned int result;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = add(%0, #-1)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic_compare_and_set
  Compares the current value of the atomic variable with the
  specified value and set to a new value when compare is successful.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target  Pointer to the atomic variable.
  @param[in]      old_val Old value to compare.
  @param[in]      new_val New value to set.

  @return
  FALSE -- Specified value is not equal to the current value. \n
  TRUE --Specified value is equal to the current value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned int
qurt_atomic_compare_and_set(unsigned int* target,
                       unsigned int old_val,
                       unsigned int new_val)
{
    unsigned int current_val;

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       p0 = cmp.eq(%0, %3)\n"
        "       if !p0 jump 2f\n"
        "       memw_locked(%2, p0) = %4\n"
        "       if !p0 jump 1b\n"
        "2:\n"
        : "=&r" (current_val),"+m" (*target)
        : "r" (target), "r" (old_val), "r" (new_val)
        : "p0");

    return (unsigned int)(current_val == old_val);
}

/**@ingroup func_qurt_atomic_barrier
  Allows the compiler to enforce an ordering constraint on memory operation issued
  before and after the function.
  
  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic_barrier(void)
{
    __asm__ __volatile__ (
        ""
        :
        :
        :
        "memory");
}


/**@ingroup func_qurt_atomic64_set
  Sets the 64-bit atomic variable with the specified value. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      value  64-bit value to set. 

  @return
  Successfuly set value.

  @dependencies
  None.
*/ 
static inline QURT_INLINE unsigned long long
qurt_atomic64_set(unsigned long long* target, unsigned long long value)
{
    unsigned long long tmp;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       memd_locked(%2, p0) = %3\n"
        "       if !p0 jump 1b\n"
        : "=&r" (tmp),"+m" (*target)
        : "r" (target), "r" (value)
        : "p0");
    return value;
}

/**@ingroup func_qurt_atomic64_and_return
  Bitwise AND operation of a 64-bit atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   64-bit mask for bitwise AND. 

  @return
  AND result of 64-bit atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_and_return(unsigned long long* target, unsigned long long mask)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = and(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_or
  Bitwise OR operation of a 64-bit atomic variable with mask.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   64-bit mask for bitwise OR. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_or(unsigned long long* target, unsigned long long mask)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = or(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");
}

/**@ingroup func_qurt_atomic64_or_return
  Bitwise OR operation of a 64-bit atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   64-bit mask for bitwise OR. 

  @return
  OR result of the atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_or_return(unsigned long long* target, unsigned long long mask)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = or(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_xor_return
  Bitwise XOR operation of 64-bit atomic variable with mask. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      mask   64-bit mask for bitwise XOR. 

  @return
  XOR result of atomic variable with mask.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_xor_return(unsigned long long* target, unsigned long long mask)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = xor(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (mask)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_set_bit
  Sets a bit in a 64-bit atomic variable at a specified position. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to set. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_set_bit(unsigned long long *target, unsigned int bit)
{
    unsigned int result;
    unsigned int *wtarget;
    unsigned int *pwtarget = (unsigned int *)target;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit & 0x1FU;
    wtarget = (unsigned int *)&pwtarget[aword];


    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = setbit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget), "r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic64_clear_bit
  Clears a bit in a 64-bit atomic variable at a specified position. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to clear. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_clear_bit(unsigned long long *target, unsigned int bit)
{
    unsigned int result;
    unsigned int *wtarget;
    unsigned int *pwtarget = (unsigned int *)target;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit & 0x1FU;
    wtarget = (unsigned int *)&pwtarget[aword];

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = clrbit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget), "r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic64_change_bit
  Toggles a bit in a 64-bit atomic variable at a bit position. 

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      bit    Bit position to toggle. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_change_bit(unsigned long long *target, unsigned int bit)
{
    unsigned int result;
    unsigned int *wtarget;
    unsigned int *pwtarget = (unsigned int *)target;
    unsigned int aword = bit / ((unsigned int)sizeof(unsigned int) * 8U); 
    unsigned int sbit = bit & 0x1FU;
    wtarget = (unsigned int *)&pwtarget[aword];

    __asm__ __volatile__(
        "1:     %0 = memw_locked(%2)\n"
        "       %0 = togglebit(%0, %3)\n"
        "       memw_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*wtarget)
        : "r" (wtarget),"r" (sbit)
        : "p0");
}

/**@ingroup func_qurt_atomic64_add
  Adds a 64-bit integer to 64-bit atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      64-bit integer value to add. 

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_add(unsigned long long *target, unsigned long long v)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");
}

/**@ingroup func_qurt_atomic64_add_return
  Adds a 64-bit integer to 64-bit atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      64-bit integer value to add. 

  @return
  Result of arithmetic sum.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_add_return(unsigned long long *target, unsigned long long v)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_sub_return
  Subtracts a 64-bit integer from an atomic variable.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.
  @param[in]      v      64-bit integer value to subtract. 

  @return
  Result of arithmetic subtraction.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_sub_return(unsigned long long *target, unsigned long long v)
{
    unsigned long long result;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = sub(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target), "r" (v)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_inc
  Increments a 64-bit atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_inc(unsigned long long *target)
{
    unsigned long long result;
    unsigned long long inc =1;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target),"r" (inc)
        : "p0");
}

/**@ingroup func_qurt_atomic64_inc_return
  Increments a 64-bit atomic variable by one

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  Incremented value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_inc_return(unsigned long long *target)
{
    unsigned long long result;
    unsigned long long inc =1;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target),"r" (inc)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_dec_return
  Decrements a 64-bit atomic variable by one.

  @note1hang The function retries until load lock and store conditional
             is successful.

  @param[in,out]  target Pointer to the atomic variable.

  @return
  Decremented value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE unsigned long long
qurt_atomic64_dec_return(unsigned long long *target)
{
    unsigned long long result;
    long long minus1 = 0xFFFFFFFFFFFFFFFFLL;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       %0 = add(%0, %3)\n"
        "       memd_locked(%2, p0) = %0\n"
        "       if !p0 jump 1b\n"
        : "=&r" (result),"+m" (*target)
        : "r" (target),"r" (minus1)
        : "p0");

    return result;
}

/**@ingroup func_qurt_atomic64_compare_and_set
  Compares the current value of an 64-bit atomic variable with 
  the specified value and sets to a new value when compare is successful.

  @note1hang The function keep retrying until load lock and store conditional
             is successful.

  @param[in,out]  target  Pointer to the atomic variable.
  @param[in]      old_val 64-bit old value to compare.
  @param[in]      new_val 64-bit new value to set.

  @return
  FALSE -- Specified value is not equal to the current value. \n
  TRUE -- Specified value is equal to the current value.
  
  @dependencies
  None.
*/
static inline QURT_INLINE int
qurt_atomic64_compare_and_set(unsigned long long *target,
                       unsigned long long old_val,
                       unsigned long long new_val)
{
    unsigned long long current_val;

    __asm__ __volatile__(
        "1:     %0 = memd_locked(%2)\n"
        "       p0 = cmp.eq(%0, %3)\n"
        "       if !p0 jump 2f\n"
        "       memd_locked(%2, p0) = %4\n"
        "       if !p0 jump 1b\n"
        "2:\n"
        : "=&r" (current_val),"+m" (*target)
        : "r" (target), "r" (old_val), "r" (new_val)
        : "p0");

    return (int)(current_val == old_val);
}

/**@ingroup func_qurt_atomic64_barrier
  Allows compiler to enforce an ordering constraint on memory operation issued
  before and after the function.

  @return
  None.
  
  @dependencies
  None.
*/
static inline QURT_INLINE void
qurt_atomic64_barrier(void)
{
    /** @cond */
    __asm__ __volatile__ (
        ""
        :
        :
        :
        "memory");
    /** @endcond */
}

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ATOMIC_OPS_H */
