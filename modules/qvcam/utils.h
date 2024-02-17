#ifndef QVCAM_UTILS_H
#define QVCAM_UTILS_H

#include <linux/types.h>
#include <linux/version.h>

#define UNUSED(x) (void)(x)
#define QVCAM_MAX_STRING_SIZE 1024

#define qvcam_min(value1, value2) \
    ((value1) < (value2)? (value1): (value2))

#define qvcam_max(value1, value2) \
    ((value1) > (value2)? (value1): (value2))

#define qvcam_abs(value) \
    ((value) < 0? -(value): (value))

#define qvcam_between(min, value, max) \
    ((value) >= (min) && (value) <= (max))

#define qvcam_bound(min, value, max) \
    ((value) < (min)? (min): (value) > (max)? (max): (value))

#define qvcam_align_up(value, align) \
    (((value) + (align) - 1) & ~((align) - 1))

#define qvcam_align32(value) qvcam_align_up(value, 32)

#define qvcam_mod(value, mod) \
    (((value) % (mod) + (mod)) % (mod))

#define qvcam_signal(class, signal, ...) \
    typedef int (*qvcam_##class##_##signal##_proc)(void *user_data, __VA_ARGS__); \
    \
    typedef struct \
    { \
        void *user_data; \
        qvcam_##class##_##signal##_proc callback; \
    } qvcam_##class##_##signal##_callback, *qvcam_##class##_##signal##_callback_t; \
    \
    void qvcam_##class##_set_##signal##_callback(qvcam_##class##_t self, \
                                                  const qvcam_##class##_##signal##_callback callback)

#define qvcam_signal_no_args(class, signal) \
    typedef int (*qvcam_##class##_##signal##_proc)(void *user_data); \
    \
    typedef struct \
    { \
        void *user_data; \
        qvcam_##class##_##signal##_proc callback; \
    } qvcam_##class##_##signal##_callback, *qvcam_##class##_##signal##_callback_t; \
    \
    void qvcam_##class##_set_##signal##_callback(qvcam_##class##_t self, \
                                                  const qvcam_##class##_##signal##_callback callback)

#define qvcam_signal_callback(class, signal) \
    qvcam_##class##_##signal##_callback signal##_callback

#define qvcam_signal_define(class, signal) \
    void qvcam_##class##_set_##signal##_callback(qvcam_##class##_t self, \
                                                  const qvcam_##class##_##signal##_callback callback) \
    { \
        self->signal##_callback = callback; \
    }

#define qvcam_connect(class, sender, signal, receiver, method) \
    do { \
        qvcam_##class##_##signal##_callback signal_callback; \
        signal_callback.user_data = receiver; \
        signal_callback.callback = (qvcam_##class##_##signal##_proc) method; \
        qvcam_##class##_set_##signal##_callback(sender, signal_callback); \
    } while (false)

#define qvcam_emit(self, signal, ...) \
    do { \
        if ((self)->signal##_callback.callback) \
            (self)->signal##_callback.callback(self->signal##_callback.user_data, \
                                               __VA_ARGS__); \
    } while (false)

#define qvcam_emit_no_args(self, signal) \
    do { \
        if ((self)->signal##_callback.callback) \
            (self)->signal##_callback.callback(self->signal##_callback.user_data); \
    } while (false)

#define qvcam_call(self, signal, ...) \
({ \
    int result = 0; \
    \
    if ((self)->signal##_callback.callback) \
        result = (self)->signal##_callback.callback(self->signal##_callback.user_data, \
                                                    __VA_ARGS__); \
    \
    result; \
})

#define qvcam_call_no_args(self, signal) \
({ \
    int result = 0; \
    \
    if ((self)->signal##_callback.callback) \
        result = (self)->signal##_callback.callback(self->signal##_callback.user_data); \
    \
    result; \
})

#define qvcam_init_field(v4l2_struct, field) \
    memset((v4l2_struct)->field, 0, sizeof((v4l2_struct)->field))

#define qvcam_init_reserved(v4l2_struct) \
    qvcam_init_field(v4l2_struct, reserved)

#define qvcam_wait_condition(wait_queue, condition, mtx, msecs) \
({ \
    int result; \
    int mutex_result; \
    \
    mutex_unlock(mtx); \
    result = wait_event_interruptible_timeout(wait_queue, \
                                              condition, \
                                              msecs_to_jiffies(msecs)); \
    mutex_result = mutex_lock_interruptible(mtx); \
    \
    if (mutex_result) \
        result = mutex_result; \
    \
    result; \
})

#define qvcam_strlen(str) \
({ \
    size_t len = 0; \
    \
    if (str) \
        len = strnlen(str, QVCAM_MAX_STRING_SIZE); \
    \
    len; \
})

typedef enum
{
    QVCAM_MEMORY_TYPE_KMALLOC,
    QVCAM_MEMORY_TYPE_VMALLOC,
} QVCAM_MEMORY_TYPE;

typedef bool (*qvcam_are_equals_t)(const void *element_data, const void *data);
typedef void *(*qvcam_copy_t)(void *data);
typedef void (*qvcam_delete_t)(void *data);

uint64_t qvcam_id(void);
int qvcam_get_last_error(void);
int qvcam_set_last_error(int error);
void qvcam_string_from_error(int error, char *str, size_t len);
char *qvcam_strdup(const char *str, QVCAM_MEMORY_TYPE type);
char *qvcam_strip_str(const char *str, QVCAM_MEMORY_TYPE type);
char *qvcam_strip_str_sub(const char *str,
                           size_t from,
                           size_t size,
                           QVCAM_MEMORY_TYPE type);
void qvcam_replace(char *str, char from, char to);

#endif // QVCAM_UTILS_H
