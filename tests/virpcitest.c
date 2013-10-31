/*
 * Copyright (C) 2013 Red Hat, Inc.
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
 *
 * Author: Michal Privoznik <mprivozn@redhat.com>
 */

#include <config.h>

#include "testutils.h"

#ifdef __linux__

# include <stdlib.h>
# include <stdio.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <virpci.h>

# define VIR_FROM_THIS VIR_FROM_NONE

static int
testVirPCIDeviceNew(const void *opaque ATTRIBUTE_UNUSED)
{
    int ret = -1;
    virPCIDevicePtr dev;
    const char *devName;

    if (!(dev = virPCIDeviceNew(0, 0, 0, 0)))
        goto cleanup;

    devName = virPCIDeviceGetName(dev);
    if (STRNEQ(devName, "0000:00:00.0")) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       "PCI device name mismatch: %s, expecting %s",
                       devName, "0000:00:00.0");
        goto cleanup;
    }

    ret = 0;
cleanup:
    virPCIDeviceFree(dev);
    return ret;
}

# define CHECK_LIST_COUNT(list, cnt)                                    \
    if ((count = virPCIDeviceListCount(list)) != cnt) {                 \
        virReportError(VIR_ERR_INTERNAL_ERROR,                          \
                       "Unexpected count of items in " #list ": %d, "   \
                       "expecting " #cnt, count);                       \
        goto cleanup;                                                   \
    }

static int
testVirPCIDeviceDetach(const void *oaque ATTRIBUTE_UNUSED)
{
    int ret = -1;
    virPCIDevicePtr dev = NULL, unbindedDev = NULL;
    virPCIDeviceListPtr activeDevs = NULL, inactiveDevs = NULL;
    int count;

    if (!(dev = virPCIDeviceNew(0, 0, 1, 0)) ||
        !(unbindedDev = virPCIDeviceNew(0, 0, 3, 0)) ||
        !(activeDevs = virPCIDeviceListNew()) ||
        !(inactiveDevs = virPCIDeviceListNew()))
        goto cleanup;

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 0);

    if (virPCIDeviceSetStubDriver(dev, "pci-stub") < 0 ||
        virPCIDeviceSetStubDriver(unbindedDev, "pci-stub") < 0)
        goto cleanup;

    if (virPCIDeviceDetach(dev, activeDevs, inactiveDevs) < 0)
        goto cleanup;

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 1);

    if (virPCIDeviceDetach(unbindedDev, activeDevs, inactiveDevs) < 0)
        goto cleanup;

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 2);

    ret = 0;
cleanup:
    virPCIDeviceFree(dev);
    virPCIDeviceFree(unbindedDev);
    virObjectUnref(activeDevs);
    virObjectUnref(inactiveDevs);
    return ret;
}

# define FAKESYSFSDIRTEMPLATE abs_builddir "/fakesysfsdir-XXXXXX"

static int
testVirPCIDeviceReattach(const void *oaque ATTRIBUTE_UNUSED)
{
    int ret = -1;
    virPCIDevicePtr dev = NULL, unbindedDev = NULL;
    virPCIDeviceListPtr activeDevs = NULL, inactiveDevs = NULL;
    int count;

    if (!(dev = virPCIDeviceNew(0, 0, 1, 0)) ||
        !(unbindedDev = virPCIDeviceNew(0, 0, 3, 0)) ||
        !(activeDevs = virPCIDeviceListNew()) ||
        !(inactiveDevs = virPCIDeviceListNew()))
        goto cleanup;

    if (virPCIDeviceListAdd(inactiveDevs, dev) < 0) {
        virPCIDeviceFree(dev);
        virPCIDeviceFree(unbindedDev);
        goto cleanup;
    }

    if (virPCIDeviceListAdd(inactiveDevs, unbindedDev) < 0) {
        virPCIDeviceFree(unbindedDev);
        goto cleanup;
    }

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 2);

    if (virPCIDeviceSetStubDriver(dev, "pci-stub") < 0)
        goto cleanup;

    if (virPCIDeviceReattach(dev, activeDevs, inactiveDevs) < 0)
        goto cleanup;

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 1);

    if (virPCIDeviceReattach(unbindedDev, activeDevs, inactiveDevs) < 0)
        goto cleanup;

    CHECK_LIST_COUNT(activeDevs, 0);
    CHECK_LIST_COUNT(inactiveDevs, 0);

    ret = 0;
cleanup:
    virObjectUnref(activeDevs);
    virObjectUnref(inactiveDevs);
    return ret;
}
static int
mymain(void)
{
    int ret = 0;
    char *fakesysfsdir;

    if (VIR_STRDUP_QUIET(fakesysfsdir, FAKESYSFSDIRTEMPLATE) < 0) {
        fprintf(stderr, "Out of memory\n");
        abort();
    }

    if (!mkdtemp(fakesysfsdir)) {
        fprintf(stderr, "Cannot create fakesysfsdir");
        abort();
    }

    setenv("LIBVIRT_FAKE_SYSFS_DIR", fakesysfsdir, 1);

# define DO_TEST(fnc)                                   \
    do {                                                \
        if (virtTestRun(#fnc, fnc, NULL) < 0)           \
            ret = -1;                                   \
    } while (0)

    DO_TEST(testVirPCIDeviceNew);
    DO_TEST(testVirPCIDeviceDetach);
    DO_TEST(testVirPCIDeviceReattach);

    if (getenv("LIBVIRT_SKIP_CLEANUP") == NULL)
        virFileDeleteTree(fakesysfsdir);

    VIR_FREE(fakesysfsdir);

    return ret==0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

VIRT_TEST_MAIN_PRELOAD(mymain, abs_builddir "/.libs/virpcimock.so")
#else
int
main(void)
{
    return EXIT_AM_SKIP;
}
#endif
