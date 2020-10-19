#include "AssetSpawner.h"
#include "AssetModifier.h"
#include "Tags.h"
#include "Engine/StaticMeshActor.h"
#if WITH_EDITOR
#include "Editor.h"
#endif


bool FAssetSpawner::SpawnAsset(UWorld* World, const FSpawnAssetParams Params, FString &FinalActorName)
{
	//Check if World is avialable
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: Couldn't find the World."), *FString(__FUNCTION__));
		return false;
	}

	FString Label = (Params.ActorLabel.IsEmpty() ? Params.Name : Params.ActorLabel);
#if WITH_EDITOR
	GEditor->BeginTransaction(FText::FromString(TEXT("Spawning: ")+ Label));
	World->Modify();
#endif

	//Setup SpawnParameters
	FActorSpawnParameters SpawnParams;
	//SpawnParams.Instigator = Instigator;
	//SpawnParams.Owner = this;


	//Load Mesh and check if it succeded.
	UStaticMesh* Mesh = FAssetModifier::LoadMesh(Params.Name, Params.StartDir);
	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: Could not find Mesh: %s."), *FString(__FUNCTION__), *Params.Name);
#if WITH_EDITOR
		GEditor->EndTransaction();
#endif
		return false;
	}

	AStaticMeshActor* SpawnedItem;

	//Check if Id is used already
	TArray<AActor*> Actors = FTags::GetActorsWithKeyValuePair(World, TEXT("SemLog"), TEXT("Id"), Params.Id);

	if (!Actors.IsValidIndex(0))
	{
		//Actual Spawning MeshComponent
		SpawnedItem = World->SpawnActor<AStaticMeshActor>(Params.Location, Params.Rotator, SpawnParams);


		// Needs to be movable if the game is running.
		SpawnedItem->SetMobility(EComponentMobility::Movable);
		//Assigning the Mesh and Material to the Component
		SpawnedItem->GetStaticMeshComponent()->SetStaticMesh(Mesh);



		if (Params.MaterialPaths.Num())
		{
			for (int i = 0; i < Params.MaterialPaths.Num(); i++)
			{
				UMaterialInterface* Material = FAssetModifier::LoadMaterial(Params.MaterialNames[i], Params.MaterialPaths[i]);
				if (Material)
				{
					SpawnedItem->GetStaticMeshComponent()->SetMaterial(i, Material);
				}
			}
		}

#if WITH_EDITOR
		SpawnedItem->SetActorLabel(Label);
#endif

		FPhysicsProperties Properties = Params.PhysicsProperties;
		SpawnedItem->GetStaticMeshComponent()->SetSimulatePhysics(Properties.bSimulatePhysics);
		SpawnedItem->GetStaticMeshComponent()->SetGenerateOverlapEvents(Properties.bGenerateOverlapEvents);
		SpawnedItem->GetStaticMeshComponent()->SetEnableGravity(Properties.bGravity);
		SpawnedItem->GetStaticMeshComponent()->SetMassOverrideInKg(NAME_None, Properties.Mass);

		SpawnedItem->SetMobility(Properties.Mobility);

	}
	else
	{
		//ID is already taken
		UE_LOG(LogTemp, Error, TEXT("[%s]: Semlog id: \"%s\" is not unique, therefore nothing was spawned."), *FString(__FUNCTION__), *Params.Id);

#if WITH_EDITOR
	GEditor->EndTransaction();
#endif
		return false;
	}

	//Id tag to Actor
	FTags::AddKeyValuePair(
		SpawnedItem,
		TEXT("SemLog"),
		TEXT("id"),
		Params.Id);


	//Add other Tags to Actor
	for (auto Tag : Params.Tags)
	{
		FTags::AddKeyValuePair(
			SpawnedItem,
			Tag.TagType,
			Tag.Key,
			Tag.Value);
	}
#if WITH_EDITOR
	SpawnedItem->Modify();
	GEditor->EndTransaction();
#endif
	FinalActorName = SpawnedItem->GetName();

	return true;
}
