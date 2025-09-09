#include "metal/time.h"
#include "platform_port.h"

unsigned long long metal_get_timestamp(void)
{
    return TIMESTAMP_GET;
}
