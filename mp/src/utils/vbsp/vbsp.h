//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined( VBSP_H )
#define VBSP_H

	   
#include "cmdlib.h"
#include "mathlib/vector.h"
#include "scriplib.h"
#include "polylib.h"
#include "threads.h"
#include "bsplib.h"
#include "qfiles.h"
#include "utilmatlib.h"
#include "chunkfile.h"
#ifdef MAPBASE_VSCRIPT
#include "vscript/ivscript.h"
#endif

#ifdef WIN32
#pragma warning( disable: 4706 )
#endif

class CUtlBuffer;

#define	MAX_BRUSH_SIDES	128
#define	CLIP_EPSILON	0.1

#define	TEXINFO_NODE		-1		// side is allready on a node

// this will output glview files for the given brushmodel.  Brushmodel 1 is the world, 2 is the first brush entity, etc.
#define DEBUG_BRUSHMODEL 0

#ifdef MAPBASE
// Activates compiler code for parallax corrected cubemaps
// https://developer.valvesoftware.com/wiki/Parallax_Corrected_Cubemaps
#define PARALLAX_CORRECTED_CUBEMAPS 1
#endif

struct portal_t;
struct node_t;

struct plane_t : public dplane_t
{
	plane_t			*hash_chain;

	plane_t() { normal.Init(); }
};


struct brush_texture_t
{
	Vector	UAxis;
	Vector	VAxis;
	vec_t	shift[2];
	vec_t	rotate;
	vec_t	textureWorldUnitsPerTexel[2];
	vec_t	lightmapWorldUnitsPerLuxel;
	char	name[TEXTURE_NAME_LENGTH];
	int		flags;

	brush_texture_t() : UAxis(0,0,0), VAxis(0,0,0) {}
};

struct mapdispinfo_t;

struct side_t
{
	int			    planenum;
	int			    texinfo;
	mapdispinfo_t	*pMapDisp;

	winding_t	    *winding;
	side_t			*original;	    // bspbrush_t sides will reference the mapbrush_t sides
	int			    contents;		// from miptex
	int			    surf;			// from miptex
	qboolean	    visible;		// choose visble planes first
	qboolean	    tested;			// this plane allready checked as a split
	qboolean	    bevel;			// don't ever use for bsp splitting

    side_t			*next;
    int             origIndex;      
	int				id;				// This is the unique id generated by worldcraft for this side.
	unsigned int	smoothingGroups;
	CUtlVector<int>	aOverlayIds;		// List of overlays that reside on this side.
	CUtlVector<int>	aWaterOverlayIds;	// List of water overlays that reside on this side.
	bool			m_bDynamicShadowsEnabled;	// Goes into dface_t::SetDynamicShadowsEnabled().
};

struct mapbrush_t
{
	int		entitynum;
	int		brushnum;
	int		id;						// The unique ID of this brush in the editor, used for reporting errors.
	int		contents;
	Vector	mins, maxs;
	int		numsides;
	side_t	*original_sides;
};

#define	PLANENUM_LEAF			-1

#define	MAXEDGES		32

struct face_t
{
	int				id;

	face_t			*next;		            // on node

	// the chain of faces off of a node can be merged or split,
	// but each face_t along the way will remain in the chain
	// until the entire tree is freed
	face_t			*merged;	            // if set, this face isn't valid anymore
	face_t			*split[2];	            // if set, this face isn't valid anymore

	portal_t		*portal;
	int				texinfo;
	int             dispinfo;
	// This is only for surfaces that are the boundaries of fog volumes
	// (ie. water surfaces)
	// All of the rest of the surfaces can look at their leaf to find out
	// what fog volume they are in.
	node_t			*fogVolumeLeaf;

    int				planenum;
	int				contents;	            // faces in different contents can't merge
	int				outputnumber;
	winding_t		*w;
	int				numpoints;
	qboolean		badstartvert;	        // tjunctions cannot be fixed without a midpoint vertex
	int				vertexnums[MAXEDGES];
    side_t          *originalface;          // save the "side" this face came from
	int				firstPrimID;
	int				numPrims;
	unsigned int	smoothingGroups;
};

void EmitFace( face_t *f, qboolean onNode );

struct mapdispinfo_t
{
	face_t			face;
	int				entitynum;
    int				power;
    int				minTess;
    float			smoothingAngle;
    Vector			uAxis;
    Vector			vAxis;
	Vector			startPosition;
	float			alphaValues[MAX_DISPVERTS];
    float			maxDispDist;
    float			dispDists[MAX_DISPVERTS];
    Vector			vectorDisps[MAX_DISPVERTS];
	Vector			vectorOffsets[MAX_DISPVERTS];
    int				contents;
	int				brushSideID;
	unsigned short	triTags[MAX_DISPTRIS];
	int				flags;

#ifdef VSVMFIO
	float			m_elevation;						// "elevation"
	Vector			m_offsetNormals[ MAX_DISPTRIS ];	// "offset_normals"
#endif // VSVMFIO

};

extern int              nummapdispinfo;
extern mapdispinfo_t    mapdispinfo[MAX_MAP_DISPINFO];

extern float			g_defaultLuxelSize;
extern float			g_luxelScale;
extern float			g_minLuxelScale;
extern bool				g_BumpAll;
extern int				g_nDXLevel;

int GetDispInfoEntityNum( mapdispinfo_t *pDisp );
void ComputeBoundsNoSkybox( );

struct bspbrush_t
{
	int					id;
	bspbrush_t			*next;
	Vector	            mins, maxs;
	int		            side, testside;		// side of node during construction
	mapbrush_t	        *original;
	int		            numsides;
	side_t	            sides[6];			// variably sized
};


#define	MAX_NODE_BRUSHES	8

struct leafface_t 
{
	face_t		*pFace;
	leafface_t	*pNext;
};

struct node_t
{
	int				id;

	// both leafs and nodes
	int				planenum;	// -1 = leaf node
	node_t			*parent;
	Vector			mins, maxs;	// valid after portalization
	bspbrush_t		*volume;	// one for each leaf/node

	// nodes only
	side_t			*side;		// the side that created the node
	node_t			*children[2];
	face_t			*faces;		// these are the cutup ones that live in the plane of "side".

	// leafs only
	bspbrush_t		*brushlist;	// fragments of all brushes in this leaf
	leafface_t		*leaffacelist;
	int				contents;	// OR of all brush contents
	int				occupied;	// 1 or greater can reach entity
	entity_t		*occupant;	// for leak file testing
	int				cluster;	// for portalfile writing
	int				area;		// for areaportals
	portal_t		*portals;	// also on nodes during construction
	int				diskId;		// dnodes or dleafs index after this has been emitted
};


struct portal_t
{
	int				id;
	plane_t		    plane;
	node_t		    *onnode;		// NULL = outside box
	node_t		    *nodes[2];		// [0] = front side of plane
	portal_t		*next[2];
	winding_t	    *winding;
	qboolean	    sidefound;		// false if ->side hasn't been checked
	side_t		    *side;			// NULL = non-visible
	face_t		    *face[2];		// output face in bsp file
};


struct tree_t
{
	node_t		*headnode;
	node_t		outside_node;
	Vector		mins, maxs;
	bool		leaked;
};


extern	int			entity_num;

struct LoadSide_t;
struct LoadEntity_t;
class CManifest;

class CMapFile
{
public:
			CMapFile( void ) { Init(); }

	void	Init( void );

	void				AddPlaneToHash (plane_t *p);
	int					CreateNewFloatPlane (Vector& normal, vec_t dist);
	int					FindFloatPlane (Vector& normal, vec_t dist);
	int					PlaneFromPoints(const Vector &p0, const Vector &p1, const Vector &p2);
	void				AddBrushBevels (mapbrush_t *b);
	qboolean			MakeBrushWindings (mapbrush_t *ob);
	void				MoveBrushesToWorld( entity_t *mapent );
	void				MoveBrushesToWorldGeneral( entity_t *mapent );
	void				RemoveContentsDetailFromEntity( entity_t *mapent );
	int					SideIDToIndex( int brushSideID );
	void				AddLadderKeys( entity_t *mapent );
	ChunkFileResult_t	LoadEntityCallback(CChunkFile *pFile, int nParam);
	void				ForceFuncAreaPortalWindowContents();
	ChunkFileResult_t	LoadSideCallback(CChunkFile *pFile, LoadSide_t *pSideInfo);
	ChunkFileResult_t	LoadConnectionsKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity);
	ChunkFileResult_t	LoadSolidCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity);
	void				TestExpandBrushes(void);

	static char			m_InstancePath[ MAX_PATH ];
	static void			SetInstancePath( const char *pszInstancePath );
	static const char	*GetInstancePath( void ) { return m_InstancePath; }
	static bool			DeterminePath( const char *pszBaseFileName, const char *pszInstanceFileName, char *pszOutFileName );

	void				CheckForInstances( const char *pszFileName );
	void				MergeInstance( entity_t *pInstanceEntity, CMapFile *Instance );
	void				MergePlanes( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix );
	void				MergeBrushes( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix );
	void				MergeBrushSides( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix );
	void				ReplaceInstancePair( epair_t *pPair, entity_t *pInstanceEntity );
	void				MergeEntities( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix );
	void				MergeOverlays( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix );

	static int	m_InstanceCount;
	static int	c_areaportals;

	plane_t		mapplanes[MAX_MAP_PLANES];
	int			nummapplanes;

	#define	PLANE_HASHES	1024
	plane_t		*planehash[PLANE_HASHES];

	int			nummapbrushes;
	mapbrush_t	mapbrushes[MAX_MAP_BRUSHES];

	Vector		map_mins, map_maxs;

	int			nummapbrushsides;
	side_t		brushsides[MAX_MAP_BRUSHSIDES];

	brush_texture_t side_brushtextures[MAX_MAP_BRUSHSIDES];

	int			num_entities;
	entity_t	entities[MAX_MAP_ENTITIES];

	int			c_boxbevels;
	int			c_edgebevels;
	int			c_clipbrushes;
	int			g_ClipTexinfo;

	class CConnectionPairs
	{
		public:
		CConnectionPairs( epair_t *pair, CConnectionPairs *next )
		{
			m_Pair = pair;
			m_Next = next;
		}

		epair_t				*m_Pair;
		CConnectionPairs	*m_Next;
	};

	CConnectionPairs	*m_ConnectionPairs;

	int					m_StartMapOverlays;
	int					m_StartMapWaterOverlays;

#ifdef MAPBASE_VSCRIPT
	HSCRIPT				GetScriptInstance();

	static ScriptHook_t	g_Hook_OnMapLoaded;

	// VScript functions
	ALLOW_SCRIPT_ACCESS();
private:

	const Vector&	GetMins() { return map_mins; }
	const Vector&	GetMaxs() { return map_maxs; }

	int				GetNumMapBrushes() { return nummapbrushes; }

	const Vector&	GetEntityOrigin(int idx) { return (idx < num_entities && idx >= 0) ? entities[idx].origin : vec3_origin; }
	int				GetEntityFirstBrush(int idx) { return (idx < num_entities && idx >= 0) ? entities[idx].firstbrush : 0; }
	int				GetEntityNumBrushes(int idx) { return (idx < num_entities && idx >= 0) ? entities[idx].numbrushes : 0; }

	void			ScriptGetEntityKeyValues(int idx, HSCRIPT hKeyTable, HSCRIPT hValTable);

	int				ScriptAddSimpleEntityKV(HSCRIPT hKV/*, const Vector& vecOrigin, int iFirstBrush, int iNumBrushes*/);
	int				ScriptAddInstance(const char *pszVMF, const Vector& vecOrigin, const QAngle& angAngles);

	int				GetNumEntities() { return num_entities; }

	HSCRIPT			m_hScriptInstance;
#endif
};

extern CMapFile	*g_MainMap;
extern CMapFile	*g_LoadingMap;

extern CUtlVector< CMapFile * > g_Maps;

extern	int			g_nMapFileVersion;

extern	qboolean	noprune;
extern	qboolean	nodetail;
extern	qboolean	fulldetail;
extern	qboolean	nomerge;
extern	qboolean	nomergewater;
extern	qboolean	nosubdiv;
extern	qboolean	nowater;
extern	qboolean	noweld;
extern	qboolean	noshare;
extern	qboolean	notjunc;
extern	qboolean	nocsg;
extern	qboolean	noopt;
extern  qboolean	dumpcollide;
extern	qboolean	nodetailcuts;
extern  qboolean	g_DumpStaticProps;
extern	qboolean	g_bSkyVis;
extern	vec_t		microvolume;
extern	bool		g_snapAxialPlanes;
extern	bool		g_NodrawTriggers;
extern	bool		g_DisableWaterLighting;
extern	bool		g_bAllowDetailCracks;
extern	bool		g_bNoVirtualMesh;
#ifdef MAPBASE
extern	bool		g_bNoHiddenManifestMaps;
extern bool			g_bPropperInsertAllAsStatic;
extern bool			g_bPropperStripEntities;
#endif
extern	char		g_outbase[32];

extern	char	g_source[1024];
extern char		g_mapbase[ 64 ];
extern CUtlVector<int> g_SkyAreas;

bool 	LoadMapFile( const char *pszFileName );
int		GetVertexnum( Vector& v );
bool Is3DSkyboxArea( int area );

//=============================================================================

// textures.c

struct textureref_t
{
	char	name[TEXTURE_NAME_LENGTH];
	int		flags;
	float	lightmapWorldUnitsPerLuxel;
	int		contents;
};

extern	textureref_t	textureref[MAX_MAP_TEXTURES];

int	FindMiptex (const char *name);

int TexinfoForBrushTexture (plane_t *plane, brush_texture_t *bt, const Vector& origin);
int GetSurfaceProperties2( MaterialSystemMaterial_t matID, const char *pMatName );

extern int g_SurfaceProperties[MAX_MAP_TEXDATA];
void LoadSurfaceProperties( void );

int PointLeafnum ( dmodel_t* pModel, const Vector& p );

//=============================================================================

void FindGCD (int *v);

mapbrush_t *Brush_LoadEntity (entity_t *ent);
int	PlaneTypeForNormal (Vector& normal);
qboolean MakeBrushPlanes (mapbrush_t *b);
int		FindIntPlane (int *inormal, int *iorigin);
void	CreateBrush (int brushnum);


//=============================================================================
// detail objects
//=============================================================================

void LoadEmitDetailObjectDictionary( char const* pGameDir );
void EmitDetailObjects();

//=============================================================================
// static props
//=============================================================================

void EmitStaticProps();
bool LoadStudioModel( char const* pFileName, char const* pEntityType, CUtlBuffer& buf );

//=============================================================================
//=============================================================================
// procedurally created .vmt files
//=============================================================================

void EmitStaticProps();

// draw.c

extern Vector	draw_mins, draw_maxs;
extern bool g_bLightIfMissing;

void Draw_ClearWindow (void);
void DrawWinding (winding_t *w);

void GLS_BeginScene (void);
void GLS_Winding (winding_t *w, int code);
void GLS_EndScene (void);

//=============================================================================

// csg

enum detailscreen_e
{
	FULL_DETAIL = 0,
	ONLY_DETAIL = 1,
	NO_DETAIL	= 2,
};

#define TRANSPARENT_CONTENTS	(CONTENTS_GRATE|CONTENTS_WINDOW)
	
#include "csg.h"

//=============================================================================

// brushbsp

void WriteBrushList (char *name, bspbrush_t *brush, qboolean onlyvis);

bspbrush_t *CopyBrush (bspbrush_t *brush);

void SplitBrush (bspbrush_t *brush, int planenum,
	bspbrush_t **front, bspbrush_t **back);

tree_t *AllocTree (void);
node_t *AllocNode (void);
bspbrush_t *AllocBrush (int numsides);
int	CountBrushList (bspbrush_t *brushes);
void FreeBrush (bspbrush_t *brushes);
vec_t BrushVolume (bspbrush_t *brush);
node_t *NodeForPoint (node_t *node, Vector& origin);

void BoundBrush (bspbrush_t *brush);
void FreeBrushList (bspbrush_t *brushes);
node_t	*PointInLeaf (node_t *node, Vector& point);

tree_t *BrushBSP (bspbrush_t *brushlist, Vector& mins, Vector& maxs);

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4
int BrushBspBoxOnPlaneSide (const Vector& mins, const Vector& maxs, dplane_t *plane);
extern qboolean WindingIsTiny (winding_t *w);

//=============================================================================

// portals.c

int VisibleContents (int contents);

void MakeHeadnodePortals (tree_t *tree);
void MakeNodePortal (node_t *node);
void SplitNodePortals (node_t *node);

qboolean	Portal_VisFlood (portal_t *p);

qboolean FloodEntities (tree_t *tree);
void FillOutside (node_t *headnode);
void FloodAreas (tree_t *tree);
void MarkVisibleSides (tree_t *tree, int start, int end, int detailScreen);
void MarkVisibleSides (tree_t *tree, mapbrush_t **ppBrushes, int nCount );
void FreePortal (portal_t *p);
void EmitAreaPortals (node_t *headnode);

void MakeTreePortals (tree_t *tree);

//=============================================================================

// glfile.c

void OutputWinding (winding_t *w, FileHandle_t glview);
void OutputWindingColor (winding_t *w, FileHandle_t glview, int r, int g, int b);
void WriteGLView (tree_t *tree, char *source);
void WriteGLViewFaces (tree_t *tree, const char *source);
void WriteGLViewBrushList( bspbrush_t *pList, const char *pName );
//=============================================================================

// leakfile.c

void LeakFile (tree_t *tree);
void AreaportalLeakFile( tree_t *tree, portal_t *pStartPortal, portal_t *pEndPortal, node_t *pStart );

//=============================================================================

// prtfile.c

void AddVisCluster( entity_t *pFuncVisCluster );
void WritePortalFile (tree_t *tree);

//=============================================================================

// writebsp.c

void SetModelNumbers (void);
void SetLightStyles (void);

void BeginBSPFile (void);
void WriteBSP (node_t *headnode, face_t *pLeafFaceList);
void EndBSPFile (void);
void BeginModel (void);
void EndModel (void);

extern	int firstmodeledge;
extern	int	firstmodelface;

//=============================================================================

// faces.c

void MakeFaces (node_t *headnode);
void MakeDetailFaces (node_t *headnode);
face_t *FixTjuncs( node_t *headnode, face_t *pLeafFaceList );

face_t	*AllocFace (void);
void FreeFace (face_t *f);
void FreeFaceList( face_t *pFaces );

void MergeFaceList(face_t **pFaceList);
void SubdivideFaceList(face_t **pFaceList);

extern face_t		*edgefaces[MAX_MAP_EDGES][2];


//=============================================================================

// tree.c

void FreeTree (tree_t *tree);
void FreeTree_r (node_t *node);
void PrintTree_r (node_t *node, int depth);
void FreeTreePortals_r (node_t *node);
void PruneNodes_r (node_t *node);
void PruneNodes (node_t *node);

// Returns true if the entity is a func_occluder
bool IsFuncOccluder( int entity_num );


//=============================================================================
// ivp.cpp
class CPhysCollide;
void EmitPhysCollision();
void DumpCollideToGlView( CPhysCollide *pCollide, const char *pFilename );
void EmitWaterVolumesForBSP( dmodel_t *pModel, node_t *headnode );

//=============================================================================
// find + find or create the texdata 
int FindTexData( const char *pName );
int FindOrCreateTexData( const char *pName );
// Add a clone of an existing texdata with a new name
int AddCloneTexData( dtexdata_t *pExistingTexData, char const *cloneTexDataName );
int FindOrCreateTexInfo( const texinfo_t &searchTexInfo );
int FindAliasedTexData( const char *pName, dtexdata_t *sourceTexture );
int FindTexInfo( const texinfo_t &searchTexInfo );

//=============================================================================
// normals.c
void SaveVertexNormals( void );

//=============================================================================
// cubemap.cpp
#ifdef PARALLAX_CORRECTED_CUBEMAPS
extern char* g_pParallaxObbStrs[MAX_MAP_CUBEMAPSAMPLES];
void Cubemap_InsertSample( const Vector& origin, int size, char* pParallaxObbStr );
#else
void Cubemap_InsertSample( const Vector& origin, int size );
#endif
void Cubemap_CreateDefaultCubemaps( void );
void Cubemap_SaveBrushSides( const char *pSideListStr );
void Cubemap_FixupBrushSidesMaterials( void );
void Cubemap_AttachDefaultCubemapToSpecularSides( void );
// Add skipped cubemaps that are referenced by the engine
void Cubemap_AddUnreferencedCubemaps( void );

//=============================================================================
// overlay.cpp
#define OVERLAY_MAP_STRLEN			256

struct mapoverlay_t
{
	int					nId;
	unsigned short		m_nRenderOrder;
	char				szMaterialName[OVERLAY_MAP_STRLEN];
	float				flU[2];
	float				flV[2];
	float				flFadeDistMinSq;
	float				flFadeDistMaxSq;
	Vector				vecUVPoints[4];
	Vector				vecOrigin;
	Vector				vecBasis[3];
	CUtlVector<int>		aSideList;
	CUtlVector<int>		aFaceList;
};

extern CUtlVector<mapoverlay_t>	g_aMapOverlays;
extern CUtlVector<mapoverlay_t> g_aMapWaterOverlays;

int Overlay_GetFromEntity( entity_t *pMapEnt );
void Overlay_UpdateSideLists( int StartIndex );
void Overlay_AddFaceToLists( int iFace, side_t *pSide );
void Overlay_EmitOverlayFaces( void );
void OverlayTransition_UpdateSideLists( int StartIndex );
void OverlayTransition_AddFaceToLists( int iFace, side_t *pSide );
void OverlayTransition_EmitOverlayFaces( void );
void Overlay_Translate( mapoverlay_t *pOverlay, Vector &OriginOffset, QAngle &AngleOffset, matrix3x4_t &Matrix );

//=============================================================================

void RemoveAreaPortalBrushes_R( node_t *node );

dtexdata_t *GetTexData( int index );

#endif

