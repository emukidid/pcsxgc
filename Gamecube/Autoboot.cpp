#include "Autoboot.h"
#include <string.h>

static const char* autobootPath = NULL;

namespace Autoboot
{
    void setPath(const char* path)
    {
        if (!path) {
			autobootPath = NULL;
            return;
		}

        if (strncmp(path, "sd:/", 4) == 0 ||
            strncmp(path, "usb:/", 5) == 0)
        {
            autobootPath = path;
        }
    }

    const char* getPath()
    {
        return autobootPath;
    }

    bool hasPath()
    {
        return autobootPath != NULL;
    }
}
