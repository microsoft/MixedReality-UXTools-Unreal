// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "MicrosoftOpenXRDetails.h"

#define LOCTEXT_NAMESPACE "FMicrosoftOpenXRDetails"

TSharedRef<IDetailCustomization> FMicrosoftOpenXRDetails::MakeInstance()
{
	return MakeShareable(new FMicrosoftOpenXRDetails);
}

void FMicrosoftOpenXRDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	statusTextWidget = SNew(STextBlock);

	UMicrosoftOpenXRRuntimeSettings::Get()->OnRemotingStatusChanged.BindSP(this, &FMicrosoftOpenXRDetails::SetStatusText);

	IDetailCategoryBuilder& remotingCategory = DetailBuilder.EditCategory(TEXT("OpenXR Holographic Remoting"));
	remotingCategory.AddCustomRow(LOCTEXT("Connect Button", "Connect Button"))
	[
		SNew(SButton)
		.Text(LOCTEXT("Connect", "Connect"))
		.OnClicked_Raw(this, &FMicrosoftOpenXRDetails::OnConnectButtonClicked)
		.IsEnabled_Raw(this, &FMicrosoftOpenXRDetails::AreButtonsEnabled)
	];
	
	remotingCategory.AddCustomRow(LOCTEXT("Disconnect Button", "Disconnect Button"))
	[
		SNew(SButton)
		.Text(LOCTEXT("Disconnect", "Disconnect"))
		.OnClicked_Raw(this, &FMicrosoftOpenXRDetails::OnDisconnectButtonClicked)
		.IsEnabled_Raw(this, &FMicrosoftOpenXRDetails::AreButtonsEnabled)
	];

	remotingCategory.AddCustomRow(LOCTEXT("Status Text", "Status Text"))[statusTextWidget.ToSharedRef()];
}

void FMicrosoftOpenXRDetails::SetStatusText(FString message, FLinearColor statusColor)
{
	if (statusTextWidget == nullptr)
	{
		return;
	}

	statusTextWidget->SetText(FText::FromString(message));
	statusTextWidget->SetColorAndOpacity(FSlateColor(statusColor));
}

FReply FMicrosoftOpenXRDetails::OnConnectButtonClicked()
{
	MicrosoftOpenXR::RemotingConnectionData data;
	UMicrosoftOpenXRRuntimeSettings::ParseAddress(
		UMicrosoftOpenXRRuntimeSettings::Get()->RemoteHoloLensIP,
		data.IP, data.Port);
	data.Bitrate = UMicrosoftOpenXRRuntimeSettings::Get()->MaxBitrate;
	data.EnableAudio = UMicrosoftOpenXRRuntimeSettings::Get()->EnableAudio;
	data.ConnectionType = UMicrosoftOpenXRRuntimeSettings::Get()->ConnectionType;
	data.ConnectionCodec = UMicrosoftOpenXRRuntimeSettings::Get()->ConnectionCodec;

	UMicrosoftOpenXRRuntimeSettings::Get()->OnRemotingConnect.ExecuteIfBound(data);

	return FReply::Handled();
}
	
FReply FMicrosoftOpenXRDetails::OnDisconnectButtonClicked()
{
	UMicrosoftOpenXRRuntimeSettings::Get()->OnRemotingDisconnect.ExecuteIfBound();

	return FReply::Handled();
}
	
bool FMicrosoftOpenXRDetails::AreButtonsEnabled() const
{
	UMicrosoftOpenXRRuntimeSettings* settings = UMicrosoftOpenXRRuntimeSettings::Get();
	return settings->bEnableRemotingForEditor;
}

#undef LOCTEXT_NAMESPACE
