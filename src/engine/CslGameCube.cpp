/***************************************************************************
 *   Copyright (C) 2007 by Glen Masgai                                     *
 *   mimosius@gmx.de                                                       *
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

#include "CslEngine.h"
#include "CslGameCube.h"
#include "CslTools.h"

#include "../img/cb_24.xpm"
#include "../img/cb_16.xpm"

CslGameCube::CslGameCube()
{
    m_name=CSL_DEFAULT_NAME_CB;
    m_defaultMasterConnection=CslMasterConnection(CSL_DEFAULT_MASTER_CB,CSL_DEFAULT_MASTER_PATH_CB);
    m_capabilities=CSL_CAPABILITY_CONNECT_PASS;
}

CslGameCube::~CslGameCube()
{
}

wxString CslGameCube::GetVersionName(wxInt32 n) const
{
    static const wxChar* versions[] =
    {
        wxT("122")
    };
    wxUint32 v=CSL_LAST_PROTOCOL_CB-n;
    return (v>=0 && v<sizeof(versions)/sizeof(versions[0])) ?
           wxString(versions[v]) : wxString::Format(wxT("%d"),n);
}

wxString CslGameCube::GetModeName(wxInt32 n) const
{
    static const wxChar* modes[] =
    {
        wxT("ffa/default"),wxT("coopedit"),wxT("ffa/duel"),wxT("teamplay"),
        wxT("instagib"),wxT("instagib team"),wxT("efficiency"),wxT("efficiency team"),
        wxT("insta arena"),wxT("insta clan arena"),wxT("tactics arena")
    };
    return (n>=0 && (size_t)n<sizeof(modes)/sizeof(modes[0])) ?
           wxString(modes[n]) : wxString(_("unknown"));
}

bool CslGameCube::ParseDefaultPong(ucharbuf& buf,CslServerInfo& info) const
{
    char text[_MAXDEFSTR];
    wxUint32 l;

    //weird hack
    for (l=0;l<(wxUint32)buf.maxlength();l++)
        *buf.at(l)^=0x61;

    wxInt32 prot=getint(buf);
    if (prot!=CSL_LAST_PROTOCOL_CB)
        return false;
    info.Protocol=prot;
    info.Version=GetVersionName(prot);
    info.GameMode=GetModeName(getint(buf));
    info.Players=getint(buf);
    info.TimeRemain=getint(buf);
    getstring(text,buf);
    info.Map=A2U(text);
    getstring(text,buf);
    l=strlen(text);
    StripColours(text,&l,2);
    info.SetDescription(A2U(text));

    return true;
}

wxString CslGameCube::GameStart(CslServerInfo *info,wxUint32 mode,wxString *error)
{
    wxString address,password;
    wxString bin=m_clientSettings.Binary;
    wxString opts=m_clientSettings.Options;

    if (m_clientSettings.Binary.IsEmpty() || !::wxFileExists(m_clientSettings.Binary))
    {
        *error=_("Client binary for game Cube not found!\nCheck your settings.");
        return wxEmptyString;
    }
    if (m_clientSettings.GamePath.IsEmpty() || !::wxDirExists(m_clientSettings.GamePath))
    {
        *error=_("Game path for game Cube not found!\nCheck check your settings.");
        return wxEmptyString;
    }

    address=info->Host;
    if (GetDefaultPort()!=info->Port)
        address+=m_portDelimiter+wxString::Format(wxT("%d"),info->Port);

    bin+=wxString(wxT(" "))+opts;

    if (mode==CSL_CONNECT_ADMIN_PASS)
        password=info->Password;

    return InjectConfig(address,password,error)==CSL_ERROR_NONE ? bin:wxString(wxEmptyString);
}

wxInt32 CslGameCube::GameEnd(wxString *error)
{
    wxString cfg=m_clientSettings.GamePath+CSL_DEFAULT_INJECT_FILE_CB;
    wxString bak=cfg+wxT(".csl");

    if (!::wxFileExists(bak))
        return CSL_ERROR_FILE_DONT_EXIST;
    if (!::wxRenameFile(bak,cfg))
        return CSL_ERROR_FILE_OPERATION;

    return CSL_ERROR_NONE;
}

wxInt32 CslGameCube::InjectConfig(const wxString& address,const wxString& password,wxString *error)
{
    char *buf;
    wxFile file;
    bool autoexec=true;
    wxString cfg,script;

    cfg=m_clientSettings.GamePath+wxString(wxT("autoexec.cfg"));

    // check autoexec.cfg
    if (!::wxFileExists(cfg))
    {
        if (!file.Create(cfg,false,wxS_IRUSR|wxS_IWUSR))
            return CSL_ERROR_FILE_OPERATION;
        file.Close();
    }
    if (!file.Open(cfg,wxFile::read_write))
        return CSL_ERROR_FILE_OPERATION;

    wxUint32 size=file.Length();

    if (size)
    {
        buf=new char[size+1];
        buf[size]=0;

        if (file.Read((void*)buf,size)!=(wxInt32)size)
        {
            delete[] buf;
            file.Close();
            return CSL_ERROR_FILE_OPERATION;
        }

        if (strstr(buf,"alias csl_connect 1"))
            autoexec=false;

        delete[] buf;
    }

    if (autoexec)
    {
        if (!file.Write(wxT("\r\nalias csl_connect 1\r\n")))
        {
            file.Close();
            return CSL_ERROR_FILE_OPERATION;
        }
    }
    file.Close();

    // make a backup of the map config
    cfg=m_clientSettings.GamePath+CSL_DEFAULT_INJECT_FILE_CB;
    if (::wxFileExists(cfg))
    {
        if (!::wxCopyFile(cfg,cfg+wxT(".csl")))
            return CSL_ERROR_FILE_OPERATION;
    }
    else
    {
        file.Create(cfg+wxT(".csl"),false,wxS_IRUSR|wxS_IWUSR);
        file.Close();
    }

    if (!password.IsEmpty())
        script=wxT("password ")+password;

    script+=wxString::Format(wxT("\r\nif (= $csl_connect 1) [ sleep 1000 [ connect %s ] ]\r\n%s\r\n"),
                             address.c_str(),wxT("alias csl_connect 0"));

    return WriteTextFile(cfg,U2A(script),wxFile::write_append);
}

const char** CslGameCube::GetIcon(wxInt32 size) const
{
    switch (size)
    {
        case 16:
            return cb_16_xpm;
        case 24:
            return cb_24_xpm;
        default:
            break;
    }
    return NULL;
}

void CslGameCube::ProcessOutput(char *data,wxUint32 *len) const
{
    StripColours(data,len,1);
}