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
#ifndef __VImageLayout__
#define __VImageLayout__

BEGIN_TOOLBOX_NAMESPACE

/** image layout 
@remarks
	class dedicated to manage image layout
*/
class VDocImageLayout: public VDocBaseLayout
{
public:
							VDocImageLayout(const VString& inURL);

		/** create from the passed image node */
							VDocImageLayout(const VDocImage *inNode);

virtual						~VDocImageLayout();
		
/////////////////	IDocNodeLayout interface

		/** set layout DPI */
virtual	void				SetDPI( const GReal inDPI = 72);

		/** update layout metrics 
		@remarks
			it will also load - or reload image if image file has been modified
		*/ 
virtual void				UpdateLayout(VGraphicContext *inGC);

		/** render layout element in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);

		/** update ascent (compute ascent relatively to line & text ascent, descent in px (relative to dpi set by SetDPI) - not including line embedded node(s) metrics) 
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateLayout
		*/
virtual	void				UpdateAscent(const GReal inLineAscent = 0, const GReal inLineDescent = 0, const GReal inTextAscent = 0, const GReal inTextDescent = 0);

		/** get ascent in px - relative to fDPI 
		@remarks
			should be called after UpdateAscent
		*/
virtual	GReal				GetAscent() const { return fAscent; }

		/** get descent in px - relative to fDPI 
		@remarks
			should be called after UpdateAscent
		*/
virtual	GReal				GetDescent() const { return fDesignBounds.GetHeight()-fAscent; }

/////////////////


/////////////////	VDocBaseLayout override

		/** initialize from the passed node */
virtual	void				InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited = false);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

/////////////////


		/** get image width in px (relative to layout dpi)
		@remarks
			valid only after UpdateLayout
		*/
		GReal				GetImageWidth() const;

		/** get image height in px (relative to layout dpi)  
		@remarks
			valid only after UpdateLayout
		*/
		GReal				GetImageHeight() const;

		const VString&		GetText() const { return fText; }

		/** return true if image exist */
		bool				ImageExists() const { return fPictureData != NULL; }

protected:
		void				_Init();

		void				_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw = false);

		//design datas


		/** image url */
		VString				fURL;

		/** image alternate text */
		VString				fText;


		//computed datas


		/** image width in TWIPS (not including margin, padding & border) */
		sLONG				fImgWidthTWIPS;

		/** image height in TWIPS (not including margin, padding & border) */
		sLONG				fImgHeightTWIPS;

		/** ref on picture data */
		const VPictureData*	fPictureData;

		/** ascent in px (used to align image relatively to baseline if image is embedded in a paragraph) */
		GReal				fAscent;
};



/** background image layout 
@remarks
	class dedicated to manage background image layout

	this class might be embedded in any VDocBaseLayout derived class & so does not derive from VDocBaseLayout:
	it manages VDocBaseLayout background image layout

	it might be used stand-alone (without VDocBaseLayout derived class) to paint background image
*/
class VDocBackgroundImageLayout: public VObject
{
public:
							VDocBackgroundImageLayout(const VString& inURL);

virtual						~VDocBackgroundImageLayout();
		

		/** set layout DPI */
void						SetDPI( const GReal inDPI = 72) { fDPI = inDPI; }

		/** update layout metrics 
		@param inGC
			graphic context
		@param inClipRect
			clip rect (user space)
		@param inOriginRect
			origin rect (user space)
		@param inHAlign
			horizontal alignment (relative to origin rect - for starting tile if image is repeated)
		@param inVAlign
			vertical alignment (relative to origin rect - for starting tile if image is repeated)
		@param inRepeat
			repeat mode (painting is bound by inClipRect)
		@param inDefaultSize
			kTEXT_BACKGROUND_SIZE_COVER : background image is scaled (preserving aspect ratio) in order to cover all origin rect (some image part might not be visible) - cf CSS W3C 'background-size'
			kTEXT_BACKGROUND_SIZE_CONTAIN : background image is scaled (preserving aspect ratio) in order to be fully contained in origin rect (all image is visible) - cf CSS W3C 'background-size'		
			kTEXT_BACKGROUND_SIZE_AUTO: image width & height are determined by the following parameters
		@param inWidth (used only if inDefaultSize == kTEXT_BACKGROUND_SIZE_AUTO)
			if equal to kTEXT_BACKGROUND_SIZE_AUTO, width is automatic (proportional to height according to image aspect ratio)
			otherwise it is either width in user space if inWidthIsPercentage == false, or percentage of origin rect width if inWidthIsPercentage == true
		@param inHeight (used only if inDefaultSize == kTEXT_BACKGROUND_SIZE_AUTO)
			if equal to kTEXT_BACKGROUND_SIZE_AUTO, height is automatic (proportional to width according to image aspect ratio)
			otherwise it is either height in user space if inWidthIsPercentage == false, or percentage of origin rect height if inWidthIsPercentage == true
		@param inWidthIsPercentage
			if true, inWidth is percentage of origin rect width (1 is equal to 100%)
		@param inHeightIsPercentage
			if true, inHeight is percentage of origin rect height (1 is equal to 100%)

		@remarks
			it will also load - or reload image if image file has been modified
		*/ 
		void				UpdateLayout(VGraphicContext *inGC, const VRect& inClipRect, const VRect& inOriginRect, 
										 const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
										 const eDocPropBackgroundRepeat inRepeat = kTEXT_BACKGROUND_REPEAT,
										 const eDocPropBackgroundSize inDefaultSize = kTEXT_BACKGROUND_SIZE_AUTO,
										 const GReal inWidth = kTEXT_BACKGROUND_SIZE_AUTO, const GReal inHeight = kTEXT_BACKGROUND_SIZE_AUTO,
										 const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false);

		/** render the background image in the passed graphic context (using the layout parameters as passed to UpdateLayout)
		@remarks		
			method does not save & restore gc context
		*/
		void				Draw( VGraphicContext *inGC);


		/** get image width in px (relative to layout dpi)  
		@remarks
			valid only after UpdateLayout
		*/
		GReal				GetImageWidth() const;

		/** get image height in px (relative to layout dpi) 
		@remarks
			valid only after UpdateLayout
		*/
		GReal				GetImageHeight() const;

		/** return true if image exist */
		bool				ImageExists() const { return fPictureData != NULL; }

protected:
		void				_Init();

		//design datas

		/** layout DPI */
		GReal				fDPI;

		/** image url */
		VString				fURL;


		//computed datas


		/** image width in TWIPS */
		sLONG				fImgWidthTWIPS;

		/** image height in TWIPS */
		sLONG				fImgHeightTWIPS;

		/** ref on picture data */
		const VPictureData*	fPictureData;
};



/** image layout picture data manager
@remarks
	it manages a picture data cache for absolute & relative urls
	(relative urls are searched only with database & app resource base url)
*/

class XTOOLBOX_API VPictureProxyCache_Folder;

class XTOOLBOX_API VDocImageLayoutPictureDataManager: public VObject
{
public:

static	VDocImageLayoutPictureDataManager*	Get();
static	bool							Init();
static	void							Deinit();

										VDocImageLayoutPictureDataManager();
virtual									~VDocImageLayoutPictureDataManager();


		/** return picture data from encoded url */
		const VPictureData*				Retain( const VString& inUrl);

		/** sync cached pictures */ 
		void							AutoWash();

		/** clear cached pictures */
		void							Clear();		

		/** set application interface */
		void							SetApplicationInterface( IGraphics_ApplicationIntf *inApplication)	
		{ 
			fAppIntf = inApplication; 
		}

		/** get application interface */
		IGraphics_ApplicationIntf*		GetApplicationInterface() { return fAppIntf;}

protected:
		typedef std::vector<VPictureProxyCache_Folder *> VectorOfFolderCache;

		IGraphics_ApplicationIntf*			fAppIntf;

		VFilePath							fCurDBPath;

		VectorOfFolderCache					fFolderCache;

		sLONG								fResourceFolderCacheCount;

		VCriticalSection					fMutex;

static	VDocImageLayoutPictureDataManager*	sSingleton;
};


END_TOOLBOX_NAMESPACE

#endif
