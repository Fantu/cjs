/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2008  LiTL, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <config.h>

#include <string.h>

#include <gjs/jsapi-util.h>
#include "repo.h"

#include <util/log.h>

#include <jsapi.h>

#include <girepository.h>

#include "enumeration.h"

JSObject*
gjs_lookup_enumeration(JSContext    *context,
                          GIEnumInfo   *info)
{
    JSObject *ns;
    JSObject *enum_obj;

    ns = gjs_lookup_namespace_object(context, (GIBaseInfo*) info);

    if (ns == NULL)
        return NULL;

    if (gjs_define_enumeration(context, ns, info,
                               &enum_obj))
        return enum_obj;
    else
        return NULL;
}

static JSBool
gjs_define_enum_value(JSContext    *context,
                      JSObject     *in_object,
                      GIValueInfo  *info)
{
    const char *value_name;
    int value_val;

    value_name = g_base_info_get_name( (GIBaseInfo*) info);
    value_val = (int) g_value_info_get_value(info);

    gjs_debug(GJS_DEBUG_GENUM,
              "Defining enum value %s %d",
              value_name, value_val);

    if (!JS_DefineProperty(context, in_object,
                           value_name, INT_TO_JSVAL(value_val),
                           NULL, NULL,
                           GJS_MODULE_PROP_FLAGS)) {
        gjs_throw(context, "Unable to define enumeration value %s %d (no memory most likely)",
                     value_name, value_val);
        return JS_FALSE;
    }

    return JS_TRUE;
}

JSBool
gjs_define_enumeration(JSContext    *context,
                       JSObject     *in_object,
                       GIEnumInfo   *info,
                       JSObject    **enumeration_p)
{
    const char *enum_name;
    JSObject *enum_obj;
    jsval value;
    int i;
    int n_values;

    /* An enumeration is simply an object containing integer attributes for
     * each enum value. It does not have a special JSClass.
     *
     * We could make this more typesafe and also print enum values as strings
     * if we created a class for each enum and made the enum values instances
     * of that class. However, it would have a lot more overhead and just
     * be more complicated in general. I think this is fine.
     */

    enum_name = g_base_info_get_name( (GIBaseInfo*) info);

    if (gjs_object_get_property(context, in_object, enum_name, &value)) {
        if (!JSVAL_IS_OBJECT(value)) {
            gjs_throw(context, "Existing property '%s' does not look like an enum object",
                      enum_name);
            return JS_FALSE;
        }

        enum_obj = JSVAL_TO_OBJECT(value);

        if (enumeration_p)
            *enumeration_p = enum_obj;

        return JS_TRUE;
    }

    enum_obj = JS_ConstructObject(context, NULL, NULL, NULL);
    if (enum_obj == NULL)
        return JS_FALSE;

    /* Fill in enum values first, so we don't define the enum itself until we're
     * sure we can finish successfully.
     */
    n_values = g_enum_info_get_n_values(info);
    for (i = 0; i < n_values; ++i) {
        GIValueInfo *value_info = g_enum_info_get_value(info, i);
        gboolean failed;

        failed = !gjs_define_enum_value(context, enum_obj, value_info);

        g_base_info_unref( (GIBaseInfo*) value_info);

        if (failed) {
            return JS_FALSE;
        }
    }

    gjs_debug(GJS_DEBUG_GENUM,
              "Defining %s.%s as %p",
              g_base_info_get_namespace( (GIBaseInfo*) info),
              enum_name, enum_obj);

    if (!JS_DefineProperty(context, in_object,
                           enum_name, OBJECT_TO_JSVAL(enum_obj),
                           NULL, NULL,
                           GJS_MODULE_PROP_FLAGS)) {
        gjs_throw(context, "Unable to define enumeration property (no memory most likely)");
        return JS_FALSE;
    }

    if (enumeration_p)
        *enumeration_p = enum_obj;

    return JS_TRUE;
}