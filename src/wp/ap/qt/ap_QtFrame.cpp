/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */
/* AbiWord
 * Copyright (C) 2012 Hubert Figuiere
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

#include "ap_App.h"
#include "ap_QtFrame.h"
#include "ap_QtFrameImpl.h"

AP_QtFrame::AP_QtFrame()
	: AP_Frame(new AP_QtFrameImpl(this))
{
}

AP_QtFrame::AP_QtFrame(AP_QtFrame * f)
	: AP_Frame(f)
{
}

AP_QtFrame::~AP_QtFrame()
{
}


// Make this generic and template
XAP_Frame*	AP_QtFrame::cloneFrame(void)
{
	AP_Frame * pClone = new AP_QtFrame(this);
	return static_cast<XAP_Frame *>(pClone);
}

bool AP_QtFrame::initialize(XAP_FrameMode frameMode)
{
#warning TODO implement
}

void AP_QtFrame::setXScrollRange(void)
{
#warning TODO implement
}

void AP_QtFrame::setYScrollRange(void)
{
#warning TODO implement
}

void AP_QtFrame::setStatusMessage(const char * szMsg)
{
#warning TODO implement
}

void AP_QtFrame::toggleTopRuler(bool bRulerOn)
{
#warning TODO implement
}

void AP_QtFrame::toggleLeftRuler(bool bRulerOn)
{
#warning TODO implement
}

bool AP_QtFrame::_createViewGraphics(GR_Graphics *& pG, UT_uint32 iZoom)
{
#warning TODO implement
}

void AP_QtFrame::_bindToolbars(AV_View *pView)
{
#warning TODO implement
}

void AP_QtFrame::_setViewFocus(AV_View *pView)
{
#warning TODO implement
}

bool AP_QtFrame::_createScrollBarListeners(AV_View * pView,
										   AV_ScrollObj *& pScrollObj,
										   ap_ViewListener *& pViewListener,
										   ap_Scrollbar_ViewListener *& pScrollbarViewListener,
										   AV_ListenerId &lid,
										   AV_ListenerId &lidScrollbarViewListener)
{
#warning TODO implement
}

UT_sint32 AP_QtFrame::_getDocumentAreaWidth()
{
#warning TODO implement
}

UT_sint32 AP_QtFrame::_getDocumentAreaHeight()
{
#warning TODO implement
}
