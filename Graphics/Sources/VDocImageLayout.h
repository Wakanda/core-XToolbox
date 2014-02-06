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

/** image layer settings */
struct sDocImageLayerSettings
{
	GraphicContextType  fGCType;
	GReal				fDPI;
	VAffineTransform	fDrawTransform;
	VRect				fDrawUserBounds;
	uLONG8				fStamp;
};

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

		/** set layout DPI 
		@remarks
			on default image dpi is assumed to be 72 (for 4D form compatibility)
			and so image is scaled to inDPI/72
			(for instance if a image is 48x48 it will be 48px x 48px  if inDPI=72
			 and 64px x 64px if inDPI=96 - 64px = 48*96/72)
		*/
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
virtual	GReal				GetDescent() const { return fLayoutBounds.GetHeight()-fAscent; }

/////////////////	IDocBaseLayout override

		/** return document node type */
virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_IMAGE; }

		/** return local text length 
		@remarks
			it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
		*/
virtual	uLONG				GetTextLength() const { return 1; }

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

		/** create or retain image layer 
		
		@remarks
			it is used to store a snapshot of the transformed image 
			
			depending on differences between current & last draw settings 
			it will create new layer (created from passed picture data & current draw settings - snapshot of transformed image) 
			or return the one from last draw unmodified
		*/
static	VImageOffScreenRef	CreateOrRetainImageLayer( VGraphicContext *inGC, const VPictureData* inPictureData, const VImageOffScreenRef inLayer, const sDocImageLayerSettings& inDrawSettings, const sDocImageLayerSettings* inLastDrawSettings = NULL, bool inRepeat = false);

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
		uLONG8				fPictureDataStamp;

		/** image offscreen layer (to store snapshot of the transformed image) */
		VRefPtr<VImageOffScreen>	fImgLayer;

		sDocImageLayerSettings		fImgLayerSettings;

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
							/** background image layout 

							@param inURL
								image url (encoded)
							@param inHAlign
								horizontal alignment (relative to origin rect - for starting tile if image is repeated)
							@param inVAlign
								vertical alignment (relative to origin rect - for starting tile if image is repeated)
							@param inRepeat
								repeat mode 
							@param inWidth 
								if equal to kDOC_BACKGROUND_SIZE_COVER:	background image is scaled (preserving aspect ratio) in order to cover all origin rect (some image part might not be visible) - cf CSS W3C 'background-size'
																			and inHeight is ignored
								if equal to kDOC_BACKGROUND_SIZE_CONTAIN:	background image is scaled (preserving aspect ratio) in order to be fully contained in origin rect (all image is visible) - cf CSS W3C 'background-size'
																			and inHeight is ignored
								if equal to kDOC_BACKGROUND_SIZE_AUTO:		width is automatic (proportional to height according to image aspect ratio)
							
								otherwise it is either width in point if inWidthIsPercentage == false, or percentage of origin rect width if inWidthIsPercentage == true
							@param inHeight (mandatory only if inWidth >= kDOC_BACKGROUND_SIZE_AUTO)
								if equal to kDOC_BACKGROUND_SIZE_AUTO:		height is automatic (proportional to width according to image aspect ratio)
							
								otherwise it is either height in point if inWidthIsPercentage == false, or percentage of origin rect height if inWidthIsPercentage == true
							@param inWidthIsPercentage
								if true, inWidth is percentage of origin rect width (1 is equal to 100%)
							@param inHeightIsPercentage
								if true, inHeight is percentage of origin rect height (1 is equal to 100%)
							*/ 
							VDocBackgroundImageLayout(const VString& inURL,	
										 const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
										 const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
										 const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
										 const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false);
	
virtual						~VDocBackgroundImageLayout();
		

		/** set layout DPI 
		@remarks
			on default image dpi is assumed to be 72 (for 4D form compatibility)
			and so image is scaled to inDPI/72
			(for instance if a image is 48x48 it will be 48px x 48px  if inDPI=72
			 and 64px x 64px if inDPI=96 - 64px = 48*96/72)
		*/
void						SetDPI( const GReal inDPI = 72) { fDPI = inDPI; }


		/** return url string (encoded) */
		const VString&		GetURL() const { return fURL; }
		
		/** set clipping bounds (in px - relative to layout dpi) */
		void				SetClipBounds( const VRect& inClipRect);
		const VRect&		GetClipBounds() const { return fClipBounds; }

		/** set origin bounds (in px - relative to layout dpi) */
		void				SetOriginBounds( const VRect& inOriginRect);
		const VRect&		GetOriginBounds() const { return fOriginBounds; }

		/** update layout 
		@remarks
			it will also load - or reload image if image file has been modified
			
			SetClipBounds & SetOriginBounds should have been called before
		*/
		void				UpdateLayout(VGraphicContext *inGC);

		/** render the background image in the passed graphic context (using the layout parameters as passed to UpdateLayout)
		@remarks		
			method does not save & restore gc context
		*/
		void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint());


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

		/** get horizontal alignment */
		eStyleJust			GetHAlign() const { return fHAlign; }

		/** get vertical alignment */
		eStyleJust			GetVAlign() const { return fVAlign; }

		/** get repeat mode */
		eDocPropBackgroundRepeat	GetRepeat() const { return fRepeat; }

		/** get image design width (in point or percentage) */
		GReal				GetWidth() const { return fWidth; }
		bool				IsWidthPercentage() const { return fWidthIsPercentage; }

		/** get image design height (in point or percentage) */
		GReal				GetHeight() const { return fHeight; }
		bool				IsHeightPercentage() const { return fHeightIsPercentage; }

		/** return true if image exist */
		bool				ImageExists() const { return fPictureData != NULL; }

protected:
		friend class VDocBaseLayout;

		void				_Init();

		//design datas

		/** layout DPI */
		GReal						fDPI;

		/** image url */
		VString						fURL;

		/** clip rectangle (in px - relative to layout dpi) */
		VRect						fClipBounds;

		/** origin rectangle (used for image alignment) (in px - relative to layout dpi) */
		VRect						fOriginBounds;

		/** horizontal alignment */
		eStyleJust					fHAlign;

		/** vertical alignment */
		eStyleJust					fVAlign;

		/** repeat mode */ 
		eDocPropBackgroundRepeat	fRepeat;

		/** design image width in point
		@remarks
			if equal to kDOC_BACKGROUND_SIZE_COVER:	background image is scaled (preserving aspect ratio) in order to cover all origin rect (some image part might not be visible) - cf CSS W3C 'background-size'
														and inHeight is ignored
			if equal to kDOC_BACKGROUND_SIZE_CONTAIN:	background image is scaled (preserving aspect ratio) in order to be fully contained in origin rect (all image is visible) - cf CSS W3C 'background-size'
														and inHeight is ignored
			if equal to kDOC_BACKGROUND_SIZE_AUTO:		width is automatic (proportional to height according to image aspect ratio)
		
			otherwise it is either width in point if fWidthIsPercentage == false, or percentage of origin rect width if fWidthIsPercentage == true
		*/ 
		GReal						fWidth;
		bool						fWidthIsPercentage;

		/** design image height in point
		@remarks	
			if equal to kDOC_BACKGROUND_SIZE_AUTO:		height is automatic (proportional to width according to image aspect ratio)
		
			otherwise it is either height in point if inWidthIsPercentage == false, or percentage of origin rect height if inWidthIsPercentage == true
		*/ 
		GReal						fHeight;
		bool						fHeightIsPercentage;

		//computed datas

		/** image width in TWIPS */
		sLONG						fImgWidthTWIPS;

		/** image height in TWIPS */
		sLONG						fImgHeightTWIPS;

		/** ref on picture data */
		const VPictureData*			fPictureData;
		uLONG8						fPictureDataStamp;

		/** image offscreen layer (to store snapshot of the transformed image) */
		VRefPtr<VImageOffScreen>	fImgLayer;

		sDocImageLayerSettings		fImgLayerSettings;

		/** image drawing bounds in user space (if repeated it is start image pattern drawing bounds) */
		VRect						fImgDrawBounds;

		/** pre-computed repeat x count */
		sLONG						fRepeatXCount;

		/** pre-computed repeat y count */
		sLONG						fRepeatYCount;
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
		const VPictureData*				Retain( const VString& inUrl, uLONG8 *outStamp = NULL);

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
