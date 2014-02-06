
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
#ifndef __VDocImage__
#define __VDocImage__

BEGIN_TOOLBOX_NAMESPACE


/** document image class */
class XTOOLBOX_API VDocImage : public VDocNode
{
public:
friend	class VDocNode;

									VDocImage():VDocNode() 
		{
			fType = kDOC_NODE_TYPE_IMAGE;
		}
virtual								~VDocImage() 
		{
		}

virtual	VDocNode*					Clone() const {	return static_cast<VDocNode *>(new VDocImage(this)); }

		/** insert child node at passed position (0-based) */
virtual	void						InsertChild( VDocNode *inNode, VIndex inIndex) { xbox_assert(false); } //this node cannot have children 

		/** add child node */
virtual	void						AddChild( VDocNode *inNode) { xbox_assert(false); } //this node cannot have children 

		//image properties
		
		/** get image source (url) */
		const VString&				GetSource() const;

		/** set image source (url) */
		void						SetSource( const VString& inSource);

		/** get image alternate text */
		const VString&				GetText() const;

		/** set image alternate text */
		void						SetText( const VString& inText);
	
		/** get associated html element name 
		@remarks
			it is used only to apply CSS rules
		*/
virtual	const VString&				GetElementName() const { return sElementName; }

protected:
									VDocImage( const VDocImage* inNodeImage):VDocNode(static_cast<const VDocNode *>(inNodeImage))	
									{
									}


static	VString						sElementName;

};


END_TOOLBOX_NAMESPACE

#endif
