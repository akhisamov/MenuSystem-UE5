// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"

namespace DebugLogPrinter
{
	static void Info(const FString& Message)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Green,
				Message
			);
		}
	}

	static void Warning(const FString& Message)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				Message
			);
		}
	}

	static void Error(const FString& Message)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				Message
			);
		}
	}

	static void Debug(const FString& Message)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				Message
			);
		}
	}
}

void UMenu::MenuSetup(int32 NumberOfPublicConnections, const FString& TypeOfMatch)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();

	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		DebugLogPrinter::Info(TEXT("Session created successfuly!"));

		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel("/Game/Maps/Lobby?listen");
		}
	}
	else
	{
		DebugLogPrinter::Error(TEXT("Failed to create session!"));
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		DebugLogPrinter::Error(TEXT("Failed to find sessions!"));
		return;
	}

	if (!MultiplayerSessionsSubsystem)
	{
		DebugLogPrinter::Error(TEXT("Failed to start joining a session!"));
		return;
	}

	for (const auto& Result : SearchResults)
	{
		if (!Result.IsValid())
		{
			continue;
		}

		FString ResultMatchType;
		Result.Session.SessionSettings.Get("MatchType", ResultMatchType);
		if (ResultMatchType == MatchType)
		{
			DebugLogPrinter::Info(TEXT("Joining..."));
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
	DebugLogPrinter::Warning(TEXT("Search results are empty!"));
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result, const FString& Address)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		DebugLogPrinter::Error(FString::Printf(
			TEXT("Failed to join session with error \"%s\""),
			LexToString(Result)));
		return;
	}

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (PlayerController)
	{
		PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		DebugLogPrinter::Info("Session destroyed");
	}
	else
	{
		DebugLogPrinter::Error("Failed to destroy session!");
	}
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		DebugLogPrinter::Info("Session started");
	}
	else
	{
		DebugLogPrinter::Error("Failed to start session!");
	}
}

void UMenu::HostButtonClicked()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	if (MultiplayerSessionsSubsystem)
	{
		DebugLogPrinter::Info(TEXT("Searching..."));
		MultiplayerSessionsSubsystem->FindSessions(100000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputMapData;
			PlayerController->SetInputMode(InputMapData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
