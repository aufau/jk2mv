#include "RoffSystem.h"
#include "qcommon.h"
#include "../client/client.h"
#include "../server/server.h"

// The one and only instance...
CROFFSystem theROFFSystem;

//---------------------------------------------------------------------------
// CROFFSystem::CROFF::CROFF
//	Simple constructor for CROFF object
//
// INPUTS:
//	pass in the filepath and the id of the roff object to create
//
// RETURN:
//	none
//---------------------------------------------------------------------------
CROFFSystem::CROFF::CROFF( const char *file, int id )
{
	Q_strncpyz( mROFFFilePath, file, sizeof(mROFFFilePath) );

	mID				= id;
	mMoveRotateList = NULL;
	mNoteTrackIndexes = 0;
	mUsedByClient = mUsedByServer = qfalse;
}


//---------------------------------------------------------------------------
// CROFFSystem::CROFF::~CROFF()
//	Frees any resources when the CROFF object dies
//
// INPUTS:
//	none
//
// RETURN:
//	none
//---------------------------------------------------------------------------
CROFFSystem::CROFF::~CROFF()
{
	if ( mMoveRotateList )
	{
		delete [] mMoveRotateList;
	}

	if (mNoteTrackIndexes)
	{
		delete mNoteTrackIndexes[0];
		delete [] mNoteTrackIndexes;
	}
}


//---------------------------------------------------------------------------
// CROFFSystem::Restart
//	Cleans up the roff system, not sure how useful this really is
//
// INPUTS:
//	none
//
// RETURN:
//	success or failure
//---------------------------------------------------------------------------
qboolean CROFFSystem::Restart()
{
	TROFFList::iterator itr = mROFFList.begin();

	// remove everything from the list
	while( itr != mROFFList.end() )
	{
		delete (*itr).second;

		mROFFList.erase( itr );
		itr = mROFFList.begin();
	}

	// clear CROFFSystem unique ID counter
	mID = 0;

	return qtrue;
}


//---------------------------------------------------------------------------
// CROFFSystem::IsROFF
//	Makes sure that the requested file is actually a ROFF
//
// INPUTS:
//	pass in the file data
//
// RETURN:
//	returns test success or failure
//---------------------------------------------------------------------------
qboolean CROFFSystem::IsROFF( unsigned char *data )
{
	TROFFHeader		*hdr = (TROFFHeader *)data;
	TROFF2Header	*hdr2 = (TROFF2Header *)data;

	if ( !strcmp( hdr->mHeader, ROFF_STRING ))
	{ // bad header
		return qfalse;
	}

	if (hdr->mVersion != ROFF_VERSION && hdr->mVersion != ROFF_NEW_VERSION)
	{	// bad version
		return qfalse;
	}

	if (hdr->mVersion == ROFF_VERSION && hdr->mCount <= 0.0f)
	{	// bad count
		return qfalse;
	}

	if (hdr->mVersion == ROFF_NEW_VERSION && hdr2->mCount <= 0)
	{	// bad count
		return qfalse;
	}

	return qtrue;
}


//---------------------------------------------------------------------------
// CROFFSystem::InitROFF
//	Handles stuffing the roff data in the CROFF object
//
// INPUTS:
//	pass in the file data and the object to stuff the data into.
//
// RETURN:
//	returns initialization success or failure
//---------------------------------------------------------------------------
qboolean CROFFSystem::InitROFF( unsigned char *data, CROFF *obj )
{
	int	i;

	TROFFHeader *hdr = (TROFFHeader *)data;

	if (hdr->mVersion == ROFF_NEW_VERSION)
	{
		return InitROFF2(data, obj);
	}

	obj->mROFFEntries		= hdr->mCount;
	obj->mMoveRotateList	= new TROFF2Entry[((int)hdr->mCount)];
	obj->mFrameTime			= 1000 / ROFF_SAMPLE_RATE;		// default 10 hz
	obj->mLerp				= ROFF_SAMPLE_RATE;
	obj->mNumNoteTracks		= 0;
	obj->mNoteTrackIndexes	= 0;

	if ( obj->mMoveRotateList != 0 )
	{ // Step past the header to get to the goods
		TROFFEntry *roff_data = ( TROFFEntry *)&hdr[1];

		// Copy all of the goods into our ROFF cache
		for ( i = 0; i < (int)hdr->mCount; i++ )
		{
			VectorCopy( roff_data[i].mOriginOffset, obj->mMoveRotateList[i].mOriginOffset );
			VectorCopy( roff_data[i].mRotateOffset, obj->mMoveRotateList[i].mRotateOffset );
			obj->mMoveRotateList[i].mStartNote = -1;
			obj->mMoveRotateList[i].mNumNotes = 0;
		}

		FixBadAngles(obj);
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}


//---------------------------------------------------------------------------
// CROFFSystem::InitROFF2
//	Handles stuffing the roff data in the CROFF object for version 2
//
// INPUTS:
//	pass in the file data and the object to stuff the data into.
//
// RETURN:
//	returns initialization success or failure
//---------------------------------------------------------------------------
qboolean CROFFSystem::InitROFF2( unsigned char *data, CROFF *obj )
{
	int	i;

	TROFF2Header *hdr = (TROFF2Header *)data;

	obj->mROFFEntries		= hdr->mCount;
	obj->mMoveRotateList	= new TROFF2Entry[(hdr->mCount)];
	obj->mFrameTime			= hdr->mFrameRate;
	obj->mLerp				= 1000 / hdr->mFrameRate;
	obj->mNumNoteTracks		= hdr->mNumNotes;

	if ( obj->mMoveRotateList != 0 )
	{ // Step past the header to get to the goods
		TROFF2Entry *roff_data = ( TROFF2Entry *)&hdr[1];

		// Copy all of the goods into our ROFF cache
		for ( i = 0; i < (int)hdr->mCount; i++ )
		{
			VectorCopy( roff_data[i].mOriginOffset, obj->mMoveRotateList[i].mOriginOffset );
			VectorCopy( roff_data[i].mRotateOffset, obj->mMoveRotateList[i].mRotateOffset );
			obj->mMoveRotateList[i].mStartNote = roff_data[i].mStartNote;
			obj->mMoveRotateList[i].mNumNotes = roff_data[i].mNumNotes;
		}

		FixBadAngles(obj);

		if (obj->mNumNoteTracks)
		{
			size_t		size;
			char	*ptr, *start;

			ptr = start = (char *)&roff_data[i];
			size = 0;

			for(i=0;i<obj->mNumNoteTracks;i++)
			{
				size += strlen(ptr) + 1;
				ptr += strlen(ptr) + 1;
			}

			obj->mNoteTrackIndexes = new char *[obj->mNumNoteTracks];
			ptr = obj->mNoteTrackIndexes[0] = new char[size];
			memcpy(obj->mNoteTrackIndexes[0], start, size);

			for(i=1;i<obj->mNumNoteTracks;i++)
			{
				ptr += strlen(ptr) + 1;
				obj->mNoteTrackIndexes[i] = ptr;
			}
		}
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

/************************************************************************************************
 * CROFFSystem::FixBadAngles																	*
 *	This function will attempt to fix bad angles (large) that come in from the exporter.	  *
 *																							  *
 * Input																						*
 *	obj: the ROFF object																	  *
 *																							  *
 * Output / Return																			  *
 *	none																					  *
 *																							  *
 ************************************************************************************************/
void CROFFSystem::FixBadAngles(CROFF *obj)
{
// Ideally we would fix the ROFF exporter, if that doesn't happen, this may be an adequate solution
#ifdef ROFF_AUTO_FIX_BAD_ANGLES
	int		index, t;

	// Attempt to fix bad angles

	for(index=0;index<obj->mROFFEntries;index++)
	{
		for ( t = 0; t < 3; t++ )
		{
			if ( obj->mMoveRotateList[index].mRotateOffset[t] > 180.0f )
			{ // found a bad angle
			//	Com_Printf( S_COLOR_YELLOW"Fixing bad roff angle\n <%6.2f> changed to <%6.2f>.\n",
			//				roff_data[i].mRotateOffset[t], roff_data[i].mRotateOffset[t] - 360.0f );
				obj->mMoveRotateList[index].mRotateOffset[t] -= 360.0f;
			}
			else if ( obj->mMoveRotateList[index].mRotateOffset[t] < -180.0f )
			{ // found a bad angle
			//	Com_Printf( S_COLOR_YELLOW"Fixing bad roff angle\n <%6.2f> changed to <%6.2f>.\n",
			//				roff_data[i].mRotateOffset[t], roff_data[i].mRotateOffset[t] + 360.0f );
				obj->mMoveRotateList[index].mRotateOffset[t] += 360.0f;
			}
		}
	}
#endif // ROFF_AUTO_FIX_BAD_ANGLES
}

//---------------------------------------------------------------------------
// CROFFSystem::Cache
//	Pre-caches roff data to avoid file hits during gameplay.  Disallows
//		repeated caches of existing roffs.
//
// INPUTS:
//	pass in the filepath of the roff to cache
//
// RETURN:
//	returns ID of the roff, whether its an existing one or new one.
//---------------------------------------------------------------------------
int CROFFSystem::Cache( const char *file, qboolean isClient )
{
	// See if this item is already cached
	int				len;
	int				id = GetID( file );
	unsigned char	*data;
	CROFF			*cROFF;

	if ( id )
	{
#ifdef DEBUG
		Com_Printf( S_COLOR_YELLOW"Ignoring. File '%s' already cached.\n", file );
#endif
	}
	else
	{ // Read the file in one fell swoop
		len = FS_ReadFile( file, (void**) &data);

		if ( len <= 0 )
		{
			Com_Printf( S_COLOR_RED"Could not open .ROF file '%s'\n", file );
			return 0;
		}

		// Make sure that the file is roff
		if ( !IsROFF( data ) )
		{
			Com_Printf( S_COLOR_RED"cache failed: roff <%s> does not exist or is not a valid roff\n", file );
			FS_FreeFile( data );

			return 0;
		}

		// Things are looking good so far, so create a new CROFF object
		id = NewID();

		cROFF = new CROFF( file, id );

		mROFFList[id] = cROFF;

		if ( !InitROFF( data, cROFF ) )
		{ // something failed, so get rid of the object
			Unload( id );
			id = 0;
		}

		FS_FreeFile( data );
	}

	cROFF = (*mROFFList.find( id )).second;
	if (isClient)
	{
		cROFF->mUsedByClient = qtrue;
	}
	else
	{
		cROFF->mUsedByServer = qtrue;
	}

	// If we haven't requested a new ID, we'll just be returning the ID of the existing roff
	return id;
}


//---------------------------------------------------------------------------
// CROFFSystem::GetID
//	Finds the associated (internal) ID of the specified roff file
//
// INPUTS:
//	pass in the roff file path
//
// RETURN:
//	returns ID if there is one, zero if nothing was found
//---------------------------------------------------------------------------
int	CROFFSystem::GetID( const char *file )
{
	TROFFList::iterator itr;

	// Attempt to find the requested roff
	for ( itr = mROFFList.begin(); itr != mROFFList.end(); ++itr )
	{
		if ( !strcmp( (*itr).second->mROFFFilePath, file ) )
		{ // return the ID to this roff
			return (*itr).first;
		}
	}

	// Not found
	return 0;
}


//---------------------------------------------------------------------------
// CROFFSystem::Unload
//	Removes the roff from the list, deleting it to free up any used resources
//
// INPUTS:
//	pass in the id of the roff to delete, use GetID if you only know the roff
//		filepath
//
// RETURN:
//	qtrue if item was in the list, qfalse otherwise
//---------------------------------------------------------------------------
qboolean CROFFSystem::Unload( int id )
{
	TROFFList::iterator itr;

	itr = mROFFList.find( id );

	if ( itr != mROFFList.end() )
	{ // requested item found in the list, free mem, then remove from list
		delete (*itr).second;

		itr = mROFFList.erase( itr );

#ifdef DEBUG
		Com_Printf( S_COLOR_GREEN"roff unloaded\n" );
#endif

		return qtrue;
	}
	else
	{ // not found

#ifdef DEBUG
		Com_Printf( S_COLOR_RED"unload failed: roff <%i> does not exist\n", id );
#endif
		return qfalse;
	}
}

//---------------------------------------------------------------------------
// CROFFSystem::Clean
//	Cleans out all Roffs, freeing up any used resources
//
// INPUTS:
//	none
//
// RETURN:
//	success of operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::Clean(qboolean isClient)
{
	TROFFList::iterator			itr, next;
	TROFFEntList::iterator		entI, nextEnt;

	itr = mROFFList.begin();
	while ( itr != mROFFList.end() )
	{
		next = itr;
		next++;

		if (isClient)
		{
			(*itr).second->mUsedByClient = qfalse;
		}
		else
		{
			(*itr).second->mUsedByServer = qfalse;
		}
		if ((*itr).second->mUsedByClient == qfalse && (*itr).second->mUsedByServer == qfalse)
		{	// we are not used on both client and server, so unload
			Unload( (*itr).first );
		}

		itr = next;
	}

	entI = mROFFEntList.begin();
	while ( entI != mROFFEntList.end() )
	{
		nextEnt = entI;
		nextEnt++;

		if ((*entI)->mIsClient == isClient)
		{
			delete (*entI);
			mROFFEntList.erase( entI );
		}

		entI = nextEnt;
	}

	return qtrue;
}

//---------------------------------------------------------------------------
// CROFFSystem::List
//	Dumps the file path to the current set of cached roffs, for debug purposes
//
// INPUTS:
//	none
//
// RETURN:
//	none
//---------------------------------------------------------------------------
void CROFFSystem::List()
{
	TROFFList::iterator itr;

	Com_Printf( S_COLOR_GREEN"\n--Cached ROFF files--\n" );
	Com_Printf( S_COLOR_GREEN"ID   FILE\n" );

	for ( itr = mROFFList.begin(); itr != mROFFList.end(); ++itr )
	{
		Com_Printf( S_COLOR_GREEN"%2i - %s\n", (*itr).first, (*itr).second->mROFFFilePath );
	}

	Com_Printf( S_COLOR_GREEN"\nFiles: %i\n", (int)mROFFList.size() );
}


//---------------------------------------------------------------------------
// CROFFSystem::List
//	Overloaded version of List, dumps the specified roff data to the console
//
// INPUTS:
//	id of roff to display
//
// RETURN:
//	success or failure of operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::List( int id )
{
	TROFFList::iterator itr;

	itr = mROFFList.find( id );

	if ( itr != mROFFList.end() )
	{ // requested item found in the list
		CROFF		*obj = (*itr).second;
		TROFF2Entry	*dat = obj->mMoveRotateList;

		Com_Printf( S_COLOR_GREEN"File: %s\n", obj->mROFFFilePath );
		Com_Printf( S_COLOR_GREEN"ID: %i\n", id );
		Com_Printf( S_COLOR_GREEN"Entries: %i\n\n", obj->mROFFEntries );

		Com_Printf( S_COLOR_GREEN"MOVE                 ROTATE\n" );

		for ( int i = 0; i < obj->mROFFEntries; i++ )
		{
			Com_Printf( S_COLOR_GREEN"%6.2f %6.2f %6.2f   %6.2f %6.2f %6.2f\n",
						dat[i].mOriginOffset[0], dat[i].mOriginOffset[1], dat[i].mOriginOffset[2],
						dat[i].mRotateOffset[0], dat[i].mRotateOffset[1], dat[i].mRotateOffset[2] );
		}

		return qtrue;
	}

	Com_Printf( S_COLOR_YELLOW"ROFF not found: id <%d>\n", id );

	return qfalse;
}


//---------------------------------------------------------------------------
// CROFFSystem::Play
//	Start roff playback on an entity
//
// INPUTS:
//	the id of the entity that will be roffed
//	the id of the roff to play
//
// RETURN:
//	success or failure of add operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::Play( int entID, int id, qboolean doTranslation, qboolean isClient )
{
	sharedEntity_t *ent = SV_GentityNum( entID );

	ent->r.mIsRoffing = qtrue;
/*rjr	if(ent->GetPhysics() == PHYSICS_TYPE_NONE)
	{
		ent->SetPhysics(PHYSICS_TYPE_BRUSHMODEL);
	}*/
	//bjg TODO: reset this latter?

	if ( ent == 0 )
	{ // shame on you..
		return qfalse;
	}

	SROFFEntity *roffing_ent = new SROFFEntity;

	roffing_ent->mEntID			= entID;
	roffing_ent->mROFFID		= id;
	roffing_ent->mNextROFFTime	= sv.time;
	roffing_ent->mROFFFrame		= 0;
	roffing_ent->mKill			= qfalse;
	roffing_ent->mSignal		= qtrue; // TODO: hook up the real signal code
	roffing_ent->mTranslated	= doTranslation;
	roffing_ent->mIsClient		= isClient;

	VectorCopy(ent->s.apos.trBase, roffing_ent->mStartAngles);

	mROFFEntList.push_back( roffing_ent );

	return qtrue;
}


//---------------------------------------------------------------------------
// CROFFSystem::ListEnts
//	List all of the ents in the roff system
//
// INPUTS:
//	none
//
// RETURN:
//	none
//---------------------------------------------------------------------------
void CROFFSystem::ListEnts()
{
/*	char	*name, *file;
	int		id;

	TROFFEntList::iterator itr = mROFFEntList.begin();
	TROFFList::iterator itrRoff;

	Com_Printf( S_COLOR_GREEN"\n--ROFFing Entities--\n" );
	Com_Printf( S_COLOR_GREEN"EntID EntName       RoffFile\n" );

	// display everything in the end list
	for ( itr = mROFFEntList.begin(); itr != mROFFEntList.end(); ++itr )
	{
		// Entity ID
		id = (*itr)->mEntID;
		// Entity Name
		name = entitySystem->GetEntityFromID( id )->GetName();
		// ROFF object that will contain the roff file name
		itrRoff = mROFFList.find( (*itr)->mROFFID );

		if ( itrRoff != mROFFList.end() )
		{ // grab our filename
			file = (*itrRoff).second->mROFFFilePath;
		}
		else
		{ // roff filename not found == bad
			file = "Error:  Unknown";
		}

		Com_Printf( S_COLOR_GREEN"%3i  %s    %s\n", id, name, file );
	}

	Com_Printf( S_COLOR_GREEN"\nEntities: %i\n", mROFFEntList.size() );*/
}


//---------------------------------------------------------------------------
// CROFFSystem::PurgeEnt
//	Prematurely purge an entity from the roff system
//
// INPUTS:
//	the id of the entity to purge
//
// RETURN:
//	success or failure of purge operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::PurgeEnt( int entID, qboolean isClient )
{
	TROFFEntList::iterator itr = mROFFEntList.begin();

	for ( itr = mROFFEntList.begin(); itr != mROFFEntList.end(); ++itr )
	{
		if ( (*itr)->mIsClient == isClient && (*itr)->mEntID == entID)
		{
			// Make sure it won't stay lerping
			ClearLerp( (*itr) );

			delete (*itr);

			mROFFEntList.erase( itr );
			return qtrue;
		}
	}

	Com_Printf( S_COLOR_RED"Purge failed:  Entity <%i> not found\n", entID );

	return qfalse;
}



//---------------------------------------------------------------------------
// CROFFSystem::PurgeEnt
//	Prematurely purge an entity from the roff system
//
// INPUTS:
//	the name fo the entity to purge
//
// RETURN:
//	success or failure of purge operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::PurgeEnt( char *name )
{
/* rjr	CEntity *ent = entitySystem->GetEntityFromName( NULL, name );

	if ( ent && ent->GetInUse() == qtrue )
	{
		return PurgeEnt( ent->GetID() );
	}
	else
	{
		Com_Printf( S_COLOR_RED"Entity <%s> not found or not in use\n", name );
		return qfalse;
	}*/

	return qfalse;
}

//---------------------------------------------------------------------------
// CROFFSystem::UpdateEntities
//	Update all of the entities in the system
//
// INPUTS:
//	none
//
// RETURN:
//	none
//---------------------------------------------------------------------------
void CROFFSystem::UpdateEntities(qboolean isClient)
{
	TROFFEntList::iterator itr = mROFFEntList.begin();
	TROFFList::iterator itrRoff;

	// display everything in the entity list
	for ( itr = mROFFEntList.begin(); itr != mROFFEntList.end(); ++itr )
	{
		if ((*itr)->mIsClient != isClient)
		{
			continue;
		}

		// Get this entities ROFF object
		itrRoff = mROFFList.find( (*itr)->mROFFID );

		if ( itrRoff != mROFFList.end() )
		{ // roff that baby!
			if ( !ApplyROFF( *itr, (*itrRoff).second ) )
			{ // done roffing, mark for death
				(*itr)->mKill = qtrue;
			}
		}
		else
		{ // roff not found == bad, dump an error message and purge this ent
			Com_Printf( S_COLOR_RED"ROFF System Error:\n" );
//			Com_Printf( S_COLOR_RED" -ROFF not found for entity <%s>\n",
//					entitySystem->GetEntityFromID((*itr)->mEntID)->GetName() );

			(*itr)->mKill = qtrue;

			ClearLerp( (*itr) );
		}
	}

	itr = mROFFEntList.begin();

	// Delete killed ROFFers from the list
	// Man, there just has to be a better way to do this
	while ( itr != mROFFEntList.end() )
	{
		if ((*itr)->mIsClient != isClient)
		{
			itr++;
			continue;
		}

		if ( (*itr)->mKill == qtrue )
		{
			//make sure ICARUS knows ROFF is stopped
//			CICARUSGameInterface::TaskIDComplete(
//				entitySystem->GetEntityFromID((*itr)->mEntID, TID_MOVE);
			// trash this guy from the list
			delete (*itr);
			mROFFEntList.erase( itr );
			itr = mROFFEntList.begin();
		}
		else
		{
			itr++;
		}
	}
}

//---------------------------------------------------------------------------
// CROFFSystem::ApplyROFF
//	Does the dirty work of applying the raw ROFF data
//
// INPUTS:
//	The the roff_entity struct and the raw roff data
//
// RETURN:
//	True == success;  False == roff playback complete or failure
//---------------------------------------------------------------------------
qboolean CROFFSystem::ApplyROFF( SROFFEntity *roff_ent, CROFFSystem::CROFF *roff )
{
	vec3_t			f, r, u, result;
	sharedEntity_t	*ent = NULL;
	trajectory_t	*originTrajectory, *angleTrajectory;
	vec_t			*origin, *angle;


	if ( sv.time < roff_ent->mNextROFFTime )
	{ // Not time to roff yet
		return qtrue;
	}

#ifndef DEDICATED
	// always false on dedicated
	if (roff_ent->mIsClient)
	{
		vec3_t		originTemp, angleTemp;
		originTrajectory = (trajectory_t *)VM_Call( cgvm, CG_GET_ORIGIN_TRAJECTORY, roff_ent->mEntID );
		angleTrajectory = (trajectory_t *)VM_Call( cgvm, CG_GET_ANGLE_TRAJECTORY, roff_ent->mEntID );
		VM_Call( cgvm, CG_GET_ORIGIN, roff_ent->mEntID, originTemp );
		origin = originTemp;
		VM_Call( cgvm, CG_GET_ANGLES, roff_ent->mEntID, angleTemp );
		angle = angleTemp;
	}
	else
#endif
	{
		// Find the entity to apply the roff to
		ent = SV_GentityNum( roff_ent->mEntID );

		if ( ent == 0 )
		{	// bad stuff
			return qfalse;
		}

		originTrajectory = &ent->s.pos;
		angleTrajectory = &ent->s.apos;
		origin = ent->r.currentOrigin;
		angle = ent->r.currentAngles;
	}


	if ( roff_ent->mROFFFrame >= roff->mROFFEntries )
	{ // we are done roffing, so stop moving and flag this ent to be removed
		SetLerp( originTrajectory, TR_STATIONARY, origin, NULL, sv.time, roff->mLerp );
		SetLerp( angleTrajectory, TR_STATIONARY, angle, NULL, sv.time, roff->mLerp );
		if (!roff_ent->mIsClient)
		{
			ent->r.mIsRoffing = qfalse;
		}
		return qfalse;
	}

	if (roff_ent->mTranslated)
	{
		AngleVectors(roff_ent->mStartAngles, f, r, u );
		VectorScale(f, roff->mMoveRotateList[roff_ent->mROFFFrame].mOriginOffset[0], result);
		VectorMA(result, -roff->mMoveRotateList[roff_ent->mROFFFrame].mOriginOffset[1], r, result);
		VectorMA(result, roff->mMoveRotateList[roff_ent->mROFFFrame].mOriginOffset[2], u, result);
	}
	else
	{
		VectorCopy(roff->mMoveRotateList[roff_ent->mROFFFrame].mOriginOffset, result);
	}

	// Set up our origin interpolation
	SetLerp( originTrajectory, TR_LINEAR, origin, result, sv.time, roff->mLerp );

	// Set up our angle interpolation
	SetLerp( angleTrajectory, TR_LINEAR, angle,
				roff->mMoveRotateList[roff_ent->mROFFFrame].mRotateOffset, sv.time, roff->mLerp );

	if (roff->mMoveRotateList[roff_ent->mROFFFrame].mStartNote >= 0)
	{
		int		i;

		for(i=0;i<roff->mMoveRotateList[roff_ent->mROFFFrame].mNumNotes;i++)
		{
			ProcessNote(roff_ent, roff->mNoteTrackIndexes[roff->mMoveRotateList[roff_ent->mROFFFrame].mStartNote + i]);
		}
	}

	// Advance ROFF frames and lock to a 10hz cycle
	roff_ent->mROFFFrame++;
	roff_ent->mNextROFFTime = sv.time + roff->mFrameTime;

	return qtrue;
}


/************************************************************************************************
 * CROFFSystem::ProcessNote																	 *
 *	This function will send the note to the client.  It will parse through the note for	   *
 *	leading or trailing white space (thus making each line feed a separate function call).	*
 *																							  *
 * Input																						*
 *	ent: the entity for which the roff is being played										*
 *	note: the note that should be passed on												   *
 *																							  *
 * Output / Return																			  *
 *	none																					  *
 *																							  *
 ************************************************************************************************/
void CROFFSystem::ProcessNote(SROFFEntity *roff_ent, char *note)
{
	char	temp[1024];
	int		pos, size;

	pos = 0;
	while(note[pos])
	{
		size = 0;
		while(note[pos] && note[pos] < ' ')
		{
			pos++;
		}

		while(note[pos] && note[pos] >= ' ')
		{
			temp[size++] = note[pos++];
		}
		temp[size] = 0;

		if (size)
		{
			if (roff_ent->mIsClient)
			{
#ifndef DEDICATED
				VM_Call( cgvm, CG_ROFF_NOTETRACK_CALLBACK, roff_ent->mEntID, temp );
#endif
			}
			else
			{
				VM_Call( gvm, GAME_ROFF_NOTETRACK_CALLBACK, roff_ent->mEntID, temp );
			}
		}
	}
}

//---------------------------------------------------------------------------
// CROFFSystem::ClearLerp
//	Helper function to clear a given entities lerp fields
//
// INPUTS:
//	The ID of the entity to clear
//
// RETURN:
//	success or failure of the operation
//---------------------------------------------------------------------------
qboolean CROFFSystem::ClearLerp( SROFFEntity *roff_ent )
{
	sharedEntity_t	*ent;
	trajectory_t	*originTrajectory, *angleTrajectory;
	vec_t			*origin, *angle;

#ifndef DEDICATED
	vec3_t			originTemp, angleTemp;
	// always false on dedicated
	if (roff_ent->mIsClient)
	{
		originTrajectory = (trajectory_t *)VM_Call( cgvm, CG_GET_ORIGIN_TRAJECTORY, roff_ent->mEntID );
		angleTrajectory = (trajectory_t *)VM_Call( cgvm, CG_GET_ANGLE_TRAJECTORY, roff_ent->mEntID );
		VM_Call( cgvm, CG_GET_ORIGIN, roff_ent->mEntID, originTemp );
		origin = originTemp;
		VM_Call( cgvm, CG_GET_ANGLES, roff_ent->mEntID, angleTemp );
		angle = angleTemp;
	}
	else
#endif
	{
		// Find the entity to apply the roff to
		ent = SV_GentityNum( roff_ent->mEntID );

		if ( ent == 0 )
		{	// bad stuff
			return qfalse;
		}

		originTrajectory = &ent->s.pos;
		angleTrajectory = &ent->s.apos;
		origin = ent->r.currentOrigin;
		angle = ent->r.currentAngles;
	}

	SetLerp( originTrajectory, TR_STATIONARY, origin, NULL, sv.time, ROFF_SAMPLE_RATE );
	SetLerp( angleTrajectory, TR_STATIONARY, angle, NULL, sv.time, ROFF_SAMPLE_RATE );

	return qtrue;
}

//---------------------------------------------------------------------------
// CROFFSystem::SetLerp
//	Helper function to set up a positional or angular interpolation
//
// INPUTS:
//	The entity trajectory field to modify, the interpolation type, the base origin,
//		and the interpolation start time
//
// RETURN:
//	none
//---------------------------------------------------------------------------
void CROFFSystem::SetLerp( trajectory_t *tr, trType_t type, vec3_t origin, vec3_t delta, int time, int rate)
{
	tr->trType = type;
	tr->trTime = time;
	VectorCopy( origin, tr->trBase );

	// Check for a NULL delta
	if ( delta )
	{
		VectorScale( delta, rate, tr->trDelta );
	}
	else
	{
		VectorClear( tr->trDelta );
	}
}

