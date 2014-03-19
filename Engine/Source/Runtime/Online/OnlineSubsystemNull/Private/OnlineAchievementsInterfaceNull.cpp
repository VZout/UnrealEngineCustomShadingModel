// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemNullPrivatePCH.h"
#include "OnlineAchievementsInterfaceNull.h"

FOnlineAchievementsNull::FOnlineAchievementsNull(class FOnlineSubsystemNull* InSubsystem)
	:	NullSubsystem(InSubsystem)
{
	check(NullSubsystem);
}

bool FOnlineAchievementsNull::ReadAchievementsFromConfig()
{
	if (Achievements.Num() > 0)
	{
		return true;
	}

	NullAchievementsConfig Config;
	return Config.ReadAchievements(Achievements);
}

bool FOnlineAchievementsNull::WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		TriggerOnAchievementsWrittenDelegates(PlayerId, false);
		WriteObject->WriteState = EOnlineAsyncTaskState::Failed;
		return false;
	}

	FUniqueNetIdString NullId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(NullId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		TriggerOnAchievementsWrittenDelegates(PlayerId, false);
		WriteObject->WriteState = EOnlineAsyncTaskState::Failed;
		return false;
	}

	// treat each achievement as unlocked
	const int32 AchNum = PlayerAch->Num();
	for (FStatPropertyArray::TConstIterator It(WriteObject->Properties); It; ++It)
	{
		const FString AchievementId = It.Key().ToString();
		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			if ((*PlayerAch)[ AchIdx ].Id == AchievementId)
			{
				TriggerOnAchievementUnlockedDelegates(PlayerId, AchievementId);
				break;
			}
		}
	}

	WriteObject->WriteState = EOnlineAsyncTaskState::Done;
	TriggerOnAchievementsWrittenDelegates(PlayerId, true);

	return true;
};

bool FOnlineAchievementsNull::ReadAchievements(const FUniqueNetId& PlayerId)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		TriggerOnAchievementsReadDelegates(PlayerId, false);
		return false;
	}

	FUniqueNetIdString NullId(PlayerId);
	if (!PlayerAchievements.Find(NullId))
	{
		// copy for a new player
		TArray<FOnlineAchievement> AchievementsForPlayer;
		const int32 AchNum = Achievements.Num();

		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			AchievementsForPlayer.Add( Achievements[ AchIdx ] );
		}

		PlayerAchievements.Add(NullId, AchievementsForPlayer);
	}

	TriggerOnAchievementsReadDelegates(PlayerId, true);
	return true;
};

bool FOnlineAchievementsNull::ReadAchievementDescriptions(const FUniqueNetId& PlayerId)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		TriggerOnAchievementsReadDelegates(PlayerId, false);
		return false;
	}

	if (AchievementDescriptions.Num() == 0)
	{
		const int32 AchNum = Achievements.Num();
		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			AchievementDescriptions.Add(Achievements[AchIdx].Id, Achievements[AchIdx]);
		}

		check(AchievementDescriptions.Num() > 0);
	}

	TriggerOnAchievementDescriptionsReadDelegates(PlayerId, true);
	return true;
};

bool FOnlineAchievementsNull::GetAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return false;
	}

	FUniqueNetIdString NullId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(NullId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		return false;
	}

	const int32 AchNum = PlayerAch->Num();
	for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
	{
		if ((*PlayerAch)[ AchIdx ].Id == AchievementId)
		{
			OutAchievement = (*PlayerAch)[ AchIdx ];
			return true;
		}
	}

	// no such achievement
	return false;
};

bool FOnlineAchievementsNull::GetAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement> & OutAchievements)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return false;
	}

	FUniqueNetIdString NullId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(NullId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		return false;
	}

	OutAchievements = *PlayerAch;
	return true;
};

bool FOnlineAchievementsNull::GetAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return false;
	}

	if (AchievementDescriptions.Num() == 0 )
	{
		// don't have descs
		return false;
	}

	FOnlineAchievementDesc * AchDesc = AchievementDescriptions.Find(AchievementId);
	if (NULL == AchDesc)
	{
		// no such achievement
		return false;
	}

	OutAchievementDesc = *AchDesc;
	return true;
};

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsNull::ResetAchievements(const FUniqueNetId& PlayerId)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		UE_LOG_ONLINE(Warning, TEXT("No achievements have been configured"));
		return false;
	}

	FUniqueNetIdString NullId(PlayerId);
	TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(NullId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		UE_LOG_ONLINE(Warning, TEXT("Could not find achievements for player %s"), *PlayerId.ToString());
		return false;
	}

	// treat each achievement as unlocked
	const int32 AchNum = PlayerAch->Num();
	for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
	{
		(*PlayerAch)[ AchIdx ].Progress = 0.0;
	}

	return true;
};
#endif // !UE_BUILD_SHIPPING