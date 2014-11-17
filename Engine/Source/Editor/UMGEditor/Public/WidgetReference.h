// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UWidget;
class SWidget;
class UWidgetBlueprint;

/**
 * The Widget reference is a useful way to hold onto the selection in a way that allows for up to date access to the current preview object.
 * Because the designer could end up rebuilding the preview, it's best to hold onto an FWidgetReference.
 */
struct UMGEDITOR_API FWidgetReference
{
public:
	/**
	 * Creates a widget reference using the template.
	 */
	static FWidgetReference FromTemplate(TSharedPtr<class FWidgetBlueprintEditor> InWidgetEditor, UWidget* TemplateWidget);

	/**
	 * Creates a widget reference using the preview.  Which is used to lookup the stable template pointer.
	 */
	static FWidgetReference FromPreview(TSharedPtr<class FWidgetBlueprintEditor> InWidgetEditor, UWidget* PreviewWidget);

	/** Constructor for the null widget reference.  Not intended for normal use. */
	FWidgetReference();

	/** Destructor */
	~FWidgetReference();

	/** @returns true if both the template and the preview pointers are valid. */
	bool IsValid() const;

	/** @returns The template widget.  This is the widget that is serialized to disk */
	UWidget* GetTemplate() const;

	/** @returns the preview widget.  This is the transient representation of the template.  Constantly being destroyed and recreated.  Do not cache this pointer. */
	UWidget* GetPreview() const;

	/** Checks if widget reference is the same as nother widget reference, based on the template pointers. */
	bool operator==( const FWidgetReference& Other ) const
	{
		return TemplateWidget.Get() == Other.TemplateWidget.Get();
	}

private:
	FWidgetReference(TSharedPtr<class FWidgetBlueprintEditor> WidgetEditor, UWidget* TemplateWidget);

	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);

private:

	TWeakPtr<class FWidgetBlueprintEditor> WidgetEditor;

	//TODO UMG We probably want to make this a weak object reference.
	TWeakObjectPtr<UWidget> TemplateWidget;
};

inline uint32 GetTypeHash(const struct FWidgetReference& WidgetRef)
{
	return ::GetTypeHash(reinterpret_cast<void*>( WidgetRef.GetTemplate() ));
}