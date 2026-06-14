#include "NetGameMode.h"
#include "NetBaseCharacter.h"
#include "NetGameState.h"
#include "NetPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"

ANetGameMode::ANetGameMode()
{
    DefaultPawnClass = ANetBaseCharacter::StaticClass();
    PlayerStateClass = ANetPlayerState::StaticClass();
    GameStateClass = ANetGameState::StaticClass();

    NextBluePlayer = nullptr;
    ConnectedPlayers = 0;

    MatchDuration = 30.0f;
}

AActor* ANetGameMode::GetPlayerStart(FString Name, int Index)
{
    FName PSName;
    if (Index < 0) {
        PSName = *Name;
    }
    else {
        PSName = *FString::Printf(TEXT("%s%d"), *Name, Index % 4);
    }

    for (TActorIterator<APlayerStart> It(GWorld); It; ++It)
    {
        if (APlayerStart* PS = Cast<APlayerStart>(*It))
        {
            if (PS->PlayerStartTag == PSName) return *It;
        }
    }
    return nullptr;
}

AActor* ANetGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AActor* Start = AssignTeamAndPlayerStart(Player);
    return Start ? Start : Super::ChoosePlayerStart_Implementation(Player);
}

AActor* ANetGameMode::AssignTeamAndPlayerStart(AController* Player)
{
    AActor* Start = nullptr;
    ANetPlayerState* State = Player->GetPlayerState<ANetPlayerState>();

    if (State) {
        if (TotalGames == 0) {
            State->TeamID = TotalPlayerCount == 0 ? EPlayerTeam::TEAM_Blue : EPlayerTeam::TEAM_Red;
        }
        else {
            State->TeamID = (State == NextBluePlayer) ? EPlayerTeam::TEAM_Blue : EPlayerTeam::TEAM_Red;
        }

        State->ClosestApproachToBlue = 550.0f;

        if (State->TeamID == EPlayerTeam::TEAM_Blue) {
            Start = GetPlayerStart("Blue", -1);
        }
        else {
            Start = GetPlayerStart("Red", PlayerStartIndex++);
        }

        State->PlayerIndex = TotalPlayerCount++;
        AllPlayers.Add(Cast<APlayerController>(Player));

        ConnectedPlayers++;
        if (ConnectedPlayers == 1)
        {
            ANetGameState* GState = GetGameState<ANetGameState>();
            if (GState) {
                GState->TimeRemaining = FMath::RoundToInt(MatchDuration);
            }
        }
        else if (ConnectedPlayers >= 2)
        {
            GetWorld()->GetTimerManager().SetTimer(SwapTimerHandle, this, &ANetGameMode::AdvanceTimer, 1.0f, true);
        }
    }
    return Start;
}

void ANetGameMode::AdvanceTimer()
{
    ANetGameState* GState = GetGameState<ANetGameState>();

    if (!GState || GState->WinningPlayer >= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
        return;
    }

    if (GState->TimeRemaining <= 0)
    {
        GState->TimeRemaining = FMath::RoundToInt(MatchDuration);
        return;
    }

    GState->TimeRemaining--;

    if (GState->TimeRemaining <= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
        TimeoutAndSwap();
    }
}

void ANetGameMode::AvatarsOverlapped(ANetAvatar* AvatarA, ANetAvatar* AvatarB)
{
    if (!AvatarA || !AvatarB) return;

    ANetGameState* GState = GetGameState<ANetGameState>();
    if (GState == nullptr || GState->WinningPlayer >= 0) return;

    ANetPlayerState* StateA = AvatarA->GetPlayerState<ANetPlayerState>();
    ANetPlayerState* StateB = AvatarB->GetPlayerState<ANetPlayerState>();

    if (StateA == nullptr || StateB == nullptr) return;

    if (StateA->TeamID == StateB->TeamID) return;

    GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);

    if (StateA->TeamID == EPlayerTeam::TEAM_Red) {
        GState->WinningPlayer = StateA->PlayerIndex;
        NextBluePlayer = StateA;
    }
    else {
        GState->WinningPlayer = StateB->PlayerIndex;
        NextBluePlayer = StateB;
    }

    AvatarA->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    AvatarB->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    GState->OnVictory();

    TotalGames++;
    FTimerHandle EndGameTimerHandle;
    GWorld->GetTimerManager().SetTimer(EndGameTimerHandle, this, &ANetGameMode::EndGame, 2.5f, false);
}

void ANetGameMode::EndGame()
{
    PlayerStartIndex = 0;
    ConnectedPlayers = 0;
    GetGameState<ANetGameState>()->WinningPlayer = -1;
    GetGameState<ANetGameState>()->TimeRemaining = 0;

    for (APlayerController* Player : AllPlayers)
    {
        APawn* Pawn = Player->GetPawn();
        Player->UnPossess();
        if (Pawn) { Pawn->Destroy(); }
        Player->StartSpot.Reset();
        RestartPlayer(Player);
    }

    ANetGameState* GState = GetGameState<ANetGameState>();
    GState->TriggerRestart();
}

// SWAP LOGIC EXPLANATION 
// Logic: Pick the Red team member who performed the worst as the next Blue player.
// This is determined by finding the Red player with the highest 'ClosestApproachToBlue' value 
// (meaning they stayed the furthest away from the Blue player throughout the match).

void ANetGameMode::TimeoutAndSwap()
{
    ANetGameState* GState = GetGameState<ANetGameState>();
    if (GState == nullptr || GState->WinningPlayer >= 0) return;

    int BluePlayerIndex = -1;
    ANetPlayerState* WorstRedState = nullptr;
    float MaxDistance = -1.0f;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It)
    {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();
        if (PS)
        {
            if (PS->TeamID == EPlayerTeam::TEAM_Blue) {
                BluePlayerIndex = PS->PlayerIndex;
            }
            else if (PS->TeamID == EPlayerTeam::TEAM_Red) {
                if (PS->ClosestApproachToBlue > MaxDistance) {
                    MaxDistance = PS->ClosestApproachToBlue;
                    WorstRedState = PS;
                }
            }
        }
    }

    if (BluePlayerIndex != -1) {
        GState->WinningPlayer = BluePlayerIndex;
    }

    if (WorstRedState) {
        NextBluePlayer = WorstRedState;
    }

    GState->OnVictory();
    TotalGames++;

    FTimerHandle EndGameTimerHandle;
    GWorld->GetTimerManager().SetTimer(EndGameTimerHandle, this, &ANetGameMode::EndGame, 2.5f, false);
}

void ANetGameMode::ReportDash(ANetAvatar* DashingAvatar, FVector DashStart, FVector DashEnd)
{
    if (!DashingAvatar) return;

    ANetPlayerState* DasherState = DashingAvatar->GetPlayerState<ANetPlayerState>();
    if (!DasherState || DasherState->TeamID != EPlayerTeam::TEAM_Red) return;

    FVector BlueLoc = FVector::ZeroVector;
    bool bFoundBlue = false;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It) {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();
        if (PS && PS->TeamID == EPlayerTeam::TEAM_Blue) {
            BlueLoc = It->GetActorLocation();
            bFoundBlue = true;
            break;
        }
    }

    if (bFoundBlue) {
        FVector ClosestPoint = FMath::ClosestPointOnSegment(BlueLoc, DashStart, DashEnd);
        float Dist = FVector::Dist(BlueLoc, ClosestPoint);

        if (Dist < DasherState->ClosestApproachToBlue) {
            DasherState->ClosestApproachToBlue = Dist;
        }
    }
}