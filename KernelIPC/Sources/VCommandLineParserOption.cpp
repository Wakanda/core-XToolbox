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
#include "VCommandLineParserOption.h"
#include <iostream>


#define HELPUSAGE_30CHAR    CVSTR("                             ") // end



BEGIN_TOOLBOX_NAMESPACE

namespace Private
{
namespace CommandLineParser
{

	enum
	{
		//! The maximum number of char per line for the help usage
		helpMaxWidth = 80,
	};




	#if WITH_ASSERT
	//! Determine whether a string is a valid long option name or not
	static inline bool DebugValidateOptionLongName(const VString& inName)
	{
		if (inName.GetLength() > 0)
		{
			// a long option name must be greater than 1, otherwise it will be
			// a short one, and won't be found at runtime
			if (inName.GetLength() == 1)
				return false;

			// checking validity for each char
			for (VIndex i = 0; i < inName.GetLength(); ++i)
			{
				UniChar c = inName[i];
				if (! (c == '-' || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
					return false;
			}

			// checking invalid start / end
			if (inName[0] == L'-' || inName[inName.GetLength() - 1] == L'-')
				return false;
		}
		return true;
	}
	#endif


	#if WITH_ASSERT
	//! Determine whether a single char is a valid short option name or not
	static inline bool DebugValidateOptionShortName(UniChar c)
	{
		// for simplicity sake, a short option name should be restricted
		// to mere ascii characters
		return (c == '\0' || c == ' ') // no short option
			|| (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9');
	}
	#endif



	void VOption::AppendFullName(VString& out) const
	{
		bool hasLongOption = (fLongName.GetLength() > 0);

		if (fShortName != UniChar(' ') && fShortName != UniChar('\0'))
		{
			out += UniChar('-');
			out += fShortName;
			if (hasLongOption)
				out += CVSTR(", ");
		}
		if (hasLongOption)
		{
			out += CVSTR("--");
			out += fLongName;
		}
	}


	void VOption::PrintErrorWhenAppendingInvalidValue(const VString& inExpected, VString inValue)
	{
		VString msg;
		msg += CVSTR("invalid value for argument '");

		// full name of the option (e.g. -e, --long-name)
		AppendFullName(msg);

		// got "<value>..."
		msg += CVSTR("' got ");

		if (inValue.GetLength() > 0)
		{
			msg += UniChar('"');
			if (inValue.GetLength() > 20) // arbitrary value
			{
				inValue.SubString(1, 20);
				msg += inValue;
				msg += CVSTR("...\"");
			}
			else
			{
				msg += inValue;
				msg += UniChar('"');
			}
			msg += UniChar(' ');
		}
		else
			msg += CVSTR("empty value ");

		// Expected value
		msg += inExpected;

		XBOX::VCommandLineParser::PrintArgumentError(msg);
	}






	StOptionsInfos::StOptionsInfos(const std::vector<VString>& inArgs)
		: remainingOption(NULL)
		, args(inArgs)
		, mayRequireOptions((inArgs.size() > 1u))
		, mayRequireDocumentation(false)
	{
		if (mayRequireOptions)
		{
			// detecting if we should keep extra informations
			for (uLONG i = 1; i < (uLONG) args.size(); ++i)
			{
				const VString& arg = args[i];

				if (arg == CVSTR("--help") || arg == CVSTR("-h"))
				{
					mayRequireDocumentation = true;
					break;
				}

				if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
				{
					if (arg == CVSTR("/?") || arg == CVSTR("/help"))
					{
						mayRequireDocumentation = true;
						break;
					}
				}
			}
		}
	}


	StOptionsInfos::~StOptionsInfos()
	{
		// delete all options
		for (uLONG i = 0; i != (uLONG) options.size(); ++i)
			delete options[i];

		delete remainingOption;
	}




	void StOptionsInfos::Clear()
	{
		delete remainingOption; // freeAndNil()
		remainingOption = NULL;

		// using clear-and-minimize idiom to really release all the allocated memory
		using namespace Private::CommandLineParser;
		VOption::MapByShortNames().swap(byShortNames);
		VOption::MapByLongNames().swap(byLongNames);
		VOption::Vector().swap(options);
	}



	void StOptionsInfos::RegisterOption(bool requiresValue, Private::CommandLineParser::VOption* inOption)
	{
		// asserts misc
		xbox_assert(inOption
			&& "invalid pointer to VOption");
		xbox_assert(mayRequireOptions
			&& "This routine should not be called if no option are required");

		// alias to the option itself
		const Private::CommandLineParser::VOption& option = *inOption;

		// alias to the short option name
		UniChar c = option.GetShortName();
		// alias to the long option name
		const VString& longname = option.GetLongName();

		// asserts on long option names
		xbox_assert(longname.GetLength() > 1
			&& "the long option name should not looks like a short one (won't be found at runtime)");
		xbox_assert(longname != CVSTR("help")
			&& "the long option name 'help' is reserved");
		xbox_assert(DebugValidateOptionLongName(longname)
			&& "invalid long name for command line option");

		// asserts on short option names
		xbox_assert(c != UniChar('?')
			&& "the short option name '?' is reserved");
		xbox_assert(c != UniChar('h')
			&& "the short option name 'h' is reserved");
		xbox_assert(DebugValidateOptionShortName(c)
			&& "invalid short name for command line option");

		// asserts on description
		xbox_assert(((c != UniChar(' ') && c != '\0') || (longname.GetLength() > 0))
			&& option.GetDescription().GetLength() > 0
			&& "no description provided for command line option");


		// Short name registration
		if (c != UniChar(' ') && c != UniChar('\0'))
			byShortNames[c] = std::make_pair(inOption, requiresValue);

		// Long name registration
		if (longname.GetLength() > 0)
			byLongNames[longname] = std::make_pair(inOption, requiresValue);

		// appending the option after all others
		options.push_back(inOption);
	}


	static uLONG StringFindFirstOf(const VString& in, const VString& inCharsToFind, uLONG inOffset)
	{
		uLONG size = (uLONG) in.GetLength();
		for (uLONG i = inOffset; i < size; ++i)
		{
			if (0 != inCharsToFind.FindUniChar(in[i]))
				return i;
		}
		return (uLONG) -1;
	}


	template<bool Decal, uLONG LengthLimit>
	static void ExportLongDescription(const VString& inDesc, VString* out)
	{
		uLONG size = inDesc.GetLength();

		if (size > 0)
		{
			const UniChar* cstr = inDesc.GetCPointer();
			// reserve enough room
			out->EnsureSize(out->GetLength() + size + 4); // arbitrary

			uLONG start  = 0;
			uLONG end    = 0;
			uLONG offset = 0;

			do
			{
				// Jump to the next separator
				offset = StringFindFirstOf(inDesc, CVSTR(" .\r\n\t"), offset);

				// No separator, aborting
				if (size < offset)
					break;

				if (offset - start < LengthLimit)
				{
					if ('\n' == inDesc[offset])
					{
						out->AppendUniChars(cstr + start, (offset - start));
						*out += '\n';

						if (Decal)
							*out += HELPUSAGE_30CHAR;

						start = offset + 1;
						end = offset + 1;
					}
					else
						end = offset;
				}
				else
				{
					if (0 == end)
						end = offset;

					out->AppendUniChars(cstr + start, (end - start));
					*out += '\n';

					if (Decal)
						*out += HELPUSAGE_30CHAR;

					start = end + 1;
					end = offset + 1;
				}

				++offset;
			}
			while (true);

			// Display the remaining piece of string
			if (start < size)
				*out += (cstr + start);
		}
	}


	/*!
	** \brief Generate the help usage for a single option
	*/
	static inline
	void ExportOptionHelpUsage(UniChar inShort, const VString& inLong, const VString& inDesc, bool requireParam, VString* out)
	{
		if ('\0' != inShort && ' ' != inShort)
		{
			*out += CVSTR("  -");
			*out += inShort;

			if (0 == inLong.GetLength())
			{
				if (requireParam)
					*out += CVSTR(" VALUE");
			}
			else
				*out += CVSTR(", ");
		}
		else
			*out += CVSTR("      ");

		// Long name
		if (0 == inLong.GetLength())
		{
			if (requireParam)
				*out += CVSTR("              ");
			else
				*out += CVSTR("                    ");
		}
		else
		{
			*out += CVSTR("--");
			*out += inLong;

			if (requireParam)
				*out += CVSTR("=VALUE");

			if (30 <= inLong.GetLength() + 6 /*-o,*/ + 2 /*--*/ + 1 /*space*/ + (requireParam ? 6 : 0))
			{
				*out += CVSTR("\n                             ");
			}
			else
			{
				for (uLONG i = 6 + 2 + 1 + (uLONG) inLong.GetLength() + (requireParam ? 6 : 0); i < 30; ++i)
					*out += ' ';
			}
		}

		// Description
		if (inDesc.GetLength() <= 50 /* 80 - 30 */)
			*out += inDesc;
		else
			ExportLongDescription<true, 50>(inDesc, out);

		*out += '\n';
	}


	static void ExportParagraph(const VString& inDesc, VString* out)
	{
		uLONG size = inDesc.GetLength();
		if (size > 0)
		{
			if (size < helpMaxWidth)
				*out += inDesc;
			else
				ExportLongDescription<false, 80>(inDesc, out);

			// appending a final linefeed
			*out += '\n';
		}
	}


	void StOptionsInfos::HelpUsage(const VString& inArgv0) const
	{
		VString help;

		// first line
		help += CVSTR("Usage: ");
		help += inArgv0; // FIXME : extractFilePath(inArgv0)
		help += CVSTR(" [OPTION]...");
		if (remainingOption)
			help += CVSTR(" [FILE]...");
		help += '\n';

		// iterating through all options
		if (!options.empty())
		{
			Private::CommandLineParser::VOption::Vector::const_iterator end = options.end();
			Private::CommandLineParser::VOption::Vector::const_iterator i   = options.begin();

			// Add a space if the first option is not a paragraph
			// (otherwise the user would do what he wants)
			if (! (*i)->IsParagraph())
				help += '\n';

			for (; i != end; ++i)
			{
				const Private::CommandLineParser::VOption& opt = *(*i);

				if (!opt.IsParagraph())
				{
					ExportOptionHelpUsage(opt.GetShortName(), opt.GetLongName(),
						opt.GetDescription(), opt.IsRequiresParameter(), &help);
				}
				else
					ExportParagraph(opt.GetDescription(), &help);
			}
		}

		// determining if the --help option is implicit or not
		if (0 == byLongNames.count(CVSTR("help")))
		{
			// --help is implicit
			VString desc = CVSTR("Prints the help and quits");
			//
			if (byShortNames.count('h') == 0)
				ExportOptionHelpUsage('h', CVSTR("help"), desc, false, &help);
			else
				ExportOptionHelpUsage(' ', CVSTR("help"), desc, false, &help);
		}

		// display the help usage to the console
		# if VERSIONWIN
		std::wcout.write(help.GetCPointer(), help.GetLength());
		# else
		StStringConverter<char> buff(help, VTC_UTF_8);
		if (buff.IsOK())
			std::cout.write(buff.GetCPointer(), buff.GetSize());
		# endif
	}








} // namespace CommandLineParser
} // namespace Private
END_TOOLBOX_NAMESPACE

