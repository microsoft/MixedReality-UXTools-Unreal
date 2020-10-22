// Copyright (c) Microsoft Corporation. All rights reserved.

#include "MRPlatExtDetails.h"

#define LOCTEXT_NAMESPACE "FMRPlatExtDetails"

TSharedRef<IDetailCustomization> FMRPlatExtDetails::MakeInstance()
{
	return MakeShareable(new FMRPlatExtDetails);
}

void FMRPlatExtDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	statusTextWidget = SNew(STextBlock);

	UMRPlatExtRuntimeSettings::Get()->OnRemotingStatusChanged.BindSP(this, &FMRPlatExtDetails::SetStatusText);

	IDetailCategoryBuilder& remotingCategory = DetailBuilder.EditCategory(TEXT("OpenXR Holographic Remoting"));
	remotingCategory.AddCustomRow(LOCTEXT("Connect Button", "Connect Button"))
	[
		SNew(SButton)
		.Text(LOCTEXT("Connect", "Connect"))
		.OnClicked_Raw(this, &FMRPlatExtDetails::OnConnectButtonClicked)
		.IsEnabled_Raw(this, &FMRPlatExtDetails::AreButtonsEnabled)
	];
	
	remotingCategory.AddCustomRow(LOCTEXT("Disconnect Button", "Disconnect Button"))
	[
		SNew(SButton)
		.Text(LOCTEXT("Disconnect", "Disconnect"))
		.OnClicked_Raw(this, &FMRPlatExtDetails::OnDisconnectButtonClicked)
		.IsEnabled_Raw(this, &FMRPlatExtDetails::AreButtonsEnabled)
	];

	remotingCategory.AddCustomRow(LOCTEXT("Status Text", "Status Text"))[statusTextWidget.ToSharedRef()];
}

void FMRPlatExtDetails::SetStatusText(FString message, FLinearColor statusColor)
{
	if (statusTextWidget == nullptr)
	{
		return;
	}

	statusTextWidget->SetText(FText::FromString(message));
	statusTextWidget->SetColorAndOpacity(FSlateColor(statusColor));
}

FReply FMRPlatExtDetails::OnConnectButtonClicked()
{
	MRPlatExt::RemotingConnectionData data;
	UMRPlatExtRuntimeSettings::ParseAddress(
		UMRPlatExtRuntimeSettings::Get()->RemoteHoloLensIP,
		data.IP, data.Port);
	data.Bitrate = UMRPlatExtRuntimeSettings::Get()->MaxBitrate;
	data.EnableAudio = UMRPlatExtRuntimeSettings::Get()->EnableAudio;
	data.ConnectionType = UMRPlatExtRuntimeSettings::Get()->ConnectionType;
	data.ConnectionCodec = UMRPlatExtRuntimeSettings::Get()->ConnectionCodec;

	UMRPlatExtRuntimeSettings::Get()->OnRemotingConnect.ExecuteIfBound(data);

	return FReply::Handled();
}
	
FReply FMRPlatExtDetails::OnDisconnectButtonClicked()
{
	UMRPlatExtRuntimeSettings::Get()->OnRemotingDisconnect.ExecuteIfBound();

	return FReply::Handled();
}
	
bool FMRPlatExtDetails::AreButtonsEnabled() const
{
	UMRPlatExtRuntimeSettings* settings = UMRPlatExtRuntimeSettings::Get();
	return settings->bEnableRemotingForEditor;
}

#undef LOCTEXT_NAMESPACE
