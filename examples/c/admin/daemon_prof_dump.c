#include <stdio.h>
#include <libvirt/libvirt-admin.h>

int main(int argc, char **argv)
{
    int ret = -1;
    virAdmConnectPtr conn = NULL;
    virAdmServerPtr srv = NULL;     /* which server to work with */
    virTypedParameterPtr params = NULL;
    int nparams = 0;
    int maxparams = 0;
    ssize_t i;

    if (argc != 2) {
        fprintf(stderr, "One argument path for memory profiling\n");
        return -1;
    }

    /* first, open a connection to the daemon */
    if (!(conn = virAdmConnectOpen(NULL, 0)))
        goto cleanup;

    /*
     * Dump the memory profiling to argv[1]
     */
    if (!virAdmConnectMemoryProfDump(conn, argv[1], 0))
        goto cleanup;

    ret = 0;
 cleanup:
    virAdmConnectClose(conn);
    return ret;
}
