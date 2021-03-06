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

#ifndef CSLAPP_H
#define CSLAPP_H

/**
    @author Glen Masgai <mimosius@users.sourceforge.net>
*/

class CslApp: public wxApp
{
    public:
        enum
        {
            CSL_SHUTDOWN_NONE = 0,
            CSL_SHUTDOWN_NORMAL,
            CSL_SHUTDOWN_FORCE
        };

        const wxString& GetLanguage() const { return m_lang; }

        CslEngine* GetCslEngine() { return m_engine; }

        void Shutdown(wxInt32 val) { m_shutdown=val; }
        wxInt32 Shutdown() const { return m_shutdown; }

        void IpcCall(const wxString& value,wxEvtHandler *evtHandler=NULL) const;

    private:
        CslEngine *m_engine;

        wxSingleInstanceChecker *m_single;
        wxInt32 m_shutdown;

        wxString m_lang;
        wxLocale m_locale;

        virtual bool OnInit();
        virtual int OnRun();
        virtual int OnExit();

        int FilterEvent(wxEvent& event);

        void OnEndSession(wxCloseEvent& event);

        DECLARE_EVENT_TABLE()
};

DECLARE_APP(CslApp);

#endif
