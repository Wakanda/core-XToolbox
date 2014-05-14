/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#include "VKernelIPCPrecompiled.h"
#include "VCommandLineParser.h"
#include <iostream>







BEGIN_TOOLBOX_NAMESPACE


namespace // anonymous
{

	enum ArgType
	{
		//! The argument is a short option name (-a)
		ttShortOption,
		//! The argument is a long option name (--option)
		ttLongOption,
		//! The argument is a parameter (value)
		ttParameter,
		//! The argument is a windows style argument, and may short or long (/myoption, /y)
		ttBoth,
		//! The argument shall be ignored (most likely a double dash '--')
		ttIgnore,
	};


	//! Convenient alias to Error
	typedef XBOX::Private::CommandLineParser::Error Error;






	static Error RaiseUnknownOption(const VString& inName)
	{
		VString msg;
		msg += CVSTR("unknown option `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseInvalidValue(const VString& inName)
	{
		VString msg;
		msg += CVSTR("invalid value for option  `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseValueNotRequired(const VString& inName)
	{
		VString msg;
		msg += CVSTR("a value for option `");
		msg += inName;
		msg += CVSTR("` is not required\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseMissingValue(const Private::CommandLineParser::VOption& inOption)
	{
		VString msg = CVSTR("missing value for option ");
		inOption.AppendFullName(msg);
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}









	class ParserContext /*final*/
	{
	public:
		//! Default constructor
		ParserContext(Private::CommandLineParser::StOptionsInfos& inInfos);
		//! Get if we had encountered some errors (fatal)
		bool HasErrors() const;

		Error operator () ();


	private:
		//! Try to analyze a long option name (such as --key, --key=value or /key)
		Error AnalyzeLongOptionName(const VString& name);
		//! Try to analyze a long option name with an attached value to it
		// (such as --key=value)
		Error AnalyzeLongOptionNameWithAttachedValue(const VString& inName, VIndex sub);
		//! Try to analyze a long option name without any value attached to it
		// (such as --key or /key)
		Error AnalyzeLongOptionNameWithoutValue(const VString& inName);
		//! Try to analyze short option names (such as -aBc, where 'a', 'B' and 'c' might be options)
		Error AnalyzeShortOptionName(const VString& inArg);

		//! Try to find an argument typed as `parameter`
		bool findNextParameter(VString& out);
		//! Determine the type of a single argument according its first characters
		ArgType DetermineArgumentType(const VString& inArg);


	private:
		//! The current token index
		uLONG fTokenIndex;
		//! The last parameter index
		uLONG fParameterIndex;
		//! Temporary string for option name manipulation
		VString fTmp;

		// flag for forcing the parameter type for an argument,
		// espcially usefull if we encounter a double dash '--'
		bool fForceParameterType;
		//! All arguments passed from the command line
		const std::vector<VString>& fArgs;

		//! Encountered errors ?
		bool fHasError;

		//! Informations about all comman line options
		Private::CommandLineParser::StOptionsInfos& fInfos;

	}; // class ParserContext





	inline ParserContext::ParserContext(Private::CommandLineParser::StOptionsInfos& inOptions)
		: fTokenIndex(1u)
		, fParameterIndex(1u)
		, fForceParameterType(false)
		, fArgs(inOptions.args)
		, fHasError(false)
		, fInfos(inOptions)
	{
		xbox_assert(fArgs.size() > 1
			&& "No parsing should take place if there is nothing to parse");
	}


	inline ArgType ParserContext::DetermineArgumentType(const VString& inArg)
	{
		if (!fForceParameterType && inArg.GetLength() > 0) // ignore empty strings
		{
			// Trying UNIX like options
			// we may return here only '--' in some cases
			if (inArg[0] == UniChar('-'))
			{
				if (inArg[1] == UniChar('-'))
				{
					if (inArg.GetLength() == 2)
					{
						// just encountered '--', which is nothing and should force
						// to consider all other arguments as raw parameters
						fForceParameterType = true;
						return ttIgnore;
					}
					return ttLongOption;
				}
				return ttShortOption;
			}

			if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
			{
				// Trying Windows like options
				if (inArg[0] == UniChar('/'))
				{
					// Arguments with only a slash should be considered as a parameter
					// and not an option
					return (inArg[1] == UniChar('\0')) ? ttParameter : ttBoth;
				}
			}
		}

		// fallback, parameter of course
		return ttParameter;
	}



	bool ParserContext::findNextParameter(VString& out)
	{
		while (++fParameterIndex < (uLONG) fArgs.size())
		{
			if (ttParameter == DetermineArgumentType(fArgs[fParameterIndex]))
			{
				out = fArgs[fParameterIndex];
				// This argument must not be used again as a parameter
				++fParameterIndex;
				return true;
			}
		}
		return false;
	}



	Error  ParserContext::AnalyzeLongOptionNameWithAttachedValue(const VString& inName, VIndex inSub)
	{
		// the value has been directly provided
		fTmp.Clear();
		inName.GetSubString(1, inSub - 1, fTmp);

		Private::CommandLineParser::VOption::MapByLongNames::iterator i = fInfos.byLongNames.find(fTmp);
		if (i != fInfos.byLongNames.end())
		{
			// does this option requires an additional value ?
			bool requiresAdditionalValue = i->second.second;
			if (requiresAdditionalValue)
			{
				// reusing fTmp - extracting the value from the argument
				fTmp.Clear();
				inName.GetSubString(inSub + 1, inName.GetLength() - inSub, fTmp);

				// append / reset
				return i->second.first->AppendArgument(fTmp);
			}

			// this option does not require an additional value
			// since we have something like `--key=value`, this is obviously an error
			return RaiseValueNotRequired(fTmp);
		}

		// Checking if not a virtual standard value
		if (fTmp == CVSTR("help"))
			return RaiseInvalidValue(fTmp);

		// fallback - unknown option
		return RaiseUnknownOption(fTmp);
	}


	Error  ParserContext::AnalyzeLongOptionNameWithoutValue(const VString& inArg)
	{
		// No additional value has been provided yet
		Private::CommandLineParser::VOption::MapByLongNames::iterator i = fInfos.byLongNames.find(inArg);
		if (i != fInfos.byLongNames.end())
		{
			// does this option requires an additional value ?
			bool requiresAdditionalValue = i->second.second;

			if (!requiresAdditionalValue)
			{
				// This option is merely a flag
				i->second.first->SetFlag();
			}
			else
			{
				// this option requires an additional, which should be found somewhere else
				// in the argument list (reusing fTmp)
				if (!findNextParameter(fTmp))
					return RaiseMissingValue(*(i->second.first));

				if (! i->second.first->AppendArgument(fTmp))
					return Private::CommandLineParser::eFailed;
			}
			return Private::CommandLineParser::eOK;
		}

		// the option has not been found but this might be a `system` one
		if (inArg == L"help")
			return Private::CommandLineParser::eShouldExit;

		// definitevelty an error
		return RaiseUnknownOption(inArg);
	}


	inline Error  ParserContext::AnalyzeLongOptionName(const VString& inArg)
	{
		// Is an additional value already provided ?  --param=value
		VIndex sub = inArg.FindUniChar(L'=');

		if (Private::CommandLineParser::eWindowsStyleEnabled)
		{
			// the character '=' has not been found. However on Windows ':' could be used
			if (sub == 0)
				sub = inArg.FindUniChar(L':');
		}

		return (sub == 0)
			? AnalyzeLongOptionNameWithoutValue(inArg)
			: AnalyzeLongOptionNameWithAttachedValue(inArg, sub);
	}


	inline Error  ParserContext::AnalyzeShortOptionName(const VString& inArg)
	{
		// skip the first char to avoid string replacement, which can be either '-' or '/'
		for (int i = 1; i < inArg.GetLength(); ++i)
		{
			UniChar c = inArg[i];
			Private::CommandLineParser::VOption::MapByShortNames::iterator it = fInfos.byShortNames.find(c);
			if (it != fInfos.byShortNames.end())
			{
				// does this option requires an additional value ?
				bool requiresAdditionalValue = it->second.second;

				if (!requiresAdditionalValue)
				{
					// This option is merely a flag
					it->second.first->SetFlag();
				}
				else
				{
					// this option requires an additional, which should be found somewhere else
					// in the argument list (reusing fTmp)
					if (!findNextParameter(fTmp))
						return RaiseMissingValue(*(it->second.first));

					if (! it->second.first->AppendArgument(fTmp))
						return Private::CommandLineParser::eFailed;
				}
			}
			else
			{
				// the option has not been found but this might be a `system` one
				if (c == UniChar('?') || c == UniChar('h'))
					return Private::CommandLineParser::eShouldExit;

				// This short option name is unknown
				fTmp = c;
				return RaiseUnknownOption(fTmp);
			}
		}
		return Private::CommandLineParser::eOK;
	}




	Error  ParserContext::operator () ()
	{
		// temporary string for argument manipulation
		VString tmp;

		// iterate through all arguments
		while (fTokenIndex < (uLONG) fArgs.size())
		{
			// the Nth argument - the current one actually
			const VString& argN = fArgs[fTokenIndex];

			switch (DetermineArgumentType(argN))
			{
				case ttShortOption:
				{
					// The method `AnalyzeShortOptionName` will ignore the first char
					Error err = AnalyzeShortOptionName(argN);
					if (err != Private::CommandLineParser::eOK)
						return err;
					break;
				}

				case ttLongOption:
				{
					// the current argument seems to be a long option name like --key or --key=value

					// get rid of the two first char (--myoption)
					argN.GetSubString(2 + 1, argN.GetLength() - 2, tmp);
					if (tmp.GetLength() > 0)
					{
						Error err = AnalyzeLongOptionName(tmp);
						if (err != Private::CommandLineParser::eOK)
							return err;
					}
					else
					{
						// double dash - should consider all other arguments as
						// mere parameters, without option attached to it
						fForceParameterType = true;
					}
					break;
				}

				case ttParameter:
				{
					// the current argument is not attached to any option
					if (fTokenIndex >= fParameterIndex)
					{
						fParameterIndex = fTokenIndex;
						if (fInfos.remainingOption)
						{
							if (! fInfos.remainingOption->AppendArgument(argN))
								return Private::CommandLineParser::eFailed;
						}
						else
							return RaiseUnknownOption(argN);
					}
					break;
				}

				case ttBoth:
				{
					if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
					{
						// This special flag would only happen on Windows (/myoption, /y)
						switch (argN.GetLength())
						{
							case 1:
							{
								// should never happen since 'argN' should be considered
								// as a parameter, and not an option
								xbox_assert(false && "argN should be considered as a parameter");
								return RaiseInvalidValue(argN);
							}
							case 2:
							{
								// it seems to be a short option name
								// The method `AnalyzeShortOptionName` will ignore the first char
								Error err = AnalyzeShortOptionName(argN);
								if (err != Private::CommandLineParser::eOK)
									return err;
								break;
							}
							default:
							{
								// get rid of the two first char (/y /mylongoption)
								argN.GetSubString(1 + 1, argN.GetLength() - 1, tmp);
								Error err = AnalyzeLongOptionName(tmp);
								if (err != Private::CommandLineParser::eOK)
									return err;
							}
						} // switch argN
					} // windows style enabled

					break;
				}

				case ttIgnore:
				{
					// ignore the argument, as requested
					break;
				}

			} // switch argument type

			// go to the next argument
			++fTokenIndex;

		} // loop on all arguments

		return Private::CommandLineParser::eOK;
	}



	static inline Error CommandLineParserParse(Private::CommandLineParser::StOptionsInfos& inInfos)
	{
		ParserContext parser(inInfos);

		Error err = parser();
		if (err != Private::CommandLineParser::eOK) // no parse error - but should quit properly
		{
			xbox_assert(!inInfos.args.empty());
			if (!inInfos.args.empty()) // for code safety if not properly initialized
				inInfos.HelpUsage(inInfos.args[0]);
		}
		return err;
	}


} // anonymous namespace







VCommandLineParser::Error  VCommandLineParser::PrintArgumentError(const VString& inText)
{
	#if VERSIONWIN
	std::wcerr.write(inText.GetCPointer(), inText.GetLength());
	std::wcerr << std::endl;
	#else
	StStringConverter<char> buff(inText, VTC_UTF_8);
	if (buff.IsOK())
		std::cerr.write(buff.GetCPointer(), buff.GetSize());
	std::cerr << std::endl;
	#endif
	return eFailed;
}


void VCommandLineParser::Print(const VString& inText)
{
	#if VERSIONWIN
	std::wcout.write(inText.GetCPointer(), inText.GetLength());
	std::wcout << std::endl;
	#else
	StStringConverter<char> buff(inText, VTC_UTF_8);
	if (buff.IsOK())
		std::cout.write(buff.GetCPointer(), buff.GetSize());
	std::cout << std::endl;
	#endif
}


VCommandLineParser::VCommandLineParser(const std::vector<VString>& inArgs)
	: fOptions(inArgs)
{
	// nothing to do
}


VCommandLineParser::~VCommandLineParser()
{
	// nothing to do
}


void VCommandLineParser::Clear()
{
	fOptions.Clear();
}


VCommandLineParser::Error  VCommandLineParser::Parse()
{
	// remainder: the first argument is always the binary itself
	// \internal > 1 to take care of non-initialized arguments vector
	if (fOptions.args.size() > 1)
	{
		switch (CommandLineParserParse(fOptions))
		{
			case Private::CommandLineParser::eOK:
			{
				return eOK;
			}
			case Private::CommandLineParser::eShouldExit:
			{
				// should quit, but no error (help usage probably)
				return eShouldExit;
			}
			default:
			{
				// error for all other cases
				break;
			}
		}
	}
	else
	{
		// nothing to parse, no error
		return eOK;
	}

	// fallback - error if something is not handled properly
	return eFailed;
}




END_TOOLBOX_NAMESPACE

