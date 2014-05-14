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
#ifndef __XTOOLBOX_KERNELIPC_GETOPT_EXTENSION_HXX__
#define __XTOOLBOX_KERNELIPC_GETOPT_EXTENSION_HXX__

#include "Kernel/VKernel.h"
#include "Kernel/Sources/VString.h"




BEGIN_TOOLBOX_NAMESPACE

namespace Extension
{
namespace CommandLineParser
{


	/*!
	** \brief Traits for appending a string value to an arbitrary variable
	*/
	template<class T> struct Argument
	{
		// Does not compile on purpose (missing declaration / static_assert)
		// This would mean that the type has not been implemented
	};







	// GENERIC SPECIALIZATIONS

	//! Generic specialization for all pointers types
	template<class T>
	struct Argument<T*> // final
	{
		static inline bool Append(T*& out, const VString& inValue)
		{
			if (!out)
				out = new T();
			Argument<T>::Append(*out, inValue);
			return true;
		}
		static inline void SetFlag(T*& out)
		{
			if (!out)
				out = new T();
			Argument<T>::SetFlag(*out);
		}
		static inline void ExpectedValues(VString& out)
		{
			Argument<T>::ExpectedValues(out);
		}
	};

	//! Generic specialization for all ref-countable types
	template<class T>
	struct Argument<VRefPtr<T> > // final
	{
		static inline bool Append(VRefPtr<T>& out, const VString& inValue)
		{
			if (out.IsNull())
				out = new T();
			Argument<T>::Append(*out, inValue);
			return true;
		}
		static inline void SetFlag(VRefPtr<T>& out)
		{
			if (out.IsNull())
				out = new T();
			Argument<T>::SetFlag(*out);
		}
		static inline void ExpectedValues(VString& out)
		{
			Argument<T>::ExpectedValues(out);
		}
	};








	// SPECIFIC SPECIALIZATIONS

	template<>
	struct Argument<VString> // final
	{
		static inline bool Append(VString& out, const VString& inValue)
		{
			out = inValue;
			return true;
		}
		static inline void SetFlag(VString& out)
		{
			out = UniChar('1');
		}
		static inline void ExpectedValues(VString& /*out*/)
		{
		}
	};

	template<>
	struct Argument<sLONG> // final
	{
		static inline bool Append(sLONG& out, const VString& inValue)
		{
			out = inValue.GetLong();
			return true;
		}
		static inline void SetFlag(sLONG& out)
		{
			out = 1;
		}
		static inline void ExpectedValues(VString& out)
		{
			out += CVSTR("expected a signed integral");
		}
	};

	template<>
	struct Argument<uLONG> // final
	{
		static inline bool Append(uLONG& out, const VString& inValue)
		{
			out = (uLONG) inValue.GetLong();
			return true;
		}
		static inline void SetFlag(uLONG& out)
		{
			out = 1;
		}
		static inline void ExpectedValues(VString& out)
		{
			out += CVSTR("expected a unsigned integral");
		}
	};

	template<>
	struct Argument<sLONG8> // final
	{
		static inline bool Append(sLONG& out, const VString& inValue)
		{
			out = inValue.GetLong8();
			return true;
		}
		static inline void SetFlag(sLONG& out)
		{
			out = 1;
		}
		static inline void ExpectedValues(VString& out)
		{
			out += CVSTR("expected a unsigned integral");
		}
	};

	template<>
	struct Argument<uLONG8> // final
	{
		static inline bool Append(uLONG8& out, const VString& inValue)
		{
			out = (uLONG) inValue.GetLong8();
			return true;
		}
		static inline void SetFlag(uLONG& out)
		{
			out = 1;
		}
		static inline void ExpectedValues(VString& out)
		{
			out += CVSTR("expected a unsigned integral");
		}
	};

	template<>
	struct Argument<bool> // final
	{
		static inline bool Append(bool& out, const VString& inValue)
		{
			out = (inValue.GetBoolean() != 0);
			return true;
		}
		static inline void SetFlag(bool& out)
		{
			out = true;
		}
		static inline void ExpectedValues(VString& out)
		{
			out += CVSTR("expected 0, 1, true or false");
		}
	};







	// GENERIC SPECIALIZATION FOR CONTAINERS

	//! Traits for appending a string value to an arbitrary variable
	template<class T, template<class> class AllocT>
	struct Argument<std::vector<T, AllocT<T> > > // final
	{
		static inline bool Append(std::vector<T, AllocT<T> >& out, const VString& inValue)
		{
			// append a new empty element
			out.push_back(T());
			// reset the last element with the sub-type
			return Argument<T>::Append(out.back(), inValue);
		}
		static inline void SetFlag(std::vector<T, AllocT<T> >& out)
		{
			out.resize(1);
			out[0] = T();
		}
		static inline void ExpectedValues(VString& out)
		{
			Argument<T>::ExpectedValues(out);
		}
	};








} // namespace CommandLineParser
} // namespace Extension
END_TOOLBOX_NAMESPACE

#endif // __XTOOLBOX_KERNELIPC_GETOPT_EXTENSION_HXX__
