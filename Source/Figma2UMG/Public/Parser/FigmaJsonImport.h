// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "JsonObjectConverter.h"

namespace FigmaJsonImport
{
	inline constexpr int64 CheckFlags = 0;
	inline constexpr int64 SkipFlags = CPF_Transient;
	inline constexpr bool StrictMode = false;

	inline bool ImportNullAsDefault(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* Value)
	{
		if (!JsonValue.IsValid() || !JsonValue->IsNull())
		{
			return false;
		}

		if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			StringProperty->SetPropertyValue(Value, FString());
		}
		else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			NameProperty->SetPropertyValue(Value, NAME_None);
		}
		else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			TextProperty->SetPropertyValue(Value, FText::GetEmpty());
		}

		return true;
	}

	inline const FJsonObjectConverter::CustomImportCallback& GetImportCallback()
	{
		static const FJsonObjectConverter::CustomImportCallback Callback = FJsonObjectConverter::CustomImportCallback::CreateStatic(&ImportNullAsDefault);
		return Callback;
	}

	inline bool JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, const UStruct* StructDefinition, void* OutStruct, FText* OutFailReason = nullptr)
	{
		return FJsonObjectConverter::JsonObjectToUStruct(JsonObject, StructDefinition, OutStruct, CheckFlags, SkipFlags, StrictMode, OutFailReason, &GetImportCallback());
	}

	template<typename OutStructType>
	inline bool JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, OutStructType* OutStruct, FText* OutFailReason = nullptr)
	{
		return FJsonObjectConverter::JsonObjectToUStruct(JsonObject, OutStruct, CheckFlags, SkipFlags, StrictMode, OutFailReason, &GetImportCallback());
	}
}
