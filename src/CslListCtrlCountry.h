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

#ifndef CSLLISTCTRLCOUNTRY_H
#define CSLLISTCTRLCOUNTRY_H

/**
    @author Glen Masgai <mimosius@users.sourceforge.net>
*/

#include "CslListCtrl.h"
#include "engine/CslTools.h"
#include "engine/CslGame.h"

class CslListCtrlCountry;


class CslCountryEntry
{
    public:
        CslCountryEntry(const wxString& country) : Country(country),Count(1) {}

        wxString Country;
        wxUint32 Count;
};

WX_DEFINE_ARRAY_PTR(CslCountryEntry*,t_aCslCountryEntry);


class CslPanelCountry : public wxPanel
{
    public:
        enum { MODE_SERVER, MODE_PLAYER_SINGLE, MODE_PLAYER_MULTI };

        CslPanelCountry(wxWindow* parent,long listStyle=wxLC_ICON);

        wxUint32 GetMode() { return m_mode; }
        void Reset(wxUint32 mode,wxUint32 count=0);
        void UpdateData(CslServerInfo *info);

    private:
        wxUint32 m_mode;

        void OnSize(wxSizeEvent& event);

        DECLARE_EVENT_TABLE()

    protected:
        wxFlexGridSizer *m_sizer;
        CslListCtrlCountry *m_listCtrl;
        wxGauge *m_gauge;
};


class CslListCtrlCountry : public CslListCtrl
{
    public:
        CslListCtrlCountry(wxWindow* parent,wxWindowID id,const wxPoint& pos=wxDefaultPosition,
                           const wxSize& size=wxDefaultSize,long style=wxLC_ICON,
                           const wxValidator& validator=wxDefaultValidator,
                           const wxString& name=wxListCtrlNameStr);
        ~CslListCtrlCountry();

        void ListClear();
        void ListAdjustSize(const wxSize& size=wxDefaultSize);
        void UpdateData(CslServerInfo *info);

    private:
        CslListSortHelper m_sortHelper;
        t_aCslCountryEntry m_entries;

        void ListInit();

        void OnColumnLeftClick(wxListEvent& event);
        void OnItemActivated(wxListEvent& event);
        void OnContextMenu(wxContextMenuEvent& event);
        void OnMenu(wxCommandEvent& event);

        DECLARE_EVENT_TABLE()

    protected:
        void UpdateEntry(const wxString& country,wxInt32 id);
        void ListSort(const wxInt32 column);

        static int wxCALLBACK ListSortCompareFunc(long item1,long item2,long data);
};

#endif //CSLLISTCTRLCOUNTRY_H
