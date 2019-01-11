#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include <cJSON.h>
#include <array.h> // from https://github.com/shlemisto/C_data_structures

#if 1
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif

#define cmp_types(t1, t2) __builtin_types_compatible_p(typeof(t1), t2)

static bool __parse_with_default_value = 0; // suppress logging if json at field '<name>' is not found
static char g_json_parse_err_buf[1024] = { 0 }; // global error buffer

static int JSON_PARSER_DEBUG = 1;
#define JSON_PARSER "JSON_PARSER"

enum {
    JSON_PARSE_CMD_CJSON_NO_CLONE = 1,
    JSON_PARSE_CMD_STRING_FIXED_LEN,
    JSON_PARSE_CMD_STRING_NO_CLONE
};

#define json_parse_err(...) \
    do { \
        uds_snprintf(g_json_parse_err_buf, sizeof(g_json_parse_err_buf), __VA_ARGS__); \
        __uds_log(LOG_ERR, JSON_PARSER, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

#define json_parse_err_with_status(code, ...) \
    do { \
        if (!(__parse_with_default_value && UDS_ERR_NOT_FOUND == code)) { \
            uds_snprintf(g_json_parse_err_buf, sizeof(g_json_parse_err_buf), __VA_ARGS__); \
            __uds_log(LOG_ERR, JSON_PARSER, __func__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

#define json_parse_info(...) ({ if (JSON_PARSER_DEBUG) __uds_log(LOG_DEBUG, JSON_PARSER, __func__, __LINE__, __VA_ARGS__); else { ; } })

static int __json_parse_string(cJSON *js, char *field, char **str);
static int __json_parse_int(cJSON *js, char *field, int *num);
static int __json_parse_uint(cJSON *js, char *field, int *num);
static int __json_parse_bool(cJSON *js, char *field, bool *num);
static int __json_parse_uds_disk_item(cJSON *js, char *field, uds_disk_item_t **pdisk);
static int __json_parse_uds_string_array(cJSON *js, char *field, uds_string_array_t **labels);
static int __json_parse_json(cJSON *js, char *field, cJSON **jout);

static int __json_parse_string_fixed_len(cJSON *js, char *field, void *str, size_t len);
static int __json_parse_json_no_clone(cJSON *js, char *field, cJSON **jout);
static int __json_parse_string_no_clone(cJSON *js, char *field, char **str);

// 4th and 5th param for manual mode, they are zero by default
#define json_parse(json, field, var, ...) __json_parse_generic(json, field, var, ##__VA_ARGS__, 0, 0)
#define __json_parse_generic(json, field, var, cmd, sub_cmd, ...) ({ \
    int __ret = UDS_ERR_OK; \
    \
    if (cmd) \
    { \
        switch (cmd) { \
        case JSON_PARSE_CMD_CJSON_NO_CLONE: \
            __ret = __json_parse_json_no_clone(json, field, (cJSON **) var); \
            break; \
        case JSON_PARSE_CMD_STRING_FIXED_LEN: \
            __ret = __json_parse_string_fixed_len(json, field, (void *) var, sub_cmd); \
            break; \
        case JSON_PARSE_CMD_STRING_NO_CLONE: \
            __ret = __json_parse_string_no_clone(json, field, (char **) var); \
            break; \
        default: \
            __ret = UDS_ERR_INVPARAM; \
            json_parse_err("invalid command for manual mode '%d'", cmd); \
        } \
    } \
    else \
    { \
        if (cmp_types(var, char **)) \
            __ret = __json_parse_string(json, field, (char **) var); \
        else if (cmp_types(var, int *)) \
            __ret = __json_parse_int(json, field, (int *) var); \
        else if (cmp_types(var, bool *)) \
            __ret = __json_parse_bool(json, field, (bool *) var); \
        else if (cmp_types(var, unsigned int *)) \
            __ret = __json_parse_uint(json, field, (int *) var); \
        else if (cmp_types(var, uds_string_array_t **)) \
            __ret = __json_parse_uds_string_array(json, field, (uds_string_array_t **) var); \
        else if (cmp_types(var, cJSON **)) \
            __ret = __json_parse_json(json, field, (cJSON **) var); \
        else \
        { \
            json_parse_err("unable to parse unknown type at field '%s'", field); \
            __ret = UDS_ERR_INVPARAM; \
        } \
    } \
    \
    __ret; \
})

// suppress debug message, only errors will be printed out
#define json_parse_silent(json, field, var, ...) ({ \
        JSON_PARSER_DEBUG = 0; \
        int __ret = json_parse(json, field, var, __VA_ARGS__); \
        JSON_PARSER_DEBUG = 1; \
        __ret; \
})

// same as json_parse but if field not founded - assign default value
#define json_parse_with_default(json, field, var, def_val, ...) __json_parse_with_default_generic(json, field, var, def_val, ##__VA_ARGS__, 0, 0)
#define __json_parse_with_default_generic(json, field, var, def_val, cmd, sub_cmd, ...) ({ \
    __parse_with_default_value = true; \
    int __ret = json_parse(json, field, var, cmd, sub_cmd); \
    \
    if (UDS_ERR_NOT_FOUND == __ret) \
    { \
        __ret = UDS_ERR_OK; \
        \
        if (JSON_PARSE_CMD_STRING_FIXED_LEN == cmd) \
        { \
            if (strlen((char *) def_val) >= (size_t) sub_cmd) \
            { \
                __ret = UDS_ERR_BUF_TOO_SMALL; \
                json_parse_err("buffer is too small to contain default value '%s'", (char *) def_val); \
            } \
            else \
                memcpy(var, def_val, strlen(uds_cast(char *, def_val)) + 1); \
        } \
        else if (JSON_PARSE_CMD_STRING_NO_CLONE == cmd || JSON_PARSE_CMD_CJSON_NO_CLONE == cmd) \
        { \
            __ret = UDS_ERR_INVPARAM; \
            json_parse_err("unable to set default value for non-clone version"); \
        } \
        else if (cmp_types(var, char **)) \
        { \
            if (NULL == (*(uds_cast(char **, var)) = strdup(uds_cast(char *, def_val)))) \
            { \
                __ret = UDS_ERR_NOMEM; \
                json_parse_err("no mem to strdup default value '%s'", uds_cast(char *, def_val)); \
            } \
        } \
        else if (cmp_types(var, cJSON **)) \
        { \
            if (NULL == ((cJSON *) def_val)) \
                __ret = UDS_ERR_INVPARAM; \
            else \
                *(uds_cast(cJSON **, var)) = uds_cast(cJSON *, def_val); \
        } \
        else if (cmp_types(var, uds_string_array_t **)) \
        { \
            if (NULL == ((uds_string_array_t *) def_val)) \
                __ret = UDS_ERR_INVPARAM; \
            else \
                *(uds_cast(uds_string_array_t **, var)) = uds_cast(uds_string_array_t *, def_val); \
        } \
        else if (cmp_types(var, uds_static_string_t *)) \
        { \
            uds_static_string_t *string = (uds_static_string_t *) var; \
            if (UDS_ERR_BUF_TOO_SMALL == uds_snprintf(string->str, sizeof(string->str), "%s", (char *) def_val)) \
                __ret = UDS_ERR_BUF_TOO_SMALL; \
        } \
        else if (cmp_types(var, int *)) \
            *uds_cast(int *, var) = uds_cast(int, def_val); \
        else if (cmp_types(var, bool *)) \
            *uds_cast(bool *, var) = uds_cast(bool, def_val); \
        else if (cmp_types(var, unsigned int *)) \
            *uds_cast(unsigned int *, var) = uds_cast(unsigned int, def_val); \
        else \
        { \
            __ret = UDS_ERR_INVPARAM; \
            json_parse_err("!!! unable to assign default value for unknown type"); \
        } \
        \
        if (UDS_ERR_OK == __ret) \
            json_parse_info("for field '%s' was set default value", field); \
        else \
            json_parse_err("!!! setted up default value for field '%s' finished with error %d", field, __ret); \
    } \
    __parse_with_default_value = false; \
    __ret; \
})

/* --> parse primitive types --- */
static int __json_parse_string(cJSON *js, char *field, char **str)
{
    int ret = UDS_ERR_OK;
    char *tmp = NULL;

    if (UDS_ERR_OK != (ret = cJSON_GetStringFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'string' from field '%s', code %d", field, ret);
    else if (NULL == (*str = strdup(tmp)))
        ret = no_memory_trace();

    if (UDS_ERR_OK == ret)
        json_parse_info("string '%s' have parsed from field '%s'", *str, field);

    return ret;
}

static int __json_parse_string_no_clone(cJSON *js, char *field, char **str)
{
    int ret = UDS_ERR_OK;
    char *tmp = NULL;

    if (!str)
    {
        ret = UDS_ERR_INVPARAM;
        json_parse_err_with_status(ret, "<NULL> argument was passed");
    }
    else if (UDS_ERR_OK != (ret = cJSON_GetStringFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'string' from field '%s', code %d", field, ret);
    else
        *str = tmp;

    if (UDS_ERR_OK == ret)
        json_parse_info("string '%s' have parsed from field '%s'", *str, field);

    return ret;
}

static int __json_parse_string_fixed_len(cJSON *js, char *field, void *pstr, size_t len)
{
    int ret = UDS_ERR_OK;
    char *tmp = NULL;
    size_t tmp_len = 0;

    if (!pstr)
    {
        ret = UDS_ERR_INVPARAM;
        json_parse_err_with_status(ret, "<NULL> argument was passed");
    }
    else if (UDS_ERR_OK != (ret = cJSON_GetStringFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'string' from field '%s', code %d", field, ret);

    else if ((tmp_len = strlen(tmp)) >= len)
    {
        ret = UDS_ERR_BUF_TOO_SMALL;
        json_parse_err_with_status(ret, "char[%d] is too small to contain char[%d]", len, tmp_len);
    }
    else
        memcpy(pstr, tmp, tmp_len + 1);

    if (UDS_ERR_OK == ret)
        json_parse_info("string '%s' have parsed from field '%s'", (char *) pstr, field);

    return ret;
}

static int __json_parse_int(cJSON *js, char *field, int *num)
{
    int ret = UDS_ERR_OK;
    int tmp;

    if (UDS_ERR_OK != (ret = cJSON_GetIntFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'int' from field '%s', code %d", field, ret);
    else
        *num = tmp;

    if (UDS_ERR_OK == ret)
        json_parse_info("int '%d' have parsed from field '%s'", *num, field);

    return ret;
}

static int __json_parse_uint(cJSON *js, char *field, int *num)
{
    int ret = UDS_ERR_OK;
    int tmp;

    if (UDS_ERR_OK != (ret = cJSON_GetIntFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'uint' from field '%s', code %d", field, ret);
    else if (tmp < 0)
    {
        ret = UDS_ERR_FAIL;
        json_parse_err("expected positive value for field '%s', 've got '%d'", field, tmp);
    }
    else if (str_eq(field, "node_id") && (tmp < UDS_NODE_1 || tmp > UDS_NODE_COUNT_MAX))
    {
        ret = UDS_ERR_FAIL;
        json_parse_err("expected value between [%d, %d] for field '%s', 've got '%d'", UDS_NODE_1, UDS_NODE_2, field, tmp);
    }
    else
        *num = tmp;

    if (UDS_ERR_OK == ret)
        json_parse_info("uint '%d' have parsed from field '%s'", *num, field);

    return ret;
}

static int __json_parse_bool(cJSON *js, char *field, bool *num)
{
    int ret = UDS_ERR_OK;
    int tmp = 0;

    if (UDS_ERR_OK != (ret = cJSON_GetBoolFromObject_(js, field, &tmp)))
        json_parse_err_with_status(ret, "unable to get 'bool' from field '%s', code %d", field, ret);
    else
        *num = !!tmp;

    if (UDS_ERR_OK == ret)
        json_parse_info("bool '%s' have parsed from field '%s'", *num ? "true" : "false", field);

    return ret;
}
/* <-- parse primitive types --- */

static int __json_parse_uds_string_array(cJSON *js, char *field, uds_string_array_t **plabels)
{
    int ret = UDS_ERR_FAIL;
    cJSON *arr = NULL;
    uds_string_array_t *labels = NULL;

    if (NULL == (arr = cJSON_GetObjectItem_(js, field, cJSON_Array)))
    {
        ret = UDS_ERR_NOT_FOUND;
        json_parse_err_with_status(ret, "unable to get string array from field '%s', code %d", field, UDS_ERR_NOTFOUND);
        goto exit;
    }

    if (NULL == (*plabels = labels = parray_new(uds_string_array)))
    {
        ret = no_memory_trace();
        goto exit;
    }

    cJSON_ArrayForeach(label, arr)
    {
        if (!cJSON_IsString(label))
        {
            ret = UDS_ERR_INVPARAM;
            json_parse_err("disk label is not a string, type '%d'", label->type);
            goto exit;
        }
        else if (parray_push(labels, strdup(label->valuestring)))
        {
            ret = no_memory_trace();
            goto exit;
        }
    }

    char *disk;
    parray_for_each(labels, disk)
        json_parse_info("disk_label '%s' was added into array", disk);

    return UDS_ERR_OK;

exit:
    parray_free(labels);

    return ret;
}

static int __json_parse_json(cJSON *js, char *field, cJSON **jout)
{
    int ret = UDS_ERR_FAIL;
    cJSON *tmp = NULL;

    if (NULL == (tmp = cJSON_GetObjectItem(js, field)))
    {
        ret = UDS_ERR_NOT_FOUND;
        json_parse_err_with_status(ret, "unable to get json from field '%s', code %d", field, UDS_ERR_NOTFOUND);
    }
    else if (NULL == (*jout = cJSON_Clone(tmp)))
        ret = no_memory_trace();
    else
        ret = UDS_ERR_OK;

    return ret;
}

static int __json_parse_json_no_clone(cJSON *js, char *field, cJSON **jout)
{
    int ret = UDS_ERR_FAIL;
    cJSON *tmp = NULL;

    if (!jout || (jout && !*jout))
    {
        ret = UDS_ERR_INVPARAM;
        json_parse_err_with_status(ret, "<NULL> argument was passed");
    }
    else if (NULL == (tmp = cJSON_GetObjectItem(js, field)))
    {
        ret = UDS_ERR_NOT_FOUND;
        json_parse_err_with_status(ret, "unable to get json from field '%s', code %d", field, UDS_ERR_NOTFOUND);
    }
    else
    {
        *jout = tmp;
        ret = UDS_ERR_OK;
    }

    return ret;
}

#endif // __JSON_PARSER_H__
