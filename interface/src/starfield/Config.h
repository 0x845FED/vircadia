//
// starfield/Config.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Config__
#define __interface__starfield__Config__

//
// Dependencies:
//

#include "InterfaceConfig.h"
#include "renderer/ProgramObject.h"

#include <cstddef>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cctype>

#include <stdint.h>

#include <new>
#include <vector>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "AngleUtil.h"
#include "Radix2InplaceSort.h"
#include "Radix2IntegerScanner.h"
#include "FloodFill.h"

// Namespace configuration:

namespace starfield {

    using glm::vec3;
    using glm::vec4;
    using glm::dot;
    using glm::normalize;
    using glm::X;
    using glm::Y;
    using glm::Z;
    using glm::W;
    using glm::mat4;
    using glm::row;

    using namespace std;

    typedef uint32_t nuint;
    typedef uint64_t wuint;

}

#endif

