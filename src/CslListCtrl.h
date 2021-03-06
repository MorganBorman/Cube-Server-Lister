/***************************************************************************
 *   Copyright (C) 2007-2009 by Glen Masgai                                *
 *   mimosius@users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CSLLISTCTRL_H
#define CSLLISTCTRL_H

/**
    @author Glen Masgai <mimosius@users.sourceforge.net>
*/

#include "CslToolTip.h"

// since wx>2.8.4 the list control items are getting deselected on wxGZK
// this happens too on wxMAC (maybe because of the generic list control?)
#if defined(__WXGTK__) || defined(__WXMAC__)
#ifndef CSL_USE_WX_LIST_DESELECT_WORKAROUND
#if wxVERSION_NUMBER > 2804 || defined(__WXMAC__)
#define CSL_USE_WX_LIST_DESELECT_WORKAROUND
#endif
#endif
#endif


enum
{
    CSL_LIST_IMG_SORT_ASC = 0,
    CSL_LIST_IMG_SORT_DSC,

    CSL_LIST_IMG_FLAG_START,
};


class CslListSortHelper
{
    public:
        enum { SORT_ASC = 0, SORT_DSC };

        void Init(wxUint32 mode,wxUint32 type)
        {
            Mode=mode;
            Type=type;
        }

        template<typename _T>
        wxInt32 Cmp(const _T& first,const _T& second,wxInt32 mode=-1) const
        {
            wxInt32 m=mode<0 ? Mode:mode;

        return first<second ? m==SORT_ASC ? -1:1 :
                   first==second ? 0 : m==SORT_ASC ? 1:-1;
        }

        wxInt32 Mode;
        wxInt32 Type;
};


class CslListCtrl : public wxListCtrl
{
    public:
        CslListCtrl(wxWindow* parent,wxWindowID id,const wxPoint& pos=wxDefaultPosition,
                    const wxSize& size=wxDefaultSize,long style=wxLC_ICON,
                    const wxValidator& validator=wxDefaultValidator,
                    const wxString& name=wxListCtrlNameStr);
        ~CslListCtrl();

        void CreateScreenShot();
        static void CreateCountryFlagImageList();

    private:
        wxUint32 m_mouseLastMove;

        bool m_flickerFree;

#ifdef __WXMSW__
        void OnEraseBackground(wxEraseEvent& event);
#endif
        void OnMouseMove(wxMouseEvent& event);
        void OnMenu(wxCommandEvent& event);
        void OnItem(wxListEvent& event);
        void OnContextMenu(wxContextMenuEvent& event);
        void OnToolTip(CslToolTipEvent& event);

        DECLARE_EVENT_TABLE()

    protected:
        static wxImageList ListImageList;

        void FlickerFree(bool val) { m_flickerFree=val; }
        wxUint32 GetCountryFlag(wxUint32 ip,wxUint32 start=CSL_LIST_IMG_FLAG_START);

        virtual wxWindow* GetScreenShotWindow() { return this; }
        virtual wxString GetScreenShotFileName();
        virtual void GetToolTipText(wxInt32 row,CslToolTipEvent& event) {}
        virtual wxSize GetImageListSize() { return wxDefaultSize; }
};

#endif //CSLLISTCTRL_H
