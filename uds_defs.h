#ifndef __UDS_DEFS_H__
#define __UDS_DEFS_H__

#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <stdarg.h>

#include <array.h>

enum {
    UDS_ERR_OK = 0,
    UDS_ERR_FAIL = -1,
    UDS_ERR_NOT_FOUND = -2,
    UDS_ERR_BUF_TOO_SMALL = -3,
    UDS_ERR_INVPARAM = -4,
    UDS_ERR_NOMEM = -5
};

parray_generator(char *, uds_string_array, NULL, free, strcmp)

// return an error or count of written bytes
static inline int uds_snprintf(char *str, size_t size, const char *fmt, ...)
{
    int ret;
    va_list args;

    va_start(args, fmt);
        ret = vsnprintf(str, size, fmt, args);
    va_end(args);

    return ((ret < 0) || (ret >= (size))) ? UDS_ERR_BUF_TOO_SMALL: ret;
}

// assign NULL after mem free
#define uds_free(p) \
    free(p); \
    p = NULL;

#define uds_alloc(type) (type *) calloc(1, sizeof(type))
#define uds_alloc_arr(n, type) (type *) calloc(n, sizeof(type))

static inline void __uds_log(int prio, const char *file, const char *func, int line, const char *fmt, ...)
{
    va_list args;
    char buf[PATH_MAX] = { 0 };

    if (LOG_ERR == prio)
    {
        if (UDS_ERR_BUF_TOO_SMALL == uds_snprintf(buf, sizeof(buf), "ERROR: [%s/%s():%d] ", file, func, line))
            goto err_default;
    }
    else if (LOG_WARNING == prio)
    {
        if (UDS_ERR_BUF_TOO_SMALL == uds_snprintf(buf, sizeof(buf), "WARNING: [%s/%s():%d] ", file, func, line))
            goto err_default;
    }
    else if (LOG_CRIT == prio)
    {
        if (UDS_ERR_BUF_TOO_SMALL == uds_snprintf(buf, sizeof(buf), "CRIT: [%s/%s():%d] ", file, func, line))
            goto err_default;
    }
    else if (LOG_DEBUG == prio)
    {
        if (UDS_ERR_BUF_TOO_SMALL == uds_snprintf(buf, sizeof(buf), "DEBUG: [%s/%s():%d] ", file, func, line))
            goto err_default;
    }
    else if (LOG_INFO == prio)
    {
        // print as is
    }
    else
        goto err_default;

    strcat(buf, fmt);

    va_start(args, fmt);
        vprintf(buf, args);
    va_end(args);

    return;

err_default:
    va_start(args, fmt);
        vprintf(fmt, args);
    va_end(args);
}

#define err(...) __uds_log(LOG_ERR, __FILE__,  __func__, __LINE__, __VA_ARGS__)
#define crit(...) __uds_log(LOG_CRIT, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define dbg(...) __uds_log(LOG_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define warn(...) __uds_log(LOG_WARNING, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define info(...) __uds_log(LOG_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define no_memory_trace() (err("!!! NO MEMORY !!!"), UDS_ERR_NOMEM)

#define str_eq(s1, s2) (0 == strcmp(s1, s2))
#define str_neq(s1, s2) (!str_eq(s1, s2))
#define strn_eq(s1, s2, n) (0 == strncmp(s1, s2, n))
#define strn_neq(s1, s2, n) (!strn_eq(s1, s2, n))
#define str_empty(s) (!strlen(s))

#define uds_cast(type_or_obj, obj) ((typeof(type_or_obj)) (obj))

#define OK(expr) (expr)
#define NOT(expr) (!(expr))

#endif // __UDS_DEFS_H__
