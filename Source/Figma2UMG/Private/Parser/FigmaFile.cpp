// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/FigmaFile.h"

#include "Figma2UMGModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/Async.h"
#include "Builder/Asset/WidgetBlueprintBuilder.h"
#include "ObjectTools.h"
#include "Parser/FigmaJsonImport.h"
#include "Parser/Nodes/FigmaDocument.h"
#include "Parser/Nodes/FigmaInstance.h"
#include "REST/FigmaImporter.h"
#include "Parser/Properties/FigmaComponentRef.h"
#include "Dom/JsonObject.h"
#include "WidgetBlueprint.h"

namespace
{
	bool HasExistingWidgetBlueprintForNode(const UFigmaNode& Node)
	{
		if (!Node.HasWidgetBlueprintPrefix())
		{
			return false;
		}

		const FString AssetName = ObjectTools::SanitizeInvalidChars(Node.GetNodeName().TrimStartAndEnd(), INVALID_OBJECTNAME_CHARACTERS);
		if (AssetName.IsEmpty())
		{
			return false;
		}

		FARFilter Filter;
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetDataList;
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetName.IsEqual(FName(*AssetName), ENameCase::CaseSensitive))
			{
				UE_LOG_Figma2UMG(Display, TEXT("[UFigmaFile] Reusing existing WidgetBlueprint %s for WBP subtree %s; child asset generation is skipped."), *AssetData.GetObjectPathString(), *Node.GetNodeName());
				return true;
			}
		}

		return false;
	}
}

void UFigmaFile::PostSerialize(const FString& InFileKey, const FString& InPackagePath, const TSharedRef<FJsonObject> fileJsonObject, const FString& InPrimaryImportNodeId, const TArray<FString>& InAdditionalImportNodeIds)
{
	static FString DocumentStr("Document");
	FileKey = InFileKey;
	PackagePath = InPackagePath;
	PrimaryImportNodeId = InPrimaryImportNodeId;
	AdditionalImportNodeIds.Reset();
	for (const FString& NodeId : InAdditionalImportNodeIds)
	{
		const FString TrimmedNodeId = NodeId.TrimStartAndEnd();
		if (!TrimmedNodeId.IsEmpty() && !TrimmedNodeId.Equals(PrimaryImportNodeId, ESearchCase::CaseSensitive))
		{
			AdditionalImportNodeIds.AddUnique(TrimmedNodeId);
		}
	}
	ImportAssetNameOverride.Reset();

	const TSharedPtr<FJsonObject>* DocumentJsonObject = nullptr;
	if (!fileJsonObject->TryGetObjectField(DocumentStr, DocumentJsonObject) || !DocumentJsonObject || !DocumentJsonObject->IsValid())
	{
		static const FString StandardDocumentStr("document");
		if (!fileJsonObject->TryGetObjectField(StandardDocumentStr, DocumentJsonObject) || !DocumentJsonObject || !DocumentJsonObject->IsValid())
		{
			UE_LOG_Figma2UMG(Error, TEXT("[UFigmaFile::PostSerialize] Document field was not found in Figma file %s."), *Name);
			return;
		}
	}

	Document = NewObject<UFigmaDocument>();
	FText OutFailReason;
	if (!FigmaJsonImport::JsonObjectToUStruct((*DocumentJsonObject).ToSharedRef(), Document->GetClass(), Document, &OutFailReason))
	{
		UE_LOG_Figma2UMG(Error, TEXT("[UFigmaFile::PostSerialize] Failed to parse Document for Figma file %s. %s"), *Name, *OutFailReason.ToString());
		Document = nullptr;
		return;
	}

	Document->SetFigmaFile(this);
	Document->PostSerialize(nullptr, (*DocumentJsonObject).ToSharedRef());
	UpdateImportAssetNameFromPrimaryNodeId();
}

FString UFigmaFile::GetUAssetName() const
{
	return ImportAssetNameOverride.IsEmpty() ? Name : ImportAssetNameOverride;
}

FString UFigmaFile::FindComponentName(const FString& ComponentId)
{
	if (Components.Contains(ComponentId))
	{
		const FFigmaComponentRef& Component = Components[ComponentId];
		return Component.Name;
	}

	return FString();
}

FFigmaComponentRef* UFigmaFile::FindComponentRef(const FString& ComponentId)
{
	if (Components.Contains(ComponentId))
	{
		FFigmaComponentRef& Component = Components[ComponentId];
		return &Component;
	}

	return nullptr;
}

FFigmaComponentRef* UFigmaFile::FindComponentRefByKey(const FString& Key)
{
	for (TPair<FString, FFigmaComponentRef>& Element : Components)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return &Element.Value;
		}
	}

	return nullptr;
}

TObjectPtr<UFigmaComponent> UFigmaFile::FindComponentByKey(const FString& Key)
{
	for (const TPair<FString, FFigmaComponentRef>& Element : Components)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return FindByID<UFigmaComponent>(Element.Key);
		}
	}

	return nullptr;
}

const TObjectPtr<UFigmaComponent> UFigmaFile::FindComponentByKey(const FString& Key) const
{
	for (const TPair<FString, FFigmaComponentRef>& Element : Components)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return FindByID<UFigmaComponent>(Element.Key);
		}
	}

	return nullptr;
}

FString UFigmaFile::FindComponentSetName(const FString& ComponentSetId)
{
	if (ComponentSets.Contains(ComponentSetId))
	{
		const FFigmaComponentSetRef& ComponentSet = ComponentSets[ComponentSetId];
		return ComponentSet.Name;
	}

	return FString();
}

FFigmaComponentSetRef* UFigmaFile::FindComponentSetRef(const FString& ComponentSetId)
{
	if (ComponentSets.Contains(ComponentSetId))
	{
		FFigmaComponentSetRef& ComponentSet = ComponentSets[ComponentSetId];
		return &ComponentSet;
	}

	return nullptr;
}

FFigmaComponentSetRef* UFigmaFile::FindComponentSetRefByKey(const FString& Key)
{
	for (TPair<FString, FFigmaComponentSetRef>& Element : ComponentSets)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return &Element.Value;
		}
	}

	return nullptr;
}

TObjectPtr<UFigmaComponentSet> UFigmaFile::FindComponentSetByKey(const FString& Key)
{
	for (const TPair<FString, FFigmaComponentSetRef>& Element : ComponentSets)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return FindByID<UFigmaComponentSet>(Element.Key);
		}
	}

	return nullptr;
}

const TObjectPtr<UFigmaComponentSet> UFigmaFile::FindComponentSetByKey(const FString& Key) const
{
	for (const TPair<FString, FFigmaComponentSetRef>& Element : ComponentSets)
	{
		if (Element.Value.Remote)
			continue;

		if (Element.Value.Key == Key)
		{
			return FindByID<UFigmaComponentSet>(Element.Key);
		}
	}

	return nullptr;
}

void UFigmaFile::PrepareForFlow()
{
	if (!Document)
		return;

	Document->PrepareForFlow();
}

void UFigmaFile::FixComponentSetRef()
{
	for (TPair<FString, FFigmaComponentRef>& ComponentPair : Components)
	{
		if (ComponentPair.Value.ComponentSetId.IsEmpty())
			continue;

		if (ComponentSets.Contains(ComponentPair.Value.ComponentSetId))
		{
			ComponentPair.Value.SetComponentSet(&ComponentSets[ComponentPair.Value.ComponentSetId]);
		}
		else
		{
			UE_LOG_Figma2UMG(Error, TEXT("File %s's Component %s is part of a ComponentSet %s not found."), *Name, *ComponentPair.Value.Name, *ComponentPair.Value.ComponentSetId);
		}
	}
}

void UFigmaFile::FixRemoteReferences(const TMap<FString, TObjectPtr<UFigmaFile>>& LibraryFiles)
{
	FixRemoteComponentSetReferences(LibraryFiles);
	FixRemoteComponentReferences(LibraryFiles);

	for (TPair<FString, FFigmaComponentSetRef>& ComponentSetPair : ComponentSets)
	{
		if (!ComponentSetPair.Value.Remote)
			continue;

		TObjectPtr<UFigmaComponentSet> ComponentSet = ComponentSetPair.Value.GetComponentSet();
		if (ComponentSet)
		{
			TObjectPtr<UFigmaFile> OriginalFile = ComponentSet->GetFigmaFile();
			if (OriginalFile && OriginalFile->Components.Contains(ComponentSetPair.Key))
			{
				OriginalFile->ComponentSets[ComponentSetPair.Key].SetComponentSet(ComponentSet);
			}
		}
	}

	for (TPair<FString, FFigmaComponentRef>& ComponentPair : Components)
	{
		if (!ComponentPair.Value.Remote)
			continue;

		TObjectPtr<UFigmaComponent> Component = ComponentPair.Value.GetComponent();
		if (Component)
		{
			TObjectPtr<UFigmaFile> OriginalFile = Component->GetFigmaFile();
			if (OriginalFile && OriginalFile->Components.Contains(ComponentPair.Key))
			{
				OriginalFile->Components[ComponentPair.Key].SetComponent(Component);
			}
		}
	}
}

void UFigmaFile::SetImporter(UFigmaImporter* InFigmaImporter)
{
	FigmaImporter = InFigmaImporter;
}

UFigmaImporter* UFigmaFile::GetImporter() const
{
	return FigmaImporter;
}

TObjectPtr<UFigmaNode> UFigmaFile::GetPrimaryImportNode() const
{
	if (PrimaryImportNodeId.IsEmpty())
	{
		return nullptr;
	}

	return FindByID<UFigmaNode>(PrimaryImportNodeId);
}

void UFigmaFile::GetAdditionalImportNodes(TArray<TObjectPtr<UFigmaNode>>& OutNodes) const
{
	for (const FString& NodeId : AdditionalImportNodeIds)
	{
		const TObjectPtr<UFigmaNode> Node = FindByID<UFigmaNode>(NodeId);
		if (Node)
		{
			OutNodes.AddUnique(Node);
		}
		else
		{
			UE_LOG_Figma2UMG(Warning, TEXT("Could not find additional Figma node %s while resolving import roots."), *NodeId);
		}
	}
}

void UFigmaFile::UpdateImportAssetNameFromPrimaryNodeId()
{
	if (PrimaryImportNodeId.IsEmpty() || !Document)
	{
		return;
	}

	if (TObjectPtr<UFigmaNode> PrimaryImportNode = GetPrimaryImportNode())
	{
		const FString PrimaryImportNodeName = PrimaryImportNode->GetNodeName().TrimStartAndEnd();
		if (!PrimaryImportNodeName.IsEmpty())
		{
			ImportAssetNameOverride = PrimaryImportNodeName;
			UE_LOG_Figma2UMG(Display, TEXT("Using Figma node %s layer name '%s' as imported root UMG asset name."), *PrimaryImportNodeId, *ImportAssetNameOverride);
		}
	}
	else
	{
		UE_LOG_Figma2UMG(Warning, TEXT("Could not find Figma node %s while resolving imported root UMG asset name. Falling back to file name '%s'."), *PrimaryImportNodeId, *Name);
	}
}

void UFigmaFile::FixRemoteComponentReferences(const TMap<FString, TObjectPtr<UFigmaFile>>& LibraryFiles)
{
	TMap<FString, FFigmaComponentRef> PendingComponents;
	for (TPair<FString, FFigmaComponentRef>& ComponentPair : Components)
	{
		if (!ComponentPair.Value.Remote)
			continue;

		if (ComponentPair.Value.GetComponent() != nullptr)
			continue;

		for (const TPair<FString, TObjectPtr<UFigmaFile>> LibraryFile : LibraryFiles)
		{
			TObjectPtr<UFigmaComponent> Component = LibraryFile.Value->FindComponentByKey(ComponentPair.Value.Key);
			if (Component != nullptr)
			{
				AddRemoteComponent(ComponentPair.Value, LibraryFile, Component, PendingComponents);
				break;
			}
		}
	}

	if (PendingComponents.Num() > 0)
	{
		for (TPair<FString, FFigmaComponentRef>& Element : PendingComponents)
		{
			Element.Value.Remote = true;
			Components.Add(Element);
		}

		FixRemoteComponentReferences(LibraryFiles);
	}
}

void UFigmaFile::FixRemoteComponentSetReferences(const TMap<FString, TObjectPtr<UFigmaFile>>& LibraryFiles)
{
	TMap<FString, FFigmaComponentRef> PendingComponents;
	TMap<FString, FFigmaComponentSetRef> PendingComponentSets;
	for (TPair<FString, FFigmaComponentSetRef>& ComponentSetPair : ComponentSets)
	{
		if (!ComponentSetPair.Value.Remote)
			continue;

		if (ComponentSetPair.Value.GetComponentSet() != nullptr)
			continue;

		for (const TPair<FString, TObjectPtr<UFigmaFile>> LibraryFile : LibraryFiles)
		{
			TObjectPtr<UFigmaComponentSet> Component = LibraryFile.Value->FindComponentSetByKey(ComponentSetPair.Value.Key);
			if (Component != nullptr)
			{
				AddRemoteComponentSet(ComponentSetPair.Value, LibraryFile, Component, PendingComponents, PendingComponentSets);
				break;
			}
		}
	}

	if (PendingComponents.Num() > 0)
	{
		for (TPair<FString, FFigmaComponentRef>& Element : PendingComponents)
		{
			Element.Value.Remote = true;
			Components.Add(Element);
		}
	}

	if (PendingComponentSets.Num() > 0)
	{
		for (TPair<FString, FFigmaComponentSetRef>& Element : PendingComponentSets)
		{
			Element.Value.Remote = true;
			ComponentSets.Add(Element);
		}

		FixRemoteComponentSetReferences(LibraryFiles);
	}
}

void UFigmaFile::CreateAssetBuilders(const FProcessFinishedDelegate& ProcessDelegate, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	CurrentProcessDelegate = ProcessDelegate;
	AsyncTask(ENamedThreads::GameThread, [&]()
		{
			FGCScopeGuard GCScopeGuard;
			for (TPair<FString, FFigmaComponentSetRef>& ComponentSetPair : ComponentSets)
			{
				if (!ComponentSetPair.Value.Remote)
					continue;

				TObjectPtr<UFigmaComponentSet> ComponentSet = ComponentSetPair.Value.GetComponentSet();
				if (ComponentSet)
				{
					if (!CreateAssetBuilder(ComponentSetPair.Value.RemoteFileKey, *ComponentSet, AssetBuilders))
					{
						UE_LOG_Figma2UMG(Warning, TEXT("[Asset] ComponentSet %s Id %s failed to return a AssetBuilder"), *ComponentSet->GetNodeName(), *ComponentSet->GetIdForName());
					}
				}
			}

			for (TPair<FString, FFigmaComponentRef>& ComponentPair : Components)
			{
				if (!ComponentPair.Value.Remote)
					continue;

				TObjectPtr<UFigmaComponent> Component = ComponentPair.Value.GetComponent();
				if (Component)
				{
					if (!CreateAssetBuilder(ComponentPair.Value.RemoteFileKey, *Component, AssetBuilders))
					{
						UE_LOG_Figma2UMG(Warning, TEXT("[Asset] Component %s Id %s failed to return a AssetBuilder"), *Component->GetNodeName(), *Component->GetIdForName());
					}
				}
			}

			if (Document)
			{
				if (PrimaryImportNodeId.IsEmpty())
				{
					CreateAssetBuilder(FileKey, *Document, AssetBuilders);
				}
				else
				{
					Document->CreateAssetBuilder(FileKey, AssetBuilders);
					if (!CreatePrimaryImportRootAssetBuilders(AssetBuilders))
					{
						ExecuteDelegate(false);
						return;
					}
				}

				ExecuteDelegate(true);
			}
			else
			{
				ExecuteDelegate(false);
			}
		});
}

void UFigmaFile::AddRemoteComponent(FFigmaComponentRef& ComponentRef, const TPair<FString, TObjectPtr<UFigmaFile>>& LibraryFile, TObjectPtr<UFigmaComponent> Component, TMap<FString, FFigmaComponentRef>& PendingComponents)
{
	UE_LOG_Figma2UMG(Display, TEXT("[Component] Adding remote Component %s key:%s"), *ComponentRef.Name, *ComponentRef.Key);

	ComponentRef.RemoteFileKey = LibraryFile.Key;
	ComponentRef.SetComponent(Component);
	if (!ComponentRef.ComponentSetId.IsEmpty())
	{
		if (FFigmaComponentSetRef* RemoteComponentSet = LibraryFile.Value->FindComponentSetRef(ComponentRef.ComponentSetId))
		{
			ComponentRef.SetComponentSet(RemoteComponentSet);
		}
		else if (FFigmaComponentSetRef* ComponentSet = this->FindComponentSetRef(ComponentRef.ComponentSetId))
		{
			ComponentRef.SetComponentSet(ComponentSet);
		}
		else
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[Component] Can't find ComponentSet id %s"), *ComponentRef.ComponentSetId);
		}
	}

	TArray<UFigmaInstance*> SubInstances;
	Component->GetAllChildrenByType(SubInstances);

	for (UFigmaInstance* SubInstance : SubInstances)
	{
		if (PendingComponents.Contains(SubInstance->ComponentId))
			continue;

		if (LibraryFile.Value->Components.Contains(SubInstance->ComponentId))
		{
			UE_LOG_Figma2UMG(Display, TEXT("[Component] Adding dependency to Component %s id %s"), *SubInstance->GetNodeName(), *SubInstance->ComponentId);
			FFigmaComponentRef& RemoteCommponentRef = LibraryFile.Value->Components[SubInstance->ComponentId];
			if (!RemoteCommponentRef.Remote)
			{
				RemoteCommponentRef.RemoteFileKey = LibraryFile.Key;
			}
			FFigmaComponentRef& SubComponentRef = PendingComponents.Add(SubInstance->ComponentId, RemoteCommponentRef);
		}
	}
}

void UFigmaFile::AddRemoteComponentSet(FFigmaComponentSetRef& ComponentSetRef, const TPair<FString, TObjectPtr<UFigmaFile>>& LibraryFile, TObjectPtr<UFigmaComponentSet> ComponentSet, TMap<FString, FFigmaComponentRef>& PendingComponents, TMap<FString, FFigmaComponentSetRef>& PendingComponentSets)
{
	UE_LOG_Figma2UMG(Display, TEXT("[ComponentSet] Adding remote ComponentSet %s key:%s"), *ComponentSetRef.Name, *ComponentSetRef.Key);

	ComponentSetRef.RemoteFileKey = LibraryFile.Key;
	ComponentSetRef.SetComponentSet(ComponentSet);

	TArray<UFigmaComponent*> SubComponents;
	ComponentSet->GetAllChildrenByType(SubComponents);

	for (const UFigmaComponent* SubComponent : SubComponents)
	{
		if (PendingComponents.Contains(SubComponent->GetId()))
			continue;

		if (LibraryFile.Value->Components.Contains(SubComponent->GetId()))
		{
			UE_LOG_Figma2UMG(Display, TEXT("[ComponentSet] Adding dependency to Component %s id %s"), *SubComponent->GetNodeName(), *SubComponent->GetId());
			FFigmaComponentRef& RemoteCommponentRef = LibraryFile.Value->Components[SubComponent->GetId()];
			if (!RemoteCommponentRef.Remote)
			{
				RemoteCommponentRef.RemoteFileKey = LibraryFile.Key;
			}
			FFigmaComponentRef& SubComponentRef = PendingComponents.Add(SubComponent->GetId(), RemoteCommponentRef);
		}
	}


	TArray<UFigmaComponentSet*> SubComponentSets;
	ComponentSet->GetAllChildrenByType(SubComponentSets);
	for (const UFigmaComponentSet* SubComponentSet : SubComponentSets)
	{
		if (PendingComponentSets.Contains(SubComponentSet->GetId()))
			continue;

		if (LibraryFile.Value->ComponentSets.Contains(SubComponentSet->GetId()))
		{
			UE_LOG_Figma2UMG(Display, TEXT("[ComponentSet] Adding dependency to ComponentSet %s id %s"), *SubComponentSet->GetNodeName(), *SubComponentSet->GetId());
			FFigmaComponentSetRef& RemoteComponentSetRef = LibraryFile.Value->ComponentSets[SubComponentSet->GetId()];
			if (!RemoteComponentSetRef.Remote)
			{
				RemoteComponentSetRef.RemoteFileKey = LibraryFile.Key;
			}
			FFigmaComponentSetRef& SubComponentRef = PendingComponentSets.Add(SubComponentSet->GetId(), RemoteComponentSetRef);
		}
	}
}

void UFigmaFile::ExecuteDelegate(const bool Succeeded)
{
	if (CurrentProcessDelegate.ExecuteIfBound(Succeeded))
	{
//		CurrentProcessDelegate.Unbind();
	}
}

bool UFigmaFile::CreatePrimaryImportRootAssetBuilders(TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	TObjectPtr<UFigmaNode> PrimaryImportNode = GetPrimaryImportNode();
	if (!PrimaryImportNode)
	{
		UE_LOG_Figma2UMG(Error, TEXT("Could not find Figma node %s while resolving import root."), *PrimaryImportNodeId);
		return false;
	}

	if (IFigmaContainer* ImportRootContainer = Cast<IFigmaContainer>(PrimaryImportNode))
	{
		TArray<TObjectPtr<UFigmaNode>>& RootChildren = ImportRootContainer->GetChildren();
		if (!RootChildren.IsEmpty())
		{
			UE_LOG_Figma2UMG(Display, TEXT("Using Figma node %s as import start. %d direct child node(s) will be imported as UMG root content."), *PrimaryImportNodeId, RootChildren.Num());
			for (UFigmaNode* ChildNode : RootChildren)
			{
				if (ChildNode)
				{
					CreateAssetBuilder(FileKey, *ChildNode, AssetBuilders);
				}
			}

			TArray<TObjectPtr<UFigmaNode>> AdditionalImportNodes;
			GetAdditionalImportNodes(AdditionalImportNodes);
			for (UFigmaNode* AdditionalImportNode : AdditionalImportNodes)
			{
				if (AdditionalImportNode)
				{
					CreateAssetBuilder(FileKey, *AdditionalImportNode, AssetBuilders);
				}
			}

			return true;
		}
	}

	UE_LOG_Figma2UMG(Warning, TEXT("Figma node %s has no direct child nodes. Falling back to importing the selected node itself as root content."), *PrimaryImportNodeId);
	CreateAssetBuilder(FileKey, *PrimaryImportNode, AssetBuilders);
	TArray<TObjectPtr<UFigmaNode>> AdditionalImportNodes;
	GetAdditionalImportNodes(AdditionalImportNodes);
	for (UFigmaNode* AdditionalImportNode : AdditionalImportNodes)
	{
		if (AdditionalImportNode)
		{
			CreateAssetBuilder(FileKey, *AdditionalImportNode, AssetBuilders);
		}
	}
	return true;
}

bool UFigmaFile::CreateAssetBuilder(const FString& InFileKey, UFigmaNode& Node, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (Node.HasProjectTextureReferencePrefix())
	{
		return false;
	}

	bool Created = Node.CreateAssetBuilder(InFileKey, AssetBuilders);
	if (Node.HasTextureOnlyImagePrefix())
	{
		return Created;
	}
	if (HasExistingWidgetBlueprintForNode(Node))
	{
		return Created;
	}

	if (IFigmaContainer* FigmaContainer = Cast<IFigmaContainer>(&Node))
	{
		FigmaContainer->ForEach(IFigmaContainer::FOnEachFunction::CreateLambda([&](UFigmaNode& ChildNode, const int Index)
			{
				CreateAssetBuilder(InFileKey, ChildNode, AssetBuilders);
			}));
	}

	return Created;
}
