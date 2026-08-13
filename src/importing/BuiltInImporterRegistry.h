#pragma once

#include "ImporterRegistry.h"
#include "ImportProfile.h"

ImporterRegistry createBuiltInImporterRegistry(
    const ImportProfile &profile
    );