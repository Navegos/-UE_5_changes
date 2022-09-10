// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "MDLPathUtility.h"

#define ENABLE_MANGLE 1

static FString MangledPathPrefixPkg = TEXT("MANGLED");
static FString MangledElemPrefix = TEXT("z");
static FString MDLSuffix = TEXT(".mdl");
static FString FileSeparator = TEXT("/");
static FString MDLSeparator = TEXT("::");

bool IsValidMdlPathChar(TCHAR c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
}

FString MangleMdlPath(const FString& Input, bool bMDLSeparator)
{
#if ENABLE_MANGLE
	bool bHasSuffix = Input.EndsWith(MDLSuffix);
	FString ModulePath = bHasSuffix ? Input.LeftChop(MDLSuffix.Len()) : Input;

	FString Separator = bMDLSeparator ? MDLSeparator : FileSeparator;
	FString Output;
	FString RelativeOutput;
	bool bStartWithSeparator = ModulePath.StartsWith(Separator);

	TArray<FString> Modules;
	ModulePath.ParseIntoArray(Modules, *Separator);

	for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ++ModuleIndex)
	{
		auto Module = Modules[ModuleIndex];

		if (Module == TEXT(".") || Module == TEXT(".."))
		{
			RelativeOutput += Module + Separator;
		}
		else
		{
			if (Output.IsEmpty())
			{
				Output += MangledPathPrefixPkg + Separator;
			}

			Output += MangledElemPrefix;
			Output += FString::FromInt(Module.Len());
			Output += MangledElemPrefix;

			auto CharArray = Module.GetCharArray();
			for (int32 Index = 0; Index < CharArray.Num() - 1; ++Index) // Skip the last \0
			{
				auto Char = CharArray[Index];
				if (IsValidMdlPathChar(Char))
				{
					Output += Char;
				}
				else
				{
					Output += TEXT("_") + FString::FromInt(Char) + TEXT("_");
				}
			}

			if (ModuleIndex < Modules.Num() - 1)
			{
				Output += Separator;
			}
		}
	}

	Output = (bStartWithSeparator ? Separator : RelativeOutput) + Output;
	if (bHasSuffix)
	{
		Output += MDLSuffix;
	}

	return Output;
#else
	return Input;
#endif
}

FString UnmangleMdlPath(const FString& Input, bool bMDLSeparator)
{
#if ENABLE_MANGLE
	FString Output;
	FString RelativeOutput;
	FString Separator = bMDLSeparator ? MDLSeparator : FileSeparator;
	bool bHasSuffix = Input.EndsWith(MDLSuffix);
	FString ModulePath = bHasSuffix ? Input.LeftChop(MDLSuffix.Len()) : Input;
	bool bStartWithSeparator = ModulePath.StartsWith(Separator);

	TArray<FString> Modules;
	ModulePath.ParseIntoArray(Modules, *Separator);

	bool bPrefixPkgLocated = false;
	for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ++ModuleIndex)
	{
		auto Module = Modules[ModuleIndex];

		if (Module == TEXT(".") || Module == TEXT(".."))
		{
			RelativeOutput += Module + Separator;
		}
		else if (Module == MangledPathPrefixPkg)
		{
			bPrefixPkgLocated = true;
		}
		else
		{
			// prefix wasn't found
			if (!bPrefixPkgLocated)
			{
				return Input;
			}

			// element prefix wasn't found
			if (!Module.StartsWith(MangledElemPrefix))
			{
				Output += Module;
			}
			else
			{
				FString MangledModule = Module.RightChop(MangledElemPrefix.Len());

				int32 PrefixIndex = MangledModule.Find(MangledElemPrefix);
				if (PrefixIndex == INDEX_NONE)
				{
					Output += Module;
				}
				else
				{
					TCHAR* End = nullptr;
					int32 CheckSize = FCString::Strtoi(*MangledModule.Left(PrefixIndex), &End, 10);
					MangledModule = MangledModule.RightChop(PrefixIndex + MangledElemPrefix.Len());
					auto CharArray = MangledModule.GetCharArray();
					bool bUnmangleChar = false;
					FString MangledChar;
					FString UnmangledModule;
					for (int32 Index = 0; Index < CharArray.Num() - 1; ++Index) // Skip the last \0
					{
						auto Char = CharArray[Index];
						if (Char == '_')
						{
							if (!bUnmangleChar)
							{
								MangledChar.Empty();
								bUnmangleChar = true;
							}
							else
							{
								UnmangledModule += TCHAR(FCString::Strtoi(*MangledChar, &End, 10));
								bUnmangleChar = false;
							}
						}
						else if (bUnmangleChar)
						{
							MangledChar += Char;
						}
						else
						{
							UnmangledModule += Char;
						}
					}

					if (UnmangledModule.Len() == CheckSize)
					{
						Output += UnmangledModule;
					}
					else
					{
						Output += Module;
					}
				}
			}

			if (ModuleIndex < Modules.Num() - 1)
			{
				Output += Separator;
			}
		}
	}

	Output = (bStartWithSeparator ? Separator : RelativeOutput) + Output;
	if (bHasSuffix)
	{
		Output += MDLSuffix;
	}
	return Output;
#else
	return Input;
#endif
}

