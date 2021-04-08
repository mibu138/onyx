#include "locations.h"
#include <hell/common.h>
#include <string.h>
#include <assert.h>

#define MAX_PATH_LEN OBDN_MAX_PATH_LEN

static const char* filepath = __FILE__;

const char* obdn_GetObdnRoot(void)
{
    static char root[MAX_PATH_LEN];
    if (root[0] == 0) // not set yet
    {
        const uint32_t filepathlen = strnlen(filepath, MAX_PATH_LEN);
        if (filepathlen > MAX_PATH_LEN)
            hell_Error(HELL_ERR_FATAL, "File path %s length exceeds limit of %d chars.\n", __FILE__, MAX_PATH_LEN);
        strcpy(root, filepath);
        char* iter = root + filepathlen - 1;
        while (iter != filepath && *iter != '/')
            *iter-- = '\0';
        *iter = '\0';
        hell_DPrint("Obdn root: %s\n", root);
    }
    return root;
}
