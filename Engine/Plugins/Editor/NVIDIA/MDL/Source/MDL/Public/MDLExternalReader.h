// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"

class MDL_API IMDLExternalReader
{
public:
	/** Destructor, also the only way to close the file handle **/
	virtual ~IMDLExternalReader()
	{
	}

	virtual bool		FileExists(const FString& InPath, const FString& InMask) = 0;

	virtual class IFileHandle* OpenRead(const FString& InPath) = 0;
};

