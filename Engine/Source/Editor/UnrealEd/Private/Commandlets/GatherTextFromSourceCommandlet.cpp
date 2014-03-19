// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

DEFINE_LOG_CATEGORY_STATIC(LogGatherTextFromSourceCommandlet, Log, All);

//////////////////////////////////////////////////////////////////////////
//GatherTextFromSourceCommandlet

#define LOC_DEFINE_REGION

UGatherTextFromSourceCommandlet::UGatherTextFromSourceCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::DefineString(TEXT("#define "));
const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::UndefString(TEXT("#undef "));
const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::LocNamespaceStringOld(TEXT("LOC_NAMESPACE"));
const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::LocNamespaceString(TEXT("LOCTEXT_NAMESPACE"));
const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::LocDefRegionString(TEXT("LOC_DEFINE_REGION"));
const FString UGatherTextFromSourceCommandlet::FPreProcessorDescriptor::IniNamespaceString(TEXT("["));
const FString UGatherTextFromSourceCommandlet::FMacroDescriptor::TextMacroString(TEXT("TEXT"));
const FString UGatherTextFromSourceCommandlet::ChangelistName(TEXT("Update Localization"));

int32 UGatherTextFromSourceCommandlet::Main( const FString& Params )
{
	// Parse command line - we're interested in the param vals
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParamVals);

	//Set config file
	const FString* ParamVal = ParamVals.Find(FString(TEXT("Config")));
	FString GatherTextConfigPath;
	
	if ( ParamVal )
	{
		GatherTextConfigPath = *ParamVal;
	}
	else
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("No config specified."));
		return -1;
	}

	//Set config section
	ParamVal = ParamVals.Find(FString(TEXT("Section")));
	FString SectionName;

	if ( ParamVal )
	{
		SectionName = *ParamVal;
	}
	else
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("No config section specified."));
		return -1;
	}


	//Include paths
	TArray<FString> IncludePaths;
	GetConfigArray(*SectionName, TEXT("IncludePaths"), IncludePaths, GatherTextConfigPath);

	if (IncludePaths.Num() == 0)
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("No include paths in section %s"), *SectionName);
		return -1;
	}

	//Exclude paths
	TArray<FString> ExcludePaths;
	GetConfigArray(*SectionName, TEXT("ExcludePaths"), ExcludePaths, GatherTextConfigPath);

	// Check the config section for source filters. e.g. *.cpp
	TArray<FString> SourceFileSearchFilters;
	GetConfigArray(*SectionName, TEXT("SourceFileSearchFilters"), SourceFileSearchFilters, GatherTextConfigPath);

	if (SourceFileSearchFilters.Num() == 0)
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("No source filters in section %s"), *SectionName);
		return -1;
	}

	//Ensure all filters are unique.
	TArray<FString> UniqueSourceFileSearchFilters;
	for (int32 Idx=0; Idx<SourceFileSearchFilters.Num(); Idx++)
	{
		UniqueSourceFileSearchFilters.AddUnique(SourceFileSearchFilters[Idx]);
	}

	// Search in the root folder for each of the wildcard filters specified and build a list of files
	TArray<FString> AllFoundFiles;

	for(int32 i = 0; i < IncludePaths.Num(); i++)
	{
		for (int32 SourceFileSearchIdx=0; SourceFileSearchIdx < UniqueSourceFileSearchFilters.Num(); SourceFileSearchIdx++)
		{
			TArray<FString> RootSourceFiles;

			IFileManager::Get().FindFilesRecursive(RootSourceFiles, *(FPaths::RootDir() + IncludePaths[i]), *UniqueSourceFileSearchFilters[SourceFileSearchIdx], true, false,false);

			AllFoundFiles.Append(RootSourceFiles);
		}
	}

	TArray<FString> FilesToProcess;
	TArray<FString> RemovedList;

	//Run through all the files found and add any that pass the include, exclude and filter constraints to PackageFilesToProcess
	for( int32 FileIdx=0; FileIdx < AllFoundFiles.Num(); ++FileIdx )
	{
		bool bExclude = false;

		//Ensure it does not match the exclude paths if there are some.
		for( int32 ExcludePathIdx=0; ExcludePathIdx < ExcludePaths.Num() ; ++ExcludePathIdx )
		{
			if( AllFoundFiles[FileIdx].MatchesWildcard( ExcludePaths[ExcludePathIdx] ) )
			{
				bExclude = true;
				RemovedList.Add( AllFoundFiles[FileIdx] );
				break;
			}
		}

		//If we haven't failed any checks, add it to the array of files to process.
		if( !bExclude )
		{
			FilesToProcess.Add( AllFoundFiles[FileIdx] );
		}
	}
	
	// Return if no source files were found
	if( FilesToProcess.Num() == 0 )
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("The GatherTextFromSource commandlet couldn't find any source files in the specified directories."));
		return -1;
	}

	// Add any manifest dependencies if they were provided
	TArray<FString> ManifestDependenciesList;
	GetConfigArray(*SectionName, TEXT("ManifestDependencies"), ManifestDependenciesList, GatherTextConfigPath);
	
	if( !ManifestInfo->AddManifestDependencies( ManifestDependenciesList ) )
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("The GatherTextFromSource commandlet couldn't find all the specified manifest dependencies."));
		return -1;
	}

	// Get the loc macros and their syntax
	TArray<FParsableDescriptor*> Parsables;

	Parsables.Add(new FDefineDescriptor());

	Parsables.Add(new FUndefDescriptor());

	Parsables.Add(new FCommandMacroDescriptor());

	// New Localization System with Namespace as literal argument.
	Parsables.Add(new FStringMacroDescriptor( FString(TEXT("NSLOCTEXT")),
		FMacroDescriptor::FMacroArg(FMacroDescriptor::MAS_Namespace, true),
		FMacroDescriptor::FMacroArg(FMacroDescriptor::MAS_Identifier, true),
		FMacroDescriptor::FMacroArg(FMacroDescriptor::MAS_SourceText, true)));
	
	// New Localization System with Namespace as preprocessor define.
	Parsables.Add(new FStringMacroDescriptor( FString(TEXT("LOCTEXT")),
		FMacroDescriptor::FMacroArg(FMacroDescriptor::MAS_Identifier, true),
		FMacroDescriptor::FMacroArg(FMacroDescriptor::MAS_SourceText, true)));

	Parsables.Add(new FIniNamespaceDescriptor());

	// Init a parse context to track the state of the file parsing 
	FSourceFileParseContext ParseCtxt;
	ParseCtxt.ManifestInfo = ManifestInfo;

	// Parse all source files for macros and add entries to SourceParsedEntries
	for (int32 SourceFileIdx=0; SourceFileIdx < FilesToProcess.Num(); SourceFileIdx++)
	{
		ParseCtxt.Filename = FilesToProcess[SourceFileIdx];
		ParseCtxt.LineNumber = 0;
		ParseCtxt.ExcludedRegion = false;
		ParseCtxt.WithinBlockComment = false;
		ParseCtxt.WithinLineComment = false;

		FString SourceFileText;
		if (!FFileHelper::LoadFileToString(SourceFileText, *ParseCtxt.Filename))
		{
			UE_LOG(LogGatherTextFromSourceCommandlet, Error, TEXT("GatherTextSource failed to open file %s"), *ParseCtxt.Filename);
		}
		else
		{
			if (!ParseSourceText(SourceFileText, Parsables, ParseCtxt))
			{
				UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("GatherTextSource error(s) parsing source file %s"), *ParseCtxt.Filename);
			}
		}
	}
	
	// Clear parsables list safely
	for (int32 i=0; i<Parsables.Num(); i++)
	{
		delete Parsables[i];
	}

	return 0;
}

FString UGatherTextFromSourceCommandlet::RemoveStringFromTextMacro(const FString& TextMacro, const FString& IdentForLogging, bool& Error)
{
	FString Text;
	Error = true;

	// need to strip text literal out of TextMacro ( format should be TEXT("stringvalue") ) 
	if (!TextMacro.StartsWith(FMacroDescriptor::TextMacroString))
	{
		Error = false;
		//UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Missing TEXT macro in %s"), *IdentForLogging);
		return TextMacro.TrimQuotes();
	}
	else
	{
		int32 OpenQuoteIdx = TextMacro.Find(TEXT("\""), ESearchCase::CaseSensitive);
		if (0 > OpenQuoteIdx || TextMacro.Len() - 1 == OpenQuoteIdx)
		{
			UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Missing quotes in %s"), *MungeLogOutput(IdentForLogging));
		}
		else
		{
			int32 CloseQuoteIdx = TextMacro.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenQuoteIdx+1);
			if (0 > CloseQuoteIdx)
			{
				UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Missing quotes in %s"), *MungeLogOutput(IdentForLogging));
			}
			else
			{
				Text = TextMacro.Mid(OpenQuoteIdx + 1, CloseQuoteIdx - OpenQuoteIdx - 1);
				Error = false;
			}
		}
	}
	return Text;
}

bool UGatherTextFromSourceCommandlet::ParseSourceText(const FString& Text, const TArray<UGatherTextFromSourceCommandlet::FParsableDescriptor*>& Parsables, FSourceFileParseContext& ParseCtxt)
{
	// Create array of ints, one for each parsable we're looking for.
	TArray<int32> ParsableMatchCounters;
	ParsableMatchCounters.AddZeroed(Parsables.Num());

	// Cache array of tokens
	TArray<FString> ParsableTokens;
	for (int32 ParIdx=0; ParIdx<Parsables.Num(); ParIdx++)
	{
		ParsableTokens.Add(Parsables[ParIdx]->GetToken());
	}

	// Split the file into lines of 
	TArray<FString> TextLines;
	Text.ParseIntoArray(&TextLines, LINE_TERMINATOR, false);

	// Move through the text lines looking for the tokens that denote the items in the Parsables list
	for (int32 LineIdx = 0; LineIdx < TextLines.Num(); LineIdx++)
	{
		const FString& Line = TextLines[LineIdx].TrimTrailing();
		if( Line.IsEmpty() )
			continue;

		// Use these pending vars to defer parsing a token hit until longer tokens can't hit too
		int32 PendingParseIdx = -1;
		const TCHAR* ParsePoint = NULL;
		ParsableMatchCounters.Empty(Parsables.Num());
		ParsableMatchCounters.AddZeroed(Parsables.Num());
		ParseCtxt.LineNumber = LineIdx + 1;
		ParseCtxt.LineText = Line;
		ParseCtxt.EndParsingCurrentLine = false;

		const TCHAR* Cursor = *Line;
		bool EndOfLine = false;
		while (!EndOfLine && !ParseCtxt.EndParsingCurrentLine)
		{
			// Check if we're starting comments. Begins *at* "//" or "/*".
			if(!ParseCtxt.WithinLineComment && !ParseCtxt.WithinBlockComment)
			{
				const TCHAR* ForwardCursor = Cursor;
				if(*ForwardCursor == TEXT('/'))
				{
					++ForwardCursor;
					switch(*ForwardCursor)
					{
					case TEXT('/'):
						{
							ParseCtxt.WithinLineComment = true;
						}
						break;
					case TEXT('*'):
						{
							ParseCtxt.WithinBlockComment = true;
						}
						break;
					}
				}
			}

			// Check if we're ending comments. Ends *after* "*/".
			if(ParseCtxt.WithinBlockComment)
			{
				const TCHAR* ReverseCursor = Cursor;
				if(*ReverseCursor == TEXT('/') && ReverseCursor >= *Line)
				{
					--ReverseCursor;
					if(*ReverseCursor == TEXT('*')  && ReverseCursor >= *Line)
					{
						ParseCtxt.WithinBlockComment = false;
					}
				}
			}

			for (int32 ParIdx=0; ParIdx<Parsables.Num(); ParIdx++)
			{
				FString& Token = ParsableTokens[ParIdx];

				if (*Cursor == Token[ParsableMatchCounters[ParIdx]])
				{
					// Char at cursor matches the next char in the parsable's identifying token
					if (Token.Len() == ++(ParsableMatchCounters[ParIdx]))
					{
						// don't immediately parse - this parsable has seen its entire token but a longer one could be about to hit too
						const TCHAR* TokenStart = Cursor + 1 - Token.Len();
						if (0 > PendingParseIdx || ParsePoint >= TokenStart)
						{
							PendingParseIdx = ParIdx;
							ParsePoint = TokenStart;
						}
					}
				}
				else
				{
					// Char at cursor doesn't match the next char in the parsable's identifying token
					// Reset the counter to start of the token
					ParsableMatchCounters[ParIdx] = 0;
				}
			}

			// Now check PendingParse and only run it if there are no better candidates
			if (0 <= PendingParseIdx)
			{
				bool MustDefer = false; // pending will be deferred if another parsable has a equal and greater number of matched chars
				if( !Parsables[PendingParseIdx]->OverridesLongerTokens() )
				{
					for (int32 ParIdx=0; ParIdx<Parsables.Num(); ParIdx++)
					{
						if (PendingParseIdx != ParIdx)
						{
							if (ParsableMatchCounters[ParIdx] >= ParsableTokens[PendingParseIdx].Len())
							{
								// a longer token is matching so defer
								MustDefer = true;
							}
						}
					}
				}

				if (!MustDefer)
				{
					// Do the parse now
					Parsables[PendingParseIdx]->TryParse(FString(ParsePoint), ParseCtxt);
					ParsableMatchCounters[PendingParseIdx] = 0;
					PendingParseIdx = -1;
					ParsePoint = NULL;
				}
			}

			EndOfLine = ('\0' == *(++Cursor)) ? true : false;
			if(EndOfLine)
			{
				ParseCtxt.WithinLineComment = false;
			}
		}
	}

	return true;
}

bool UGatherTextFromSourceCommandlet::FSourceFileParseContext::AddManifestText( const FString& Token, const FString& InNamespace, const FString& SourceText, const FContext& Context )
{
	FString EntryDescription = FString::Printf( TEXT("In %s macro at %s - line %d:%s"),
		*Token,
		*Filename, 
		LineNumber, 
		*LineText);
	FLocItem Source( SourceText );
	return ManifestInfo->AddEntry(EntryDescription, InNamespace, Source, Context);
}

void UGatherTextFromSourceCommandlet::FDefineDescriptor::TryParse(const FString& Text, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// #define <defname>
	//  or
	// #define <defname> <value>

	if (!Context.ExcludedRegion && (Context.Filename.EndsWith(TEXT(".inl")) || (!Context.WithinBlockComment && !Context.WithinLineComment)) )
	{
		FString RemainingText = Text.RightChop(GetToken().Len()).Trim();

		if (LocDefRegionString == RemainingText)
		{
			// #define LOC_DEFINE_REGION
			Context.ExcludedRegion = true;
			Context.EndParsingCurrentLine = true;
		}

		FString FoundNamespaceString;

		if (RemainingText.StartsWith(LocNamespaceString, ESearchCase::CaseSensitive))
		{
			// #define LOCTEXT_NAMESPACE <namespace>
			FoundNamespaceString = LocNamespaceString;
		}
		else if(RemainingText.StartsWith(LocNamespaceStringOld, ESearchCase::CaseSensitive))
		{
			// #define LOC_NAMESPACE <namespace>
			FoundNamespaceString = LocNamespaceStringOld;
		}

		if( !FoundNamespaceString.IsEmpty() )
		{
			RemainingText = RemainingText.RightChop(FoundNamespaceString.Len()).Trim();

			bool RemoveStringError;
			FString DefineDesc = FString::Printf(TEXT("%s define %s(%d):%s"), *FoundNamespaceString, *Context.Filename, Context.LineNumber, *Context.LineText);
			FString Namespace = RemoveStringFromTextMacro(RemainingText, DefineDesc, RemoveStringError);
			if (!RemoveStringError)
			{
				Context.Namespace = Namespace;
				Context.EndParsingCurrentLine = true;
			}
		}
	}
}

void UGatherTextFromSourceCommandlet::FUndefDescriptor::TryParse(const FString& Text, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// #undef <defname>

	FString RemainingText = Text.RightChop(GetToken().Len()).Trim();

	if (Context.ExcludedRegion)
	{
		if (LocDefRegionString == RemainingText)
		{
			// #undef LOC_DEFINE_REGION
			Context.ExcludedRegion = false;
			Context.EndParsingCurrentLine = true;
		}
	}
	else
	{
		if( LocNamespaceString == RemainingText || LocNamespaceStringOld == RemainingText )
		{
			Context.EndParsingCurrentLine = true;
		}
	}
}

bool UGatherTextFromSourceCommandlet::FMacroDescriptor::ParseArgsFromMacro(const FString& Text, TArray<FString>& Args, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// NAME(param0, param1, param2, etc)

	bool Success = false;

	FString RemainingText = Text.RightChop(GetToken().Len()).Trim();
	int32 OpenBracketIdx = RemainingText.Find(TEXT("("));
	if (0 > OpenBracketIdx)
	{
		UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Missing bracket '(' in %s macro in %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *MungeLogOutput(Context.LineText));
		//Dont assume this is an error. It's more likely trying to parse something it shouldn't be.
		return false;
	}
	else
	{
		Args.Empty();

		bool bInDblQuotes = false;
		bool bInSglQuotes = false;
		int32 BracketStack = 1;
		bool bEscapeNextChar = false;

		const TCHAR* ArgStart = *RemainingText + OpenBracketIdx + 1;
		const TCHAR* Cursor = ArgStart;
		for (; 0 < BracketStack && '\0' != *Cursor; ++Cursor)
		{
			if (bEscapeNextChar)
			{
				bEscapeNextChar = false;
			}
			else if ((bInDblQuotes || bInSglQuotes) && !bEscapeNextChar && '\\' == *Cursor)
			{
				bEscapeNextChar = true;
			}
			else if (bInDblQuotes)
			{
				if ('\"' == *Cursor)
				{
					bInDblQuotes = false;
				}
			}
			else if (bInSglQuotes)
			{
				if ('\'' == *Cursor)
				{
					bInSglQuotes = false;
				}
			}
			else if ('\"' == *Cursor)
			{
				bInDblQuotes = true;
			}
			else if ('\'' == *Cursor)
			{
				bInSglQuotes = true;
			}
			else if ('(' == *Cursor)
			{
				++BracketStack;
			}
			else if (')' == *Cursor)
			{
				--BracketStack;

				if (0 > BracketStack)
				{
					UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Unexpected bracket ')' in %s macro in %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *MungeLogOutput(Context.LineText));
					return false;
				}
			}
			else if (1 == BracketStack && ',' == *Cursor)
			{
				// create argument from ArgStart to Cursor and set Start next char
				Args.Add(FString(Cursor - ArgStart, ArgStart));
				ArgStart = Cursor + 1;
			}
		}

		if (0 == BracketStack)
		{
			Args.Add(FString(Cursor - ArgStart - 1, ArgStart));
		}
		else
		{
			Args.Add(FString(ArgStart));
		}

		Success = 0 < Args.Num() ? true : false;	
	}

	return Success;
}

bool UGatherTextFromSourceCommandlet::FMacroDescriptor::PrepareArgument(FString& Argument, bool IsAutoText, const FString& IdentForLogging, bool& OutHasQuotes)
{
	bool Error = false;
	if (!IsAutoText)
	{
		Argument = RemoveStringFromTextMacro(Argument, IdentForLogging, Error);
		OutHasQuotes = Error ? false : true;
	}
	else
	{
		Argument = Argument.TrimTrailing().TrimQuotes(&OutHasQuotes);
	}
	return Error ? false : true;
}

void UGatherTextFromSourceCommandlet::FCommandMacroDescriptor::TryParse(const FString& Text, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// UI_COMMAND(LocKey, DefaultLangString, DefaultLangTooltipString, <IgnoredParam>, <IgnoredParam>)

	if (!Context.ExcludedRegion && (Context.Filename.EndsWith(TEXT(".inl")) || (!Context.WithinBlockComment && !Context.WithinLineComment)) )
	{
		TArray<FString> Arguments;
		if (ParseArgsFromMacro(Text, Arguments, Context))
		{
			if (Arguments.Num() != 5)
			{
				UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Too many arguments in command %s macro in %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *MungeLogOutput(Context.LineText));
			}
			else
			{
				FString Identifier = Arguments[0].Trim();
				FString Namespace = TEXT("UICommands");
				FString SourceLocation = FString( Context.Filename + TEXT(" - line ") + FString::FromInt(Context.LineNumber) );
				FString SourceText = Arguments[1].Trim();

				if ( Identifier.IsEmpty() )
				{
					//The command doesn't have an identifier so we can't gather it
					UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("UICOMMAND macro doesn't have unique identifier. %s"), *SourceLocation );
					return;
				}

				// parse DefaultLangString argument - this arg will be in quotes without TEXT macro
				bool HasQuotes;
				FString MacroDesc = FString::Printf(TEXT("\"FriendlyName\" argument in %s macro %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *Context.LineText);
				if ( PrepareArgument(SourceText, true, MacroDesc, HasQuotes) )
				{
					if ( HasQuotes && !Identifier.IsEmpty() && !SourceText.IsEmpty() )
					{
						// First create the command entry
						FContext CommandContext;
						CommandContext.Key = Identifier;
						CommandContext.SourceLocation = SourceLocation;

						Context.AddManifestText( GetToken(), Namespace, SourceText, CommandContext );

						// parse DefaultLangTooltipString argument - this arg will be in quotes without TEXT macro
						FString TooltipSourceText = Arguments[2].Trim();
						MacroDesc = FString::Printf(TEXT("\"InDescription\" argument in %s macro %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *Context.LineText);
						if (PrepareArgument(TooltipSourceText, true, MacroDesc, HasQuotes))
						{
							if (HasQuotes && !TooltipSourceText.IsEmpty())
							{
								// Create the tooltip entry
								FContext CommandTooltipContext;
								CommandTooltipContext.Key = Identifier + TEXT("_ToolTip");
								CommandTooltipContext.SourceLocation = SourceLocation;

								Context.AddManifestText( GetToken(), Namespace, TooltipSourceText, CommandTooltipContext );
							}
						}
					}
				}
			}
		}
	}
}

void UGatherTextFromSourceCommandlet::FStringMacroDescriptor::TryParse(const FString& Text, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// MACRONAME(param0, param1, param2)


	if (!Context.ExcludedRegion && (Context.Filename.EndsWith(TEXT(".inl")) || (!Context.WithinBlockComment && !Context.WithinLineComment)) )
	{
		TArray<FString> ArgArray;
		if (ParseArgsFromMacro(Text, ArgArray, Context))
		{
			int32 NumArgs = ArgArray.Num();

			if (NumArgs != Arguments.Num())
			{
				UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Too many arguments in %s macro in %s(%d):%s"), *GetToken(), *Context.Filename, Context.LineNumber, *MungeLogOutput(Context.LineText));
			}
			else
			{
				FString Identifier;
				FString Namespace = Context.Namespace;
				FString SourceLocation = FString( Context.Filename + TEXT(" - line ") + FString::FromInt(Context.LineNumber) );
				FString SourceText;

				bool ArgParseError = false;
				for (int32 ArgIdx=0; ArgIdx<Arguments.Num(); ArgIdx++)
				{
					FMacroArg Arg = Arguments[ArgIdx];
					FString ArgText = ArgArray[ArgIdx].Trim();

					bool HasQuotes;
					FString MacroDesc = FString::Printf(TEXT("argument %d of %d in localization macro %s %s(%d):%s"), ArgIdx+1, Arguments.Num(), *GetToken(), *Context.Filename, Context.LineNumber, *MungeLogOutput(Context.LineText));
					if (!PrepareArgument(ArgText, Arg.IsAutoText, MacroDesc, HasQuotes))
					{
						ArgParseError = true;
						break;
					}

					switch (Arg.Semantic)
					{
					case MAS_Namespace:
						{
							Namespace = ArgText;
						}
						break;
					case MAS_Identifier:
						{
							Identifier = ArgText;
						}
						break;
					case MAS_SourceText:
						{
							SourceText = ArgText;
						}
						break;
					}
				}

				if ( Identifier.IsEmpty() )
				{
					//The command doesn't have an identifier so we can't gather it
					UE_LOG(LogGatherTextFromSourceCommandlet, Warning, TEXT("Macro doesn't have unique identifier. %s"), *SourceLocation );
					return;
				}

				if (!ArgParseError && !Identifier.IsEmpty() && !SourceText.IsEmpty())
				{
					FContext MacroContext;
					MacroContext.Key = Identifier;
					MacroContext.SourceLocation = SourceLocation;

					Context.AddManifestText( GetToken(), Namespace, SourceText, MacroContext );
				}
			}
		}
	}
}

void UGatherTextFromSourceCommandlet::FIniNamespaceDescriptor::TryParse(const FString& Text, FSourceFileParseContext& Context) const
{
	// Attempt to parse something of the format
	// [<config section name>]
	if (!Context.ExcludedRegion)
	{
		if( FCString::Stricmp( *FPaths::GetExtension( Context.Filename, false ), TEXT("ini") ) == 0 &&
			Context.LineText[ 0 ] == '[' )
		{
			int32 ClosingBracket;
			if( Text.FindChar( ']', ClosingBracket ) && ClosingBracket > 1 )
			{
				Context.Namespace = Text.Mid( 1, ClosingBracket - 1 );
				Context.EndParsingCurrentLine = true;
			}
		}
	}
}

#undef LOC_DEFINE_REGION