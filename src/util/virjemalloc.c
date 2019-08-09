/*
 * virjemalloc.c: support for jemalloc (http://jemalloc.net/)
 *
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "internal.h"
#include "virlog.h"
#include "virerror.h"
#include "virenum.h"
#include "virjemalloc.h"

#define VIR_FROM_THIS VIR_FROM_NONE

#ifdef WITH_JEMALLOC
    #include <jemalloc/jemalloc.h>

VIR_LOG_INIT("util.jemalloc");

static int
virJemallocMallctlSet(const char *je_mallctl_ns, void *data, size_t data_len)
{
    int ret = -1;
    VIR_DEBUG("Set jemalloc mallctl namespace %s", je_mallctl_ns);
    if (mallctl(je_mallctl_ns, NULL, NULL, data, data_len)) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Failed to set jemalloc mallctl namespace %s"),
                       je_mallctl_ns);
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

static int
virJemallocMallctlGet(const char *je_mallctl_ns, void *data, size_t *data_len)
{
    int ret = -1;
    VIR_DEBUG("Get jemalloc mallctl namespace %s", je_mallctl_ns);
    if (mallctl(je_mallctl_ns, data, data_len, NULL, 0) != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Failed to get jemalloc mallctl namespace %s"),
                       je_mallctl_ns);
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

int virJemallocMallctlSetProfDump(const char *filename)
{
    return virJemallocMallctlSet("prof.dump", &filename, sizeof(const char *));
}

int virJemallocMallctlSetProfActive(bool active)
{
    return virJemallocMallctlSet("prof.active", &active, sizeof(bool));
}

bool virJemallocProfIsEnabled(void)
{
    bool enabled = false;
    size_t len = 0;
    virJemallocMallctlGet("config.prof", &enabled, &len);

    return enabled;
}
#endif
