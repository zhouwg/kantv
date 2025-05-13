#ifndef QURT_QDI_DRIVER_H
#define QURT_QDI_DRIVER_H

/**
  @file qurt_qdi_driver.h
  @brief  Definitions, macros, and prototypes used when writing a
  QDI driver.

 EXTERNALIZED FUNCTIONS
  None

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

 Copyright (c) 2018, 2019-2021, 2023 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#include "stddef.h"
#include "qurt_qdi.h"
#include "qurt_types.h"
#include "qurt_callback.h"
#include "qurt_qdi_constants.h"
#include "qurt_qdi_imacros.h"
#include "qurt_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
||  This gives the canonical form for the arguments to a QDI
||   driver invocation function.  The arguments are as follows:
||
||   int client_handle    (R0) QDI handle that represents the client
||                             that made this QDI request. If the
||                             client is remote, this is a
||                             variable handle; if the client is local
||                             (same thread and process), this is
||                             set to QDI_HANDLE_LOCAL_CLIENT.
||
||   qurt_qdi_obj_t *obj  (R1) Points at the qdi_object_t structure
||                             on which this QDI request is being made.
||                             The qdi_object_t structure is usually
||                             the first element of a larger structure
||                             that contains state associated with the
||                             object; because it is usually the first
||                             element, the object pointers can be freely
||                             interchanged through casts.
||
||   int method           (R2) Integer QDI method that represents
||                             the request type.
||
||   qurt_qdi_arg_t arg1  (R3) First three general purpose arguments
||   qurt_qdi_arg_t arg2  (R4)  to the invocation function are passed in
||   qurt_qdi_arg_t arg3  (R5)  these slots.
||
||   qurt_qdi_arg_t arg4  (SP+0)  Arguments beyond the first three are
||   qurt_qdi_arg_t arg5  (SP+4)  passed on the stack.
||   qurt_qdi_arg_t arg6  (SP+8)
||   qurt_qdi_arg_t arg7  (SP+12)
||   qurt_qdi_arg_t arg8  (SP+16)
||   qurt_qdi_arg_t arg9  (SP+20)
||
||  The canonical form of the invocation function takes a
||   total of 12 arguments, but not all of them are used.  In general,
||   the QDI infrastructure only passes those arguments provided by
||   the caller; if the invocation function accesses additional
||   arguments beyond those provided by the caller, the values are not
||   useful.
*/
/** @cond */
#define QDI_INVOKE_ARGS \
    int, struct qdiobj *, int, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t

#define QDI_EXT_INVOKE_ARGS \
    int, qurt_qdi_man_obj_t*, int, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t, \
    qurt_qdi_arg_t, qurt_qdi_arg_t, qurt_qdi_arg_t

#define BUFFER_LOCK 1
#define BUFFER_UNLOCK 0 

struct qdiobj;
/** @endcond */
/** @addtogroup driver_support_types
@{ */
typedef union {
    void *ptr; /**< Pointer to the driver handle. */
    int num;   /**< Method number. */
} qurt_qdi_arg_t;
/** @} */ /* end_addtogroup driver_support_types */
/** @cond */
/** QuRT QDI driver version */
typedef union {
    int num;
    struct {
        short major; /** Driver major version number. */
        short minor; /** Driver minor version number. */
    };
} qurt_qdi_version_t;

typedef int (*qurt_qdi_pfn_invoke_t)(QDI_INVOKE_ARGS);
typedef void (*qurt_qdi_pfn_release_t)(struct qdiobj *);
/** @endcond */
/** @addtogroup driver_support_types
@{ */
typedef struct qdiobj {
    qurt_qdi_pfn_invoke_t invoke;   /**< Invocation function that implements the driver methods.*/
    int refcnt;                     /**< Reference count, an integer value maintained by the QDI infrastructure that tracks the number of
                                         references to a driver instance. */
    qurt_qdi_pfn_release_t release; /**< Release function that performs details associated with deleting an instance
                                         of the driver object.*/
} qurt_qdi_obj_t;
/** @} */ /* end_addtogroup driver_support_types */
/** @cond */
/** QuRT QDI managed object */
typedef struct qurt_qdi_man_obj
{
    qurt_qdi_obj_t qdi_obj;
    union
    {
        struct qurt_qdi_ext_driver * opener_obj;
        struct qurt_qdi_ext_device * device_obj;
    };
}qurt_qdi_man_obj_t;

typedef int (*qurt_qdi_ext_pfn_create_t)(int client_id, const char *name, qurt_qdi_version_t version, qurt_qdi_man_obj_t **qdi_obj);
typedef int (*qurt_qdi_ext_pfn_create_device_t)(int client_id, const char *name, qurt_qdi_version_t version, struct qurt_qdi_ext_device * device, qurt_qdi_man_obj_t **qdi_obj);
typedef int (*qurt_qdi_ext_pfn_invoke_t)(QDI_EXT_INVOKE_ARGS);
typedef void (*qurt_qdi_ext_pfn_destroy_t)(qurt_qdi_man_obj_t *qdi_obj);
typedef int (*qurt_qdi_ext_pfn_probe_t)(void *handle, struct qurt_qdi_ext_device **device);

typedef struct qurt_qdi_ext_obj_info{
    qurt_qdi_man_obj_t *obj;
    int qdi_client_id;
    struct qurt_qdi_ext_obj_info *next;
}qurt_qdi_ext_obj_info_t;
typedef struct qurt_qdi_ext_obj_info *qurt_qdi_ext_obj_info_ptr;

/** QuRT QDI device */
//temporarily add this back while there are still drivers who statically define this structure
struct qurt_qdi_device {
    qurt_qdi_obj_t opener_obj;
    const char* name;
    char island_resident;
    unsigned char singleton;
    qurt_qdi_ext_pfn_create_t create;
    qurt_qdi_ext_pfn_invoke_t invoke;
    qurt_qdi_ext_pfn_destroy_t destroy;
    qurt_mutex_t qurt_qdi_ext_list_lock;
    qurt_qdi_ext_obj_info_ptr qurt_qdi_ext_obj_info_head;
};
typedef struct qurt_qdi_device qurt_qdi_man_device;

struct qurt_qdi_ext_driver {
    qurt_qdi_obj_t opener_obj;
    const char* name;
    char island_resident;
    unsigned char singleton;
    qurt_qdi_ext_pfn_create_t create;
    qurt_qdi_ext_pfn_invoke_t invoke;
    qurt_qdi_ext_pfn_destroy_t destroy;
    qurt_mutex_t qurt_qdi_ext_list_lock;
    qurt_qdi_ext_obj_info_ptr qurt_qdi_ext_obj_info_head;
    qurt_qdi_ext_pfn_create_device_t create_device;
    qurt_qdi_version_t version;
    qurt_qdi_ext_pfn_probe_t probe;
    const char* compatible;
    struct qurt_qdi_ext_device * device_list;
    //qurt_qdi_ext_device_ptr device_list;
};
typedef struct qurt_qdi_ext_driver qurt_qdi_ext_driver_t;
//above replaces qurt_qdi_man_device

extern int qurt_qdi_obj_ref_inc(qurt_qdi_obj_t *);
extern int qurt_qdi_obj_ref_dec(qurt_qdi_obj_t *);

extern int qurt_qdi_ext_opener (QDI_INVOKE_ARGS);
/** @endcond */
/**@ingroup func_qurt_qdi_method_default
  Processes a method that is unrecognized or unsupported in the driver invocation function.
  All arguments passed to the current invocation function (Section @xref{sec:invocationFunction}) must be forwarded
  to this function.

  @note1hang Invocation functions must process all unrecognized or unsupported methods
             by calling this function.

  @return
  None.

  @dependencies
  None.
*/
extern int qurt_qdi_method_default(QDI_INVOKE_ARGS);

/**@ingroup func_qurt_qdi_handle_create_from_obj_t
  Allocates a new device handle for use with the specified driver object.
   
  @param[in] client_handle  Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[out] obj           Pointer to the driver object.

  @return
  Non-negative integer -- Success; this value is the new handle. \n
  Negative value -- Error.

  @dependencies
  None.
*/
static __inline int qurt_qdi_handle_create_from_obj_t(int client_handle, qurt_qdi_obj_t *obj)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_HANDLE_CREATE_FROM_OBJ_T,
                                    obj);
}

/**@ingroup func_qurt_qdi_handle_invoke
  Allocates a new island device handle for use with the specified driver object.
   
  @param[in] client_handle Client handle obtained from the current invocation function (Section 3.4.1).
  @param[in] obj           Pointer.

  @return
  Non-negative integer value that is the new handle -- Success. \n
  Negative return value -- Error.

  @dependencies
  None.
*/
static __inline int qurt_qdi_island_handle_create_from_obj_t(int client_handle, qurt_qdi_obj_t *obj)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_ISLAND_HANDLE_CREATE_FROM_OBJ_T,
                                    obj);
}

/**@ingroup func_qurt_qdi_handle_release
  Deallocates the specified device handle.

  @param[in] client_handle     Obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[in] handle_to_release Handle to release.

  @return 
  0 -- Success. \n
  Negative value -- Error. 

  @dependencies
  None.
*/
static __inline int qurt_qdi_handle_release(int client_handle, int handle_to_release)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_HANDLE_RELEASE,
                                    handle_to_release);
}

static __inline qurt_qdi_obj_t *
qurt_qdi_objref_get_from_handle(int client_handle, int object_handle)
{
    qurt_qdi_obj_t *ret;

    ret = NULL;

    qurt_qdi_handle_invoke(client_handle,
                            QDI_CLIENT_HANDLE_OBJREF_GET,
                            object_handle,
                            &ret);

    return ret;
}

/**@ingroup func_qurt_client_add_memory
  Adds a physical address range to the HLOS physpool of the caller user PD.
   
  @param[in] client_handle  Obtained from the current invocation function (Section 3.4.1).
  @param[in] phys_addr      Starting address of the physical address range. 
  @param[in] size           Size.

  @return
  #QURT_EOK -- Pages successfully added.

  @dependencies
  None.
*/
int qurt_client_add_memory(int client_handle, qurt_addr_t phys_addr, qurt_size_t size);

/**@ingroup func_qurt_client_add_memory2
  Adds a physical address range to the HLOS physpool of the caller user PD.
   
  @param[in] client_handle  Obtained from the current invocation function (Section 3.4.1).
  @param[in] phys_addr      Starting 36-bit address of the physical address range. 
  @param[in] size           Size.

  @return
  #QURT_EOK -- Pages successfully added.

  @dependencies
  None.
*/
int qurt_client_add_memory2(int user_client_handle, qurt_paddr_64_t phys_addr, qurt_size_t size);

static __inline qurt_qdi_obj_t *
qurt_qdi_objref_get_from_pointer(qurt_qdi_obj_t *objptr)
{
    qurt_qdi_obj_t * ret = NULL;

    if (qurt_qdi_obj_ref_inc(objptr) < 0) {
        ret = NULL;
    } else {
        ret = objptr;
    }

    return ret;
}

static __inline void
qurt_qdi_objref_release(qurt_qdi_obj_t *objptr)
{
    if (qurt_qdi_obj_ref_dec(objptr) == 1) {
        (*objptr->release)(objptr);
    }
}

/**@ingroup func_qurt_qdi_copy_from_user
  Copies the contents of a user memory buffer into the current driver.

  @note1hang User buffer addresses are valid only for the duration of the current driver
  invocation.

  @param[in] client_handle   Obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[in] dest            Base address of the driver buffer.
  @param[in] src             Base address of the user buffer.
  @param[in] len             Number of bytes to copy.
  
  @return
  Negative value -- Indicates a privilege or security violation, the copy operation 
                has crossed a privilege boundary.
  
  @dependencies
  None.
*/
static __inline int qurt_qdi_copy_from_user(int client_handle, void *dest, const void *src, unsigned len)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_COPY_FROM_USER,
                                    dest, src, len);
}

/**@ingroup qurt_qdi_copy_string_from_user
  Copies the contents of a user memory buffer into the current driver.

  @note1hang User buffer addresses are valid only for the duration of the current driver
  invocation.

  @param client_handle   Obtained from the current invocation function (Section 3.4.1).
  @param dest            Base address of the driver buffer.
  @param src             Base address of the user buffer.
  @param len             Number of bytes to copy. NOTE: This is the destination buffer length.
  
  @return
  Negative error result -- privilege or security violation, the copy operation 
                has crossed a privilege boundary.
  
  @dependencies
  None.
*/
int qurt_qdi_copy_string_from_user(int client_handle, char *dest, const char *src, unsigned len);

/**@ingroup func_qurt_qdi_copy_to_user
  Copies the contents of a driver memory buffer to user memory.

  @note1hang User buffer addresses are valid only for the duration of the current driver
             invocation.

  @param[in] client_handle Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[in] dest          Base address of the user buffer.
  @param[in] src           Base address of the driver buffer.
  @param[in] len           Number of bytes to copy.

  @return
  Negative value -- Indicates a privilege or security violation, the copy operation has crossed a 
                    privilege boundary

  @dependencies
  None.
*/
static __inline int qurt_qdi_copy_to_user(int client_handle, void *dest, const void *src, unsigned len)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_COPY_TO_USER,
                                    dest, src, len);
}

/**@ingroup func_qurt_qdi_safe_cache_ops
  Do cache operations on user memory

  @note1hang User buffer addresses are valid only for the duration of the current driver
             invocation.

  @param[in] client_handle Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[in] addr          Base address of the user memory.
  @param[in] size          Size of the user memory.
  @param[in] opcode        Cache operations (QURT_MEM_CACHE_FLUSH, QURT_MEM_CACHE_INVALIDATE...)
  @param[in] type          Cache type (QURT_MEM_ICACHE, QURT_MEM_DCACHE)

  @return
  Negative value -- Indicates a privilege or security violation, the copy operation has crossed a
                    privilege boundary

  @dependencies
  None.
*/
static __inline int qurt_qdi_safe_cache_ops(int client_handle, qurt_addr_t addr, qurt_size_t size,
        qurt_mem_cache_op_t opcode, qurt_mem_cache_type_t type)
{
    return qurt_qdi_handle_invoke(client_handle,
                                  QDI_CLIENT_HANDLE_SAFE_CACHE_OPS,
                                  addr, size, opcode, type);
}


/**@ingroup func_qurt_qdi_buffer_lock
  Prepares for the direct manipulation of a potentially untrusted buffer provided by a QDI
  client.

  This function is used to permit a trusted driver to safely access memory that is
  provided by a potentially untrusted client. A driver calls this function to obtain a safe buffer
  pointer for accessing the memory.

  This function performs the following security checks: \n
  - Verifies that the entire buffer is accessible to the client. \n
  - Ensures that the pointer remains valid for the remainder of the QDI driver
      operation. \n

  @note1hang  User buffer addresses are valid only for the duration of the current driver
              invocation.

  @param[in] client_handle Obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param[in] buf           Pointer to the base address of the client buffer address.
  @param[in] len           Buffer length (in bytes).
  @param[in] perms         Bitmask value that specifies the read or write access to perform on the
                       client buffer: \n
                           - #QDI_PERM_R -- Read access \n
                           - #QDI_PERM_W -- Write access \n
                           - #QDI_PERM_RW -- Read/write access @tablebulletend
  @param[out] obuf     Pointer to the buffer address that the driver must use to access the buffer.

  @return
  Negative value -- Error; the operation crosses a privilege boundary, indicating a privilege or security violation. \n
  Nonzero value -- User passed a buffer that does not fulfill the requested read/write access permission.
                    In this case the QDI driver call must be terminated cleanly, with an appropriate error code 
                    returned to the client. \n
  Zero -- Success; when this occurs the QDI driver must use the pointer at *obuf to access memory, and not the
                    pointer passed in as buf -- even if the user process changes the mapping of memory at buf,
                   the mapping of memory at *obuf remains valid until the driver invocation completes.

  @dependencies
  None.
*/
static __inline int qurt_qdi_buffer_lock(int client_handle, void *buf, unsigned len,
                                         unsigned perms, void **obuf)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_BUFFER_LOCK,
                                    buf, len, perms, obuf);
}

/**@ingroup func_qurt_qdi_buffer_lock2
   Prepares for the direct manipulation of a possibly-untrusted buffer provided by a QDI
   client.
   This API permits a trusted driver to safely access memory 
   provided by a possibly-untrusted client. A driver calls this function to obtain a safe buffer
   pointer for accessing the memory.
   This function performs the following security checks: \n
   -- Entire buffer is accessible to the client. \n
   -- Entire buffer is mapped with permissions passed in perms field \n
   -- Entire buffer is physically contiguous \n
   In addition to the security checks, the API also locks the client mapping such that the client
   cannot remove the mapping while the physical memory is used by the trusted
   driver. \n

   @note1      Drivers are responsible for calling qurt_qdi_buffer_unlock() at appropriate time. Not 
               pairing qurt_qdi_buffer_unlock() with this API leads to resource leakages and 
               process exit failures. Drivers can keep track of which buffers are locked for
               a particular client. If the client exits abruptly, the buffers can be
               unlocked on driver release invocation for the exiting client.

   @note2      This API is supported in limited capacity when called from Island mode. Safe buffer
               unmapping or user buffer unlock is not supported in Island mode.

   @param client_handle Obtained from the current invocation function (Section 3.4.1).
   @param buf           Pointer to the base address of the client buffer address.
   @param len           Buffer length (in bytes).
   @param perms         Bitmask value that specifies the read or write access to perform on the
                        client buffer: \n
                        -- #QDI_PERM_R -- Read access \n
                        -- #QDI_PERM_W -- Write access \n
                        -- #QDI_PERM_RW -- Read/write access \n
   @param obuf         Optional parameter that returns a pointer to the buffer address that 
                       the driver must use to access the buffer. If NULL is passed, the API 
                       only performs security checks and does not create a mapping to access the user buffer in
                       a safe way.

   @return
   QURT_EINVALID   -- Arguments passed to the API are invalid. User buffer pointer is NULL or length of the
                      buffer is 0. \n
   QURT_EPRIVILEGE -- One of the security checks on the user buffer failed. \n
   QURT_EFAILED    -- Mapping cannot be created for the trusted driver. \n
   QURT_EOK        -- Lock operation was successful. When this occurs, the QDI driver must use the 
                      pointer at *obuf to perform its memory accesses, and not the
                      pointer passed in as buf. 
                      
   @dependencies
   None.
*/
static __inline int qurt_qdi_buffer_lock2(int client_handle, void *buf, unsigned len,
                                         unsigned perms, void **obuf)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_BUFFER_LOCK2,
                                    BUFFER_LOCK, buf, len, perms, obuf);
}

/**@ingroup func_qurt_qdi_buffer_unlock
   This API is paired with qurt_qdi_buffer_lock2(). A temporary overlapping mapping 
   created for the driver is removed. Client mapping for the user buffer is
   unlocked. 

   @note1      Drivers are responsible for pairing this with qurt_qdi_buffer_lock(). Not 
               pairing qurt_qdi_buffer_lock() with this API leads to resource leakages and 
               process exit failures. Drivers can keep track of which buffers are locked for
               a particular client, and if the client exits abruptly, all the buffers can be
               unlocked on driver release invocation for the exiting client.

   @note2      This API is supported in limited capacity when called from Island mode. Actual
               unmapping of driver accessible memory or unlocking of the buffer is not
               supported in Island bode.

   @param client_handle Obtained from the current invocation function (Section 3.4.1).
   @param buf           Pointer to the base address of the client buffer address.
   @param len           Buffer length (in bytes).
   @param obuf          Safe buffer address that was returned in the obuf field after calling
                        qurt_qdi_buffer_lock2().

   @return
   QURT_EINVALID   -- Arguments passed to the API are invalid. User buffer pointer is NULL or length of the
                      buffer is 0. \n
   QURT_EOK        -- Lock operation was successful. When this occurs, the QDI driver must use the 
                      pointer at *obuf to perform its memory accesses, and not the
                      pointer passed in as buf. \n
   other results   -- Safe buffer unmapping failed or unlocking of user buffer failed \n.

   @dependencies
   None.
*/
static __inline int qurt_qdi_buffer_unlock(int client_handle, void *buf, unsigned len,
                                           void *obuf)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_BUFFER_LOCK2,
                                    BUFFER_UNLOCK, buf, len, obuf);
}

/**@ingroup func_qurt_qdi_user_malloc
  Allocates memory area in the QDI heap that is read/write accessible to both the driver and
  the client. \n
  @note1hang The QDI heap has a limited amount of memory available, and only the
  device driver can free the allocated memory.

  @param client_handle Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param size          Size.

  @return
  Non-zero -- Success; this returned value points to the allocated memory area. \n
  Zero -- Error.

  @dependencies
  None.
*/
void *qurt_qdi_user_malloc(int client_handle, unsigned size);

/**@ingroup func_qurt_qdi_user_free
  Deallocates memory area in the QDI heap.

  @param client_handle Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param ptr Pointer.

  @dependencies
  None.
*/
void qurt_qdi_user_free(int client_handle, void *ptr);

/**@ingroup funct_qurt_qdi_client_detach
  Detaches a client (a process), indicating that the client does not
  participate in the qurt_wait() mechanism. This behavior
  is opt-in and irrevocable. When a client is detached, it can
  not be un-detached.

  @param client_handle Handle of the client to detach.

  @return
  Zero -- Success.  Detachable clients always return success.
  Nonzero value -- client_handle did not refer to a
    detachable user client.

  @dependencies
  None.
*/
static __inline int qurt_qdi_client_detach(int client_handle)
{
    return qurt_qdi_handle_invoke(client_handle, QDI_CLIENT_HANDLE_DETACH);
}

/**@ingroup func_qurt_qdi_signal_group_create
  Creates a new signal group for use in a device driver.
  A QDI signal group contains up to 32 signals, which can be operated on either
  individually (using the qurt_qdi_signal_* functions) or as a group (using the
  qurt_qdi_signal_group_* functions). \n
  @note1hang Driver implementation is responsible for using the proper signal group
             handle in any given situation. \n
  For more information on signals, see the Hexagon QuRT RTOS User Guide (80-VB419-78).

  @param client_handle                 Client handle obtained from the current invocation function (Section @xref{sec:invocationFunction}).
  @param p_signal_group_handle_local   Returns a handle intended for use by code that
                                       resides in the same context and process as the created signal group
                      (for example, the device driver implementation that allocated the 
                      signal group).
  @param p_signal_group_handle_remote  Returns a handle intended for use by code
                                       that resides in a different context and process than the created signal group 
                      (for example, the user-mode client of an OS driver).

  @return
  Zero return value indicates success.\n
  Negative return value indicates could not create signal group.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_group_create(int client_handle,
                                                 int *p_signal_group_handle_local,
                                                 int *p_signal_group_handle_remote)
{
    return qurt_qdi_handle_invoke(client_handle,
                                    QDI_CLIENT_HANDLE_SIGNAL_GROUP_CREATE,
                                    p_signal_group_handle_local,
                                    p_signal_group_handle_remote);
}

/**@ingroup func_qurt_qdi_signal_group_wait
  Suspends the current thread until any of the signals are set in the specified signal group.

  If a signal is set in a signal group object, and a thread waits on the signal group object,
  the thread is awakened. If the awakened thread has higher priority than the current
  thread, a context switch can occur.

  @param signal_group_handle   Handle of the signal group.

  @return
  If the client is remote:
  QURT_EOK -- Wait complete \n
  QURT_ECANCEL -- Wait cancelled.\n
  If the client is local, returns a 32-bit word with current signals.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_group_wait(int signal_group_handle)
{
    return qurt_qdi_handle_invoke(signal_group_handle,
                                    QDI_SIGNAL_GROUP_WAIT);
}

/**@ingroup func_qurt_qdi_signal_group_poll
  Returns a value that indicates if any of the signals are set in the specified signal group.

  @param signal_group_handle Handle of the signal group.

  @return
  1 -- Indicates whether any of the signals are set in the signal group.\n
  0 -- Indicates that none of the signals are set.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_group_poll(int signal_group_handle)
{
    return qurt_qdi_handle_invoke(signal_group_handle,
                                    QDI_SIGNAL_GROUP_POLL);
}


/**@ingroup func_qurt_qdi_signal_create
  Creates a new signal in the specified signal group.
  For more information on signals, see the Hexagon QuRT RTOS User Guide (80-VB419-78).

  @note1hang Driver implementation is responsible for using the proper signal handle in
             any given situation.

  @param signal_group_handle    Handle of an existing signal group.
  @param p_signal_handle_local  Returns a handle intended for use by code that resides in
                               the same context and process as the created signal (for example,
                               the device driver implementation that allocated the signal).
  @param p_signal_handle_remote Returns a handle intended for use by code that resides in
                               a different context and process than the created signal (for 
                               example, the user-mode client of an OS driver).

  @return 
  Nonzero value -- No more signals can be created in the specified
                    signal group. 

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_create(int signal_group_handle,
                                           int *p_signal_handle_local,
                                           int *p_signal_handle_remote)
{
    return qurt_qdi_handle_invoke(signal_group_handle,
                                    QDI_SIGNAL_GROUP_SIGNAL_CREATE,
                                    p_signal_handle_local,
                                    p_signal_handle_remote);
}

/**@ingroup func_qurt_qdi_signal_set
  Sets the signal in the specified signal object.

  @param signal_handle Handle of the signal.

  @return
  Always returns 0.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_set(int signal_handle)
{
    return qurt_qdi_handle_invoke(signal_handle,
                                    QDI_SIGNAL_SET);
}

/**@ingroup func_qurt_qdi_signal_clear
  Clears the signal in the specified signal object.

  @param signal_handle   Handle of the signal.
  
  @return 
  Always returns 0.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_clear(int signal_handle)
{
    return qurt_qdi_handle_invoke(signal_handle,
                                    QDI_SIGNAL_CLEAR);
}

/**@ingroup func_qurt_qdi_signal_wait
  Suspends the current thread until the specified signal is set.
  If a signal is set in a signal object, and a thread waits on the signal object, the
  thread is awakened. If the awakened thread has higher priority than the current thread, a
  context switch may occur.

  @param signal_handle Handle of the signal.

  @return
  If client is remote:
  QURT_EOK -- Wait complete. \n
  QURT_ECANCEL -- Wait cancelled.\n
  If client is local, return a 32-bit word with current signals.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_wait(int signal_handle)
{
    return qurt_qdi_handle_invoke(signal_handle,
                                    QDI_SIGNAL_WAIT);
}

/**@ingroup func_qurt_qdi_signal_poll
  Returns a value that indicates if the specified signal is set.

  @param signal_handle Handle of the signal.

  @return
  1 -- Signal is set. \n
  0 -- Signal is not set.

  @dependencies
  None.
*/
static __inline int qurt_qdi_signal_poll(int signal_handle)
{
    return qurt_qdi_handle_invoke(signal_handle,
                                    QDI_SIGNAL_POLL);
}

/**@ingroup func_qurt_qdi_devname_register
  Registers a QDI device with the generic QDI object in the 
  current QDI context.

  This function registers an exact name or a directory prefix with a QDI opener object.
  Future invocations of qurt_qdi_open() in the context of the caller invokes the
  opener object if a match is detected.

  Directory prefix names are specified by ending the name with a forward slash character.

  Example of an exact name:
  @code qurt_qdi_devname_register(/dev/foobar, foobar_opener);@endcode

  Example of a directory prefix:
  @code qurt_qdi_devname_register(/pipedev/, pipedev_opener);@endcode

  Given the two registrations shown above, the only qurt_qdi_open() requests to
  direct to the foobar_opener object are requests for the exact name
  "/dev/foobar", Any request beginning with "/pipedev/" is directed to the
  pipedev_opener object.

  The pipedev invocation function presumably examines the name argument to
  determine exactly how to handle the request. The name is passed to the invocation
  function in the a1.ptr argument (Section @xref{sec:invocationFunction}).

  @param  name   Device name or device name prefix.
  @param  opener Pointer to the opener object for the device.
 
  @return
  0 -- Device was successfully registered. \n
  Negative error code -- Device was not registered.

  @dependencies
  None.
 */
static __inline int qurt_qdi_devname_register(const char *name,
                                              qurt_qdi_obj_t *opener)
{
    return qurt_qdi_handle_invoke(QDI_HANDLE_GENERIC,
                                    QDI_DEVNAME_REGISTER,
                                    name,
                                    opener);
}

// Macros for backward compatibility with deprecated APIs
//  (These will go away soon)

#define qurt_qdi_register_devname(name, opener) \
        qurt_qdi_devname_register((name), (void *)(opener))
#define qurt_qdi_new_handle_from_obj_t(handle, obj) \
        qurt_qdi_handle_create_from_obj_t((handle), (obj))
#define qurt_qdi_release_handle(client_handle, handle) \
        qurt_qdi_handle_release((client_handle), (handle))
#define qurt_qdi_lock_buffer(handle, buf, len, perms, obuf) \
        qurt_qdi_buffer_lock((handle), (buf), (len), (perms), (obuf))
#define qurt_qdi_usermalloc(handle, size) \
        qurt_qdi_user_malloc((handle), (size))
#define qurt_qdi_userfree(handle, ptr) \
        qurt_qdi_user_free((handle), (ptr))
        
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
