// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMDLOutput, Log, All);

#if WITH_MDL_SDK

#include "MDLDependencies.h"
#include "Logging/LogMacros.h"

class FMDLOutputLogger : public mi::base::Interface_implement<mi::base::ILogger>
{
public:
	void message(mi::base::Message_severity level, const char* module_category, const mi::base::Message_details&, const char* message) override;
};

extern FMDLOutputLogger GMDLOutputLogger;

#endif
