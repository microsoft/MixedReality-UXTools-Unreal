// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;
using System.Collections.Generic;

public class UXToolsGameTarget : TargetRules
{
	public UXToolsGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "UXToolsGame", "UXToolsTests" } );
	}
}

