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
#ifndef __VCommandLineParserOption__
#define __VCommandLineParserOption__

#include "Kernel/VKernel.h"
#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VString.h"
#include <vector>
#include "VCommandLineParserExtension.h"



BEGIN_TOOLBOX_NAMESPACE

namespace Private
{
namespace CommandLineParser
{


enum // anonymous
{
	/*!
	** \brief Flag to determine whether CommandLineParser should handle windows style of cmd arguments
	**
	** Example of Windows style arguments: /myoption [long option], /y [short option]
	*/
	#if VERSIONWIN
	eWindowsStyleEnabled =  1,
	#else
	eWindowsStyleEnabled =  0,
	#endif
};


//! Error for command line parsing
enum Error
{
	//! The parse is complete
	eOK,
	//! The parse has failed (invalid parameter, unknown argument, ...)
	eFailed,
	//! The parse is complete but the program should quit
	//  (the help usage has been printed, or the version or something else...)
	eShouldExit
};





/*!
** \brief Abstract representation of a single Command line Option
*/
class XTOOLBOX_API VOption /*: public VObject*/
{
public:
	//! Array of options
	typedef std::vector<VOption*> Vector;
	//! Option + flag to determine whether an additional value is required or not
	typedef std::pair<VOption*, bool> OptionAndValueRequired;

	// \warning std::hash_map is depprecated and should be replaced as soon
	// as possible by std::unordered_map
	//! List of options, ordered by their short names
	typedef STL_EXT_NAMESPACE::hash_map<UniChar, OptionAndValueRequired>  MapByShortNames;
	//! List of options, ordered by their long names
	// \note using here the COW feature of the VString
	typedef STL_EXT_NAMESPACE::hash_map<VString, OptionAndValueRequired>  MapByLongNames;


public:
	//! \name Constructor & Destructor
	//@{
	//! Default empty constructor
	VOption();
	//! Constructor, with only a description
	explicit VOption(const VString& inDesc);
	/*!
	** \brief Default constructor
	**
	** \param inShortName Short name used for calling the option
	** \param inLongName  Long name used for calling the option
	** \param inDesc  Description of the option [mandatory]
	** \param inRequireParam Flag to determine whether this option requires an additional value or not
	*/
	VOption(char inShortName, const VString& inLongName, const VString& inDesc, bool inRequireParam);
	//! Empty destructor
	virtual ~VOption() {}
	//@}

	//! AppendArgument a new value
	virtual Error AppendArgument(const VString& value);
	//! Enable the flag
	virtual void SetFlag() {}

	//! Get the short name of the option
	UniChar GetShortName() const;
	//! Get the long name of the option
	const VString& GetLongName() const;
	//! Get the description of the option
	const VString& GetDescription() const;
	//! Get if the option requires an additional parameter
	bool IsRequiresParameter() const;

	//! Get if the option if a paragraphe and not a real option
	bool IsParagraph() const;

	//! Append the full name of the option (e.g. -e, --long-name)
	void AppendFullName(VString& out) const;

protected:
	//! Print an error when appending an invalide value to an option
	// \see AppendArgument()
	void PrintErrorWhenAppendingInvalidValue(const VString& inExpected, VString inValue);

private:
	//! The short name
	UniChar fShortName;
	//! The long name
	VString fLongName;
	//! Description associated to the command line option
	VString fDescription;
	//! Flag to determine whether this option requires an additional value or not
	bool fRequiresParameter;

}; // class VOption




/*!
** \brief Internal storage for all command line options
*/
class XTOOLBOX_API StOptionsInfos
{
public:
	//! \name Constructor & Destructor
	//@{
	//! Default constructor
	StOptionsInfos(const std::vector<VString>& inArgs);
		//! Destructor
	~StOptionsInfos();
	//@}

	//! Clear all internal data
	void Clear();

	//! Reset the option dedicated to remaining arguments
	void ResetRemainingArguments(Private::CommandLineParser::VOption* inOption);

	//! AppendArgument a new option
	void RegisterOption(bool requiresValue, Private::CommandLineParser::VOption* inOption);

	//! Print the help usage to standard output
	void HelpUsage(const VString& inArgv0) const;

public:
	//! All options, ordered by their long name
	Private::CommandLineParser::VOption::MapByShortNames  byShortNames;
	//! All options, ordered by their long name
	Private::CommandLineParser::VOption::MapByLongNames   byLongNames;
	//! The remaining option (the pointer belongs to us)
	Private::CommandLineParser::VOption* remainingOption;
	//! All options
	Private::CommandLineParser::VOption::Vector options;
	//! All arguments
	const std::vector<VString>& args;
	//! Flag to determine whether options may be required or not
	bool mayRequireOptions;
	//! Flag to determine whether extra informations (such as description or paragraphs)
	// may be required or not
	bool mayRequireDocumentation;

}; // class StOptionsInfos




/*!
** \brief Generic reprensentation of a single option with value and its associated variable
*/
template<class T>
class VValue /*final*/ : public VOption
{
public:
	/*!
	** \brief Constructor with only a reference to the target variable [remaining arguments]
	*/
	explicit VValue(T& out);

	/*!
	** \brief Constructor
	**
	** \param inShortName Short name used for calling the option
	** \param inLongName  Long name used for calling the option
	** \param inDesc  Description of the option [mandatory]
	** \param inRequireParam Flag to determine whether this option requires an additional value or not
	*/
	VValue(T& out, char inShortName, const VString& inLongName, const VString& inDesc, bool inRequireParam = false);

	virtual Error AppendArgument(const VString& inValue); // override
	virtual void SetFlag(); // override

public:
	//! Reference to the target
	T& fRef;

}; // class VValue










inline void StOptionsInfos::ResetRemainingArguments(Private::CommandLineParser::VOption* inOption)
{
	delete remainingOption;
	remainingOption = inOption;
}





inline VOption::VOption()
	: fShortName(' ')
{}


inline VOption::VOption(const VString& inDesc)
	: fShortName(' ')
	, fDescription(inDesc)
{}


inline VOption::VOption(char inShortName, const VString& inLongName, const VString& inDesc, bool inRequireParam)
	: fShortName(inShortName)
	, fLongName(inLongName)
	, fDescription(inDesc)
	, fRequiresParameter(inRequireParam)
{}


inline Error VOption::AppendArgument(const VString& /*value*/)
{
	return eFailed;
}


inline bool VOption::IsRequiresParameter() const
{
	return fRequiresParameter;
}


inline UniChar VOption::GetShortName() const
{
	return fShortName;
}


inline const VString& VOption::GetLongName() const
{
	return fLongName;
}


inline const VString& VOption::GetDescription() const
{
	return fDescription;
}


inline bool VOption::IsParagraph() const
{
	return (fShortName == ' ' && fLongName.GetLength() == 0);
}




template<class T>
inline VValue<T>::VValue(T& out)
	: VOption()
	, fRef(out)
{}


template<class T>
inline VValue<T>::VValue(T& out, char inShortName, const VString& inLongName, const VString& inDesc, bool inRequireParam)
	: VOption(inShortName, inLongName, inDesc, inRequireParam)
	, fRef(out)
{}


template<class T>
inline Error VValue<T>::AppendArgument(const VString& inValue)
{
	// AppendArgument the value to our inner variable according its type
	if (! Extension::CommandLineParser::template Argument<T>::Append(fRef, inValue))
	{
		VString expected;
		Extension::CommandLineParser::template Argument<T>::ExpectedValues(expected);
		PrintErrorWhenAppendingInvalidValue(expected, inValue);
		return eFailed;
	}
	return eOK;
}


template<class T>
inline void VValue<T>::SetFlag()
{
	// AppendArgument the value to our inner variable according its type
	Extension::CommandLineParser::template Argument<T>::SetFlag(fRef);
}






} // namespace CommandLineParser
} // namespace Private
END_TOOLBOX_NAMESPACE

#endif // __VCommandLineParserOption__
