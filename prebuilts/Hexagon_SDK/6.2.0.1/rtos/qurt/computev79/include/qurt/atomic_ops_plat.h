#ifndef ATOMIC_OPS_PLAT_H
#define ATOMIC_OPS_PLAT_H
/**
  @file atomic_ops_plat.h 

  @brief  Prototypes of atomic operations API backwards compatible.

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2013, 2023 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/


#include <qurt_atomic_ops.h>

#ifdef __cplusplus
extern "C" {
#endif
/*=============================================================================
                      CONSTANTS AND MACROS                                
=============================================================================*/
#define atomic_set(a,b)                qurt_atomic_set((unsigned int *)(a),(unsigned int)(b))
#define atomic_and(a,b)                qurt_atomic_and((unsigned int *)(a),(unsigned int)(b))
#define atomic_and_return(a,b)         qurt_atomic_and_return((unsigned int *)(a),(unsigned int)(b))
#define atomic_or(a,b)                 qurt_atomic_or((unsigned int *)(a),(unsigned int)(b))
#define atomic_or_return(a,b)          qurt_atomic_or_return((unsigned int *)(a),(unsigned int)(b))
#define atomic_xor(a,b)                qurt_atomic_xor((unsigned int *)(a),(unsigned int)(b))
#define atomic_xor_return(a,b)         qurt_atomic_xor_return((unsigned int *)(a),(unsigned int)(b)) 
#define atomic_set_bit(a,b)            qurt_atomic_set_bit((unsigned int *)(a),(unsigned int)(b))
#define atomic_clear_bit(a,b)          qurt_atomic_clear_bit((unsigned int *)(a),(unsigned int)(b))
#define atomic_change_bit(a,b)         qurt_atomic_change_bit((unsigned int *)(a),(unsigned int)(b))
#define atomic_add(a,b)                qurt_atomic_add((unsigned int *)(a),(unsigned int)(b))
#define atomic_add_return(a,b)         qurt_atomic_add_return((unsigned int *)(a),(unsigned int)(b))
#define atomic_add_unless(a,b,c)       qurt_atomic_add_unless((unsigned int *)(a),(unsigned int)(b),(unsigned int)(c))
#define atomic_sub(a,b)                qurt_atomic_sub((unsigned int *)(a),(unsigned int)(b))
#define atomic_sub_return(a,b)         qurt_atomic_sub_return((unsigned int *)(a),(unsigned int)(b))
#define atomic_inc(a)                  qurt_atomic_inc((unsigned int *)(a))
#define atomic_inc_return(a)           qurt_atomic_inc_return((unsigned int *)(a))
#define atomic_dec(a)                  qurt_atomic_dec((unsigned int *)(a))
#define atomic_dec_return(a)           qurt_atomic_dec_return((unsigned int *)(a))
#define atomic_compare_and_set(a,b,c)  qurt_atomic_compare_and_set((unsigned int *)(a),(unsigned int)(b),(unsigned int)(c))
#define atomic_barrier                 qurt_atomic_barrier
#define atomic_barrier_write           qurt_atomic_barrier_write
#define atomic_barrier_write_smp       qurt_atomic_barrier_write_smp
#define atomic_barrier_read_smp        qurt_atomic_barrier_read_smp
#define atomic_barrier_smp             qurt_atomic_barrier_smp

/*============================
 *       64 bits support
 *============================ */
#define atomic64_set(a,b)                qurt_atomic64_set((unsigned long long *)(a),(unsigned long long)(b))  
#define atomic64_and(a,b)                qurt_atomic64_and((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_and_return(a,b)         qurt_atomic64_and_return((unsigned long long *)(a),(unsigned long long)(b))  
#define atomic64_or(a,b)                 qurt_atomic64_or((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_or_return(a,b)          qurt_atomic64_or_return((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_xor(a,b)                qurt_atomic64_xor((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_xor_return(a,b)         qurt_atomic64_xor_return((unsigned long long *)(a),(unsigned long long)(b))  
#define atomic64_set_bit(a,b)            qurt_atomic64_set_bit((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_clear_bit(a,b)          qurt_atomic64_clear_bit((unsigned long long *)(a),(unsigned long long)(b))  
#define atomic64_change_bit(a,b)         qurt_atomic64_change_bit((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_add(a,b)                qurt_atomic64_add((unsigned long long *)(a),(unsigned long long)(b))  
#define atomic64_add_return(a,b)         qurt_atomic64_add_return((unsigned long long *)(a),(unsigned long long)(b))
#define atomic64_sub(a,b)                qurt_atomic64_sub((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_sub_return(a,b)         qurt_atomic64_sub_return((unsigned long long *)(a),(unsigned long long)(b)) 
#define atomic64_inc(a)                  qurt_atomic64_inc((unsigned long long *)(a))
#define atomic64_inc_return(a)           qurt_atomic64_inc_return((unsigned long long *)(a))
#define atomic64_dec(a)                  qurt_atomic64_dec((unsigned long long *)(a))
#define atomic64_dec_return(a)           qurt_atomic64_dec_return((unsigned long long *)(a))
#define atomic64_compare_and_set(a,b,c)  qurt_atomic64_compare_and_set((unsigned long long  *)(a),(unsigned long long )(b),(unsigned long long )(c))
#define atomic64_barrier                 qurt_atomic64_barrier
#define atomic64_barrier_write           qurt_atomic64_barrier_write
#define atomic64_barrier_write_smp       qurt_atomic64_barrier_write_smp
#define atomic64_barrier_read_smp        qurt_atomic64_barrier_read_smp
#define atomic64_barrier_smp             qurt_atomic64_barrier_smp

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* ATOMIC_OPS_PLAT_H */
