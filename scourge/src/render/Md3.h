#ifndef _MD3_H
#define _MD3_H
#pragma once

/**
 * Credit for this code is mainly due to:
 * DigiBen     digiben@gametutorials.com
 * Look up his other great tutorials at:
 * http://www.gametutorials.com
 */


#include <vector>
#include "render.h"

class ModelLoader;
class MD3Shape;

#define BYTE Uint8



// These defines are used to pass into GetModel(), which is member function of CModelMD3

#define kLower 0   // This stores the ID for the legs model
#define kUpper 1   // This stores the ID for the torso model
#define kHead 2   // This stores the ID for the head model
#define kWeapon 3   // This stores the ID for the weapon model

// This enumeration stores all the animations in order from the config file (.cfg)
typedef enum {
	// If one model is set to one of the BOTH_* animations, the other one should be too,
	// otherwise it looks really bad and confusing.

	BOTH_DEATH1 = 0,  // The first twirling death animation
	BOTH_DEAD1,    // The end of the first twirling death animation
	BOTH_DEATH2,   // The second twirling death animation
	BOTH_DEAD2,    // The end of the second twirling death animation
	BOTH_DEATH3,   // The back flip death animation
	BOTH_DEAD3,    // The end of the back flip death animation

	// The next block is the animations that the upper body performs

	TORSO_GESTURE,   // The torso's gesturing animation

	TORSO_ATTACK,   // The torso's attack1 animation
	TORSO_ATTACK2,   // The torso's attack2 animation

	TORSO_DROP,    // The torso's weapon drop animation
	TORSO_RAISE,   // The torso's weapon pickup animation

	TORSO_STAND,   // The torso's idle stand animation
	TORSO_STAND2,   // The torso's idle stand2 animation

	// The final block is the animations that the legs perform

	LEGS_WALKCR,   // The legs's crouching walk animation
	LEGS_WALK,    // The legs's walk animation
	LEGS_RUN,    // The legs's run animation
	LEGS_BACK,    // The legs's running backwards animation
	LEGS_SWIM,    // The legs's swimming animation

	LEGS_JUMP,    // The legs's jumping animation
	LEGS_LAND,    // The legs's landing animation

	LEGS_JUMPB,    // The legs's jumping back animation
	LEGS_LANDB,    // The legs's landing back animation

	LEGS_IDLE,    // The legs's idle stand animation
	LEGS_IDLECR,   // The legs's idle crouching animation

	LEGS_TURN,    // The legs's turn animation

	MAX_ANIMATIONS   // The define for the maximum amount of animations
} eAnimations;


// This file stores all of our structures and classes (besides the modular model ones in main.h)
// in order to read in and display a Quake3 character.  The file format is of type
// .MD3 and comes in many different files for each main body part section.  We convert
// these Quake3 structures to our own structures in Md3.cpp so that we are not dependant
// on their model data structures.  You can do what ever you want, but I like mine :P :)

/// This holds the header information that is read in at the beginning of the file.
struct tMd3Header {
	char fileID[4];     // This stores the file ID - Must be "IDP3"
	int  version;     // This stores the file version - Must be 15
	char strFile[68];    // This stores the name of the file
	int  numFrames;     // This stores the number of animation frames
	int  numTags;     // This stores the tag count
	int  numMeshes;     // This stores the number of sub-objects in the mesh
	int  numMaxSkins;    // This stores the number of skins for the mesh
	int  headerSize;     // This stores the mesh header size
	int  tagStart;     // This stores the offset into the file for tags
	int  tagEnd;      // This stores the end offset into the file for tags
	int  fileSize;     // This stores the file size
};

/// This structure is used to read in the mesh data for the .md3 models.
struct tMd3MeshInfo {
	char meshID[4];     // This stores the mesh ID (We don't care)
	char strName[68];    // This stores the mesh name (We do care)
	int  numMeshFrames;    // This stores the mesh aniamtion frame count
	int  numSkins;     // This stores the mesh skin count
	int     numVertices;    // This stores the mesh vertex count
	int  numTriangles;    // This stores the mesh face count
	int  triStart;     // This stores the starting offset for the triangles
	int  headerSize;     // This stores the header size for the mesh
	int     uvStart;     // This stores the starting offset for the UV coordinates
	int  vertexStart;    // This stores the starting offset for the vertex indices
	int  meshSize;     // This stores the total mesh size
};

/// This is our tag structure for the .MD3 file format.  These are used link other
/// models to and the rotate and translate the child models of that model.
struct tMd3Tag {
	char  strName[64];   // This stores the name of the tag (I.E. "tag_torso")
	CVector3 vPosition;    // This stores the translation that should be performed
	float  rotation[3][3];   // This stores the 3x3 rotation matrix for this frame
};

/// This stores the bone information (useless as far as I can see...).
struct tMd3Bone {
	float mins[3];     // This is the min (x, y, z) value for the bone
	float maxs[3];     // This is the max (x, y, z) value for the bone
	float position[3];    // This supposedly stores the bone position???
	float scale;      // This stores the scale of the bone
	char creator[16];    // The modeler used to create the model (I.E. "3DS Max")
};


/// This stores the normals and vertex indices.
struct tMd3Triangle {
	signed short  vertex[3];    // The vertex for this face (scale down by 64.0f)
	unsigned char normal[2];    // This stores some crazy normal values (not sure...)
};


/// This stores the indices into the vertex and texture coordinate arrays.
struct tMd3Face {
	int vertexIndices[3];
};


/// This stores UV coordinates.
struct tMd3TexCoord {
	float textureCoord[2];
};


/// This stores a skin name (We don't use this, just the name of the model to get the texture).
struct tMd3Skin {
	char strName[68];
};


/// This class handles all of the main .md3 loading code.
class CLoadMD3 {

public:

	// This inits the data members
	CLoadMD3();

	// This is the function that you call to load the MD3 model
	bool ImportMD3( t3DModel *pModel, std::string& strFileName );

	// This loads a model's .skin file
	bool LoadSkin( t3DModel *pModel, const std::string& strSkin, MD3Shape *shape );

	// This loads a weapon's .shader file
	bool LoadShader( t3DModel *pModel, const std::string& strShader, MD3Shape *shape );

private:


	// This reads in the data from the MD3 file and stores it in the member variables,
	// later to be converted to our cool structures so we don't depend on Quake3 stuff.
	void ReadMD3Data( t3DModel *pModel );

	// This converts the member variables to our pModel structure, and takes the model
	// to be loaded and the mesh header to get the mesh info.
	void ConvertDataStructures( t3DModel *pModel, tMd3MeshInfo meshHeader );

	// This frees memory and closes the file
	void CleanUp();

	// Member Variables

	// The file pointer
	FILE *m_FilePointer;

	tMd3Header    m_Header;   // The header data

	tMd3Skin    *m_pSkins;   // The skin name data (not used)
	tMd3TexCoord   *m_pTexCoords;  // The texture coordinates
	tMd3Face    *m_pTriangles;  // Face/Triangle data
	tMd3Triangle   *m_pVertices;  // Vertex/UV indices
	tMd3Bone    *m_pBones;   // This stores the bone data (not used)
};

/// This is our model class that we use to load and draw and free the Quake3 characters.
class CModelMD3 {

public:

	// These our our init and deinit() C++ functions (Constructor/Deconstructor)
	CModelMD3( ModelLoader *loader );
	~CModelMD3();

	void findBounds( vect3d min, vect3d max );
	void normalize( vect3d min, vect3d max );

	bool LoadModel( const std::string& strPath );

	bool loadSkins( const std::string& strPath, const std::string& strModel, MD3Shape *shape );

	// This loads the weapon and takes the same path and model name to be added to .md3
	bool LoadWeapon( const std::string& strPath, const std::string& strModel, MD3Shape *shape  );

	// This links a model to another model (pLink) so that it's the parent of that child.
	// The strTagName is the tag, or joint, that they will be linked at (I.E. "tag_torso").
	void LinkModel( t3DModel *pModel, t3DModel *pLink, char *strTagName );

	// This renders the character to the screen
	void DrawModel( MD3Shape *shape );

	// This frees the character's data
	void DestroyModel( t3DModel *pModel );

	// This takes a string of an animation and sets the torso animation accordingly
	void SetTorsoAnimation( char *strAnimation, bool force, MD3Shape *shape );

	// This takes a string of an animation and sets the legs animation accordingly
	void SetLegsAnimation( char *strAnimation, bool force, MD3Shape *shape );

	// This returns a pointer to a .md3 model in the character (kLower, kUpper, kHead, kWeapon)
	t3DModel *GetModel( int whichPart );

	inline float *getMin() {
		return min;
	}
	inline float *getMax() {
		return max;
	}
	inline void setAnimationPaused( bool b ) {
		paused = b;
	}
	inline ModelLoader *getLoader() {
		return loader;
	}

private:
	vect3d min, max;
	bool paused;
	ModelLoader *loader;

	void clearModel( t3DModel *pModel );
	void findModelBounds( t3DModel *pModel, vect3d min, vect3d max );
	void normalizeModel( t3DModel *pModel, vect3d min, vect3d max );
	void hashAnimations( t3DModel *pModel );

	// This loads the models textures with a given path
	void LoadModelTextures( t3DModel *pModel, const std::string& strPath, MD3Shape *shape );

	// This loads the animation config file (.cfg) for the character
	bool LoadAnimations( const std::string& strConfigFile );

	// This updates the models current frame of animation, and calls SetCurrentTime()
	void UpdateModel( t3DModel *pModel, MD3Shape *shape );

	// This sets the lastTime, t, and the currentFrame of the models animation when needed
	void SetCurrentTime( t3DModel *pModel, MD3Shape *shape );

	// This recursively draws the character models, starting with the lower.md3 model
	void DrawLink( t3DModel *pModel, MD3Shape *shape );

	// This a md3 model to the screen (not the whole character)
	void RenderModel( t3DModel *pModel, MD3Shape *shape );

	// Member Variables

	// These are are models for the character's head and upper and lower body parts
	t3DModel m_Head;
	t3DModel m_Upper;
	t3DModel m_Lower;

	// This store the players weapon model (optional load)
	t3DModel m_Weapon;

	int getAnimationIndex( char *name, t3DModel *pModel );
};


/// This is our quaternion class.
class CQuaternion {

public:

	// This is our default constructor, which initializes everything to an identity
	// quaternion.  An identity quaternion has x, y, z as 0 and w as 1.
	CQuaternion() {
		x = y = z = 0.0f;   w = 1.0f;
	}

	// Creates a constructor that will allow us to initialize the quaternion when creating it
	CQuaternion( float X, float Y, float Z, float W ) {
		x = X;  y = Y;  z = Z;  w = W;
	}

	// This takes in an array of 16 floats to fill in a 4x4 homogeneous matrix from a quaternion
	void CreateMatrix( float *pMatrix );

	// This takes a 3x3 or 4x4 matrix and converts it to a quaternion, depending on rowColumnCount
	void CreateFromMatrix( float *pMatrix, int rowColumnCount );

	// This returns a spherical linear interpolated quaternion between q1 and q2, according to t
	CQuaternion Slerp( CQuaternion &q1, CQuaternion &q2, float t );

private:

	// This stores the 4D values for the quaternion
	float x, y, z, w;
};


#endif


/////////////////////////////////////////////////////////////////////////////////
//
// * QUICK NOTES *
//
// This is a lot of structures/class and code to look over, so take your time and dissect
// each one of them to see where they come into play and what is loaded into them.
// The most important is the CLoadMD3 class and it's definitions.  This is because you
// can't really do it another way since the file format is required to be read in that way.
// Of course, you don't need to use the ConvertDataStructures() function, but I do.
// The rest of the code, like the CModelMD3 class is just my quick implementation to
// get the model loaded, drawn, then freed.
//
// Take a look at the bottom of Md3.cpp for a explanation of what all this stuff does.
// Let me know your comments and suggestion!
//
//
// Ben Humphrey (DigiBen)
// Game Programmer
// DigiBen@GameTutorials.com
// Co-Web Host of www.GameTutorials.com
//
// The Quake3 .Md3 file format is owned by ID Software.  This tutorial is being used
// as a teaching tool to help understand model loading and animation.  This should
// not be sold or used under any way for commercial use with out written consent
// from ID Software.
//
// Quake, Quake2 and Quake3 are trademarks of ID Software.
// Lara Croft is a trademark of Eidos and should not be used for any commercial gain.
// All trademarks used are properties of their respective owners.
//
//
