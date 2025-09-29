#ifndef _TRANSFORM_
#define _TRANSFORM_

#include "Common.h"

vec3 toTBNSpace(vec3 local, vec3 n, vec3 t, vec3 b) {
    return local.x * t + local.y * b + local.z * n;
}

#endif
