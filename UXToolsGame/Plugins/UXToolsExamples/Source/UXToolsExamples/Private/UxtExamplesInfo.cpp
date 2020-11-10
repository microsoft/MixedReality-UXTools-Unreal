// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtExamplesInfo.h"

#include "UxtExamplesVersion.h"

#include "Engine/World.h"
#include "Internationalization/Text.h"

FText UUxtExamplesInfo::CommitSHA(const UObject* WorldContext)
{
	// VCS information can only be reliably set in packaged projects
	if (WorldContext && (WorldContext->GetWorld()->WorldType == EWorldType::Game))
	{
		return FText::FromString(TEXT(UXTOOLSEXAMPLES_COMMIT_SHA));
	}
	return FText();
}
