//
//  RenderFetchSortCullTask.h
//  render/src/
//
//  Created by Zach Pomerantz on 12/22/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderFetchSortCullTask_h
#define hifi_RenderFetchSortCullTask_h

#include <gpu/Pipeline.h>

#include "Task.h"
#include "CullTask.h"

class RenderFetchSortCullTask : public render::Task {
public:
    using Output = std::array<render::Varying, 6>;
    using JobModel = ModelO<RenderFetchSortCullTask>;

    RenderFetchSortCullTask(render::CullFunctor cullFunctor);
};

#endif // hifi_RenderFetchSortCullTask_h
