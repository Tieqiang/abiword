/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* AbiSource Application Framework
 * Copyright (C) 1998,1999 AbiSource, Inc.
 * Copyright (C) 2004 Tomas Frydrych <tomasfrydrych@yahoo.co.uk>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#include <stdlib.h>

#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_string.h"
#include "ut_hash.h"
#include "ut_vector.h"
#include "ut_uuid.h"
#include "ut_misc.h"

#include "xad_Document.h"
#include "xap_App.h"
#include "xap_Strings.h"

#ifdef ENABLE_RESOURCE_MANAGER
#include "xap_ResourceManager.h"
#endif

AD_Document::AD_Document() :
#ifdef ENABLE_RESOURCE_MANAGER
	m_pResourceManager(new XAP_ResourceManager),
#else
	m_pResourceManager(0),
#endif
	m_iRefCount(1),
	m_szFilename(NULL),
	m_szEncodingName(""), // Should this have a default? UTF-8, perhaps?
	m_lastSavedTime(0),
	m_lastOpenedTime(time(NULL)),
    m_iEditTime(0),
    m_iVersion(0),
	m_bPieceTableChanging(false),
	m_bHistoryWasSaved(false),
	m_bMarkRevisions(false),
	m_bShowRevisions(true),
	m_iRevisionID(1),
	m_iShowRevisionID(0), // show all
	m_bAutoRevisioning(false),
    m_bForcedDirty(false),
	m_pUUID(NULL)
{	// TODO: clear the ignore list
	

	// create UUID for this doc
	UT_return_if_fail(XAP_App::getApp() && XAP_App::getApp()->getUUIDGenerator());

	m_pUUID = XAP_App::getApp()->getUUIDGenerator()->createUUID();
	UT_return_if_fail(m_pUUID);
	UT_return_if_fail(m_pUUID->isValid());
}

AD_Document::~AD_Document()
{
	UT_ASSERT(m_iRefCount == 0);

   	// NOTE: let subclass clean up m_szFilename, so it matches the alloc mechanism

	// & finally...
#ifdef ENABLE_RESOURCE_MANAGER
	DELETEP(m_pResourceManager);
#endif

	UT_VECTOR_PURGEALL(AD_VersionData*, m_vHistory);
	UT_VECTOR_PURGEALL(AD_Revision*, m_vRevisions);
}

UT_UUID * AD_Document::getNewUUID()const
{
	UT_return_val_if_fail(XAP_App::getApp() && XAP_App::getApp()->getUUIDGenerator(), NULL);

	UT_UUID * pUUID = XAP_App::getApp()->getUUIDGenerator()->createUUID();
	UT_ASSERT( pUUID && pUUID->isValid());
	return pUUID;
}

UT_uint32 AD_Document::getNewUUID32() const
{
	UT_return_val_if_fail(XAP_App::getApp() && XAP_App::getApp()->getUUIDGenerator(),0);
	return XAP_App::getApp()->getUUIDGenerator()->getNewUUID32();
}

UT_uint64 AD_Document::getNewUUID64() const
{
	UT_return_val_if_fail(XAP_App::getApp() && XAP_App::getApp()->getUUIDGenerator(),0);
	return XAP_App::getApp()->getUUIDGenerator()->getNewUUID64();
}


void AD_Document::ref(void)
{
	UT_ASSERT(m_iRefCount > 0);

	m_iRefCount++;
}


void AD_Document::unref(void)
{
	UT_ASSERT(m_iRefCount > 0);

	if (--m_iRefCount == 0)
	{
		delete this;
	}
}


bool AD_Document::isPieceTableChanging(void)
{
        return m_bPieceTableChanging;
}

const char * AD_Document::getFilename(void) const
{
	return m_szFilename;
}

// Document-wide Encoding name used for some file formats (Text, RTF, HTML)

void AD_Document::setEncodingName(const char *szEncodingName)
{
	if (szEncodingName == NULL)
		szEncodingName = "";

	m_szEncodingName = szEncodingName;
}

const char * AD_Document::getEncodingName(void) const
{
	return m_szEncodingName.size() ? m_szEncodingName.c_str() : 0;
}

void AD_Document::purgeHistory()
{
	UT_VECTOR_PURGEALL(AD_VersionData*, m_vHistory);
	m_bHistoryWasSaved = false;
}


/*!
    Add given version data to the document history.
*/
void AD_Document::addRecordToHistory(const AD_VersionData &vd)
{
	AD_VersionData * v = new AD_VersionData(vd);
	UT_return_if_fail(v);
	m_vHistory.addItem((void*)v);
}

/*!
    Get the version number for n-th record in version history
*/
UT_uint32 AD_Document::getHistoryNthId(UT_uint32 i)const
{
	if(!m_vHistory.getItemCount())
		return 0;

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return 0;

	return v->getId();
}

/*!
    Get time stamp for n-th record in version history
    NB: the time stamp represents the last save time
*/
time_t AD_Document::getHistoryNthTime(UT_uint32 i)const
{
	if(!m_vHistory.getItemCount())
		return 0;

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return 0;

	return v->getTime();
}

time_t AD_Document::getHistoryNthTimeStarted(UT_uint32 i)const
{
	if(!m_vHistory.getItemCount())
		return 0;

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return 0;

	return v->getStartTime();
}

bool AD_Document::getHistoryNthAutoRevisioned(UT_uint32 i)const
{
	if(!m_vHistory.getItemCount())
		return 0;

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return false;

	return v->isAutoRevisioned();
}


/*!
    Get get cumulative edit time for n-th record in version history
*/
UT_uint32 AD_Document::getHistoryNthEditTime(UT_uint32 i)const
{
	if(!m_vHistory.getItemCount() || !m_pUUID)
		return 0;

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return 0;

	time_t t0 = v->getStartTime();
	time_t t1 = v->getTime();

	UT_ASSERT( t1 >= t0 );
	return t1-t0;
}

/*!
    Get the UID for n-th record in version history
*/
const UT_UUID & AD_Document::getHistoryNthUID(UT_uint32 i) const
{
	if(!m_vHistory.getItemCount())
		return UT_UUID::getNull();

	AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(i);

	if(!v)
		return UT_UUID::getNull();

	return v->getUID();
}

/*!
    Returns true if both documents are based on the same root document
*/
bool AD_Document::areDocumentsRelated(const AD_Document & d) const
{
	if((!m_pUUID && d.getDocUUID()) || (m_pUUID && !d.getDocUUID()))
		return false;

	return (*m_pUUID == *(d.getDocUUID()));
}

/*!
    Returns true if both documents are based on the same root document
    and all version records have identical UID's
*/
bool AD_Document::areDocumentHistoriesEqual(const AD_Document & d) const
{
	if((!m_pUUID && d.getDocUUID()) || (m_pUUID && !d.getDocUUID()))
		return false;

	if(!(*m_pUUID == *(d.getDocUUID())))
		return false;

	UT_uint32 iHCount = getHistoryCount();
	if(iHCount != d.getHistoryCount())
		return false;

	for(UT_uint32 i = 0; i < iHCount; ++i)
	{
		AD_VersionData * v1 = (AD_VersionData*)m_vHistory.getNthItem(i);
		AD_VersionData * v2 = (AD_VersionData*)d.m_vHistory.getNthItem(i);
	
		if(!(*v1 == *v2))
			return false;
	}
	
	return true;		
}

/*!
    Set UID for the present document
*/
void AD_Document::setDocUUID(const char * s)
{
	if(!m_pUUID)
	{
		UT_return_if_fail(0);
	}
	
	m_pUUID->setUUID(s);
}

/*!
    Get the UID of this document represented as a string (this
    function is primarily for exporters)
*/
const char * AD_Document::getDocUUIDString() const
{
	UT_return_val_if_fail(m_pUUID, NULL);
	static UT_String s;
	m_pUUID->toString(s);
	return s.c_str();
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

UT_uint32 AD_Document::getHighestRevisionId() const
{
	UT_uint32 iId = 0;

	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		iId = UT_MAX(iId, reinterpret_cast<const AD_Revision *>(m_vRevisions.getNthItem(i))->getId());
	}

	return iId;
}

const AD_Revision * AD_Document::getHighestRevision() const
{
	UT_uint32 iId = 0;
	const AD_Revision * r = NULL;

	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		const AD_Revision * t = reinterpret_cast<const AD_Revision *>(m_vRevisions.getNthItem(i));
		UT_uint32 t_id = t->getId();

		if(t_id > iId)
		{
			iId = t_id;
			r = t;
		}
	}

	return r;
}

bool AD_Document::addRevision(UT_uint32 iId, UT_UCS4Char * pDesc, time_t tStart, UT_uint32 iVer)
{
	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		const AD_Revision * r = reinterpret_cast<const AD_Revision*>(m_vRevisions.getNthItem(i));
		if(r->getId() == iId)
			return false;
	}

	AD_Revision * pRev = new AD_Revision(iId, pDesc, tStart, iVer);

	m_vRevisions.addItem(static_cast<void*>(pRev));
	forceDirty();
	m_iRevisionID = iId;
	return true;
}

bool AD_Document::addRevision(UT_uint32 iId,
							  const UT_UCS4Char * pDesc, UT_uint32 iLen,
							  time_t tStart, UT_uint32 iVer)
{
	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		const AD_Revision * r = reinterpret_cast<const AD_Revision*>(m_vRevisions.getNthItem(i));
		if(r->getId() == iId)
			return false;
	}

	UT_UCS4Char * pD = NULL;
	
	if(pDesc)
	{
		pD = new UT_UCS4Char [iLen + 1];
		UT_UCS4_strncpy(pD,pDesc,iLen);
		pD[iLen] = 0;
	}
	
	AD_Revision * pRev = new AD_Revision(iId, pD, tStart, iVer);

	m_vRevisions.addItem(static_cast<void*>(pRev));
	forceDirty();
	m_iRevisionID = iId;
	return true;
}

void AD_Document::_purgeRevisionTable()
{
	UT_VECTOR_PURGEALL(AD_Revision*, m_vRevisions);
	m_vRevisions.clear();
}

void AD_Document::setMarkRevisions(bool bMark)
{
	if(m_bMarkRevisions != bMark)
	{
		m_bMarkRevisions = bMark;
		forceDirty();
	}
}

void AD_Document::toggleMarkRevisions()
{
	setMarkRevisions(!m_bMarkRevisions);
}

void AD_Document::setShowRevisions(bool bShow)
{
	if(m_bShowRevisions != bShow)
	{
		m_bShowRevisions = bShow;
		forceDirty();
	}
}

void AD_Document::toggleShowRevisions()
{
	setShowRevisions(!m_bShowRevisions);
}

void AD_Document::setShowRevisionId(UT_uint32 iId)
{
	if(iId != m_iShowRevisionID)
	{
		m_iShowRevisionID = iId;
		forceDirty();
	}
}

void AD_Document::setRevisionId(UT_uint32 iId)
{
	if(iId != m_iRevisionID)
	{
		m_iRevisionID  = iId;
		// not in this case; this value is not persistent between sessions
		//forceDirty();
	}
}

void AD_Document::setAutoRevisioning(bool b)
{
	if(b != m_bAutoRevisioning)
	{
		// first of all, we will increase the document version
		// number; this will allow us to match autorevision id to a
		// document version number
		time_t t = time(NULL);
		m_iVersion++;
		AD_VersionData v(m_iVersion, t, b);
		addRecordToHistory(v);
		
		m_bAutoRevisioning = b;

		if(b)
		{
			// now create new revision
			const XAP_StringSet * pSS = XAP_App::getApp()->getStringSet();
			UT_return_if_fail(pSS);
			UT_UCS4String ucs4(pSS->getValue(XAP_STRING_ID_MSG_AutoRevision));

			UT_uint32 iId = getRevisionId()+1;
			setRevisionId(iId);
			addRevision(iId, ucs4.ucs4_str(),ucs4.length(),t, m_iVersion);

			// collapse all revisions ...
			setShowRevisionId(0xffffffff);
		}

		setMarkRevisions(b);
	}
}

/*!
   Find revision id that corresponds to given document version
   \return: id > 0 on success or 0 on failure
*/
UT_uint32 AD_Document::findAutoRevisionId(UT_uint32 iVersion) const
{
	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		const AD_Revision *pRev=reinterpret_cast<const AD_Revision*>(m_vRevisions.getNthItem(i));
		UT_return_val_if_fail(pRev, 0);
		
		if(pRev->getVersion() == iVersion)
			return pRev->getId();
	}

	UT_DEBUGMSG(("AD_Document::findAutoRevisionId: autorevision for version %d not found\n",
				 iVersion));

	return 0;
}

/*!
    Finds the nearest autorevision for the given document version
    \param UT_uint32 iVersion: the document version
    \param bool bLesser: indicates whether nearest lesser or nearest
                         greater autorevision is required
    \return: id > 0 on success or 0 on failure
*/
UT_uint32 AD_Document::findNearestAutoRevisionId(UT_uint32 iVersion, bool bLesser) const
{
	UT_uint32 iId = 0;
	
	for(UT_uint32 i = 0; i < m_vRevisions.getItemCount(); i++)
	{
		const AD_Revision *pRev=reinterpret_cast<const AD_Revision*>(m_vRevisions.getNthItem(i));
		UT_return_val_if_fail(pRev, 0);

		if(bLesser)
		{
			if(pRev->getVersion() < iVersion)
				iId = pRev->getId();
			else
				break;
		}
		else
		{
			if(pRev->getVersion() > iVersion)
				return pRev->getId();
		}
	}

#ifdef DEBUG
	if(iId == 0)
	{
		UT_DEBUGMSG(("AD_Document::findNearestAutoRevisionId: not found [ver. %d, bLesser=%d]\n",
					 iVersion, bLesser));
	}
#endif
	return iId;
}


/*!
    Update document history and version information; should only be
    called inside save() and saveAs()
*/
void AD_Document::_adjustHistoryOnSave()
{
	// record this as the last time the document was saved + adjust
	// the cumulative edit time
	m_iVersion++;
	
	if(!m_bHistoryWasSaved || m_bAutoRevisioning)
	{
		// if this is the first save, we will record the time the doc
		// was opened as the start time, otherwise, we will use the
		// current time
		time_t t = !m_bHistoryWasSaved ? m_lastOpenedTime : time(NULL);
		
		AD_VersionData v(m_iVersion,t,m_bAutoRevisioning);
		m_lastSavedTime = v.getTime(); // store the time of this save
		addRecordToHistory(v);

		m_bHistoryWasSaved = true;
	}
	else
	{
		UT_return_if_fail(m_vHistory.getItemCount() > 0);

		// change the edit time of the last entry and create a new UID
		// for the record
		AD_VersionData * v = (AD_VersionData*)m_vHistory.getNthItem(m_vHistory.getItemCount()-1);

		UT_return_if_fail(v);
		v->setId(m_iVersion);
		v->newUID();
		m_lastSavedTime = v->getTime();
	}

	if(m_bAutoRevisioning)
	{
		// create new revision
		const XAP_StringSet * pSS = XAP_App::getApp()->getStringSet();
		UT_return_if_fail(pSS);
		UT_UCS4String ucs4(pSS->getValue(XAP_STRING_ID_MSG_AutoRevision));

		UT_uint32 iId = getRevisionId()+1;
		setRevisionId(iId);
		addRevision(iId, ucs4.ucs4_str(),ucs4.length(),time(NULL), m_iVersion);
	}
}

///////////////////////////////////////////////////
// AD_VersionData
//
// constructor for new entries
AD_VersionData::AD_VersionData(UT_uint32 v, time_t start, bool autorev)
	:m_iId(v),m_pUUID(NULL),m_tStart(start),m_bAutoRevision(autorev)
{
	UT_UUIDGenerator * pGen = XAP_App::getApp()->getUUIDGenerator();
	UT_return_if_fail(pGen);
	
	m_pUUID = pGen->createUUID();
	UT_ASSERT(m_pUUID);
	m_tStart = m_pUUID->getTime();
}


// constructors for importers
AD_VersionData::AD_VersionData(UT_uint32 v, UT_String &uuid, time_t start, bool autorev):
	m_iId(v),m_pUUID(NULL),m_tStart(start),m_bAutoRevision(autorev)
{
	UT_UUIDGenerator * pGen = XAP_App::getApp()->getUUIDGenerator();
	UT_return_if_fail(pGen);
	
	m_pUUID = pGen->createUUID(uuid);
	UT_ASSERT(m_pUUID);
};

AD_VersionData::AD_VersionData(UT_uint32 v, const char *uuid, time_t start, bool autorev):
	m_iId(v),m_pUUID(NULL),m_tStart(start),m_bAutoRevision(autorev)
{
	UT_UUIDGenerator * pGen = XAP_App::getApp()->getUUIDGenerator();
	UT_return_if_fail(pGen);
	
	m_pUUID = pGen->createUUID(uuid);
	UT_ASSERT(m_pUUID);
};

// copy constructor
AD_VersionData::AD_VersionData(const AD_VersionData & v):
	m_iId(v.m_iId), m_pUUID(NULL), m_bAutoRevision(v.m_bAutoRevision)
{
	UT_return_if_fail(v.m_pUUID);
	UT_UUIDGenerator * pGen = XAP_App::getApp()->getUUIDGenerator();
	UT_return_if_fail(pGen);
	
	m_pUUID = pGen->createUUID(*(v.m_pUUID));
	UT_ASSERT(m_pUUID);

	m_tStart = v.m_tStart;
};

AD_VersionData & AD_VersionData::operator = (const AD_VersionData &v)
{
	m_iId       = v.m_iId;
	*m_pUUID    = *(v.m_pUUID);
	m_tStart    = v.m_tStart;
	m_bAutoRevision = v.m_bAutoRevision;
	return *this;
}

bool AD_VersionData::operator == (const AD_VersionData &v)
{
	return (m_iId == v.m_iId && m_tStart == v.m_tStart
			&& *m_pUUID == *(v.m_pUUID) && m_bAutoRevision == v.m_bAutoRevision);
}

AD_VersionData::~AD_VersionData()
{
	if(m_pUUID)
		delete m_pUUID;
}

time_t AD_VersionData::getTime()const
{
	if(!m_pUUID)
		return 0;

	return m_pUUID->getTime();
}

bool AD_VersionData::newUID()
{
	if(!m_pUUID)
		return false;

	return m_pUUID->makeUUID();
}


