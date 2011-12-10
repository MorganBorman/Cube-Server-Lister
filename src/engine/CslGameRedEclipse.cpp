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

#include "Csl.h"
#include "CslEngine.h"
#include "CslGameRedEclipse.h"

#include "../img/re_16_png.h"
#include "../img/re_24_png.h"

enum { MM_OPEN, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD };
enum { CS_ALIVE, CS_DEAD, CS_SPAWNING, CS_EDITING, CS_SPECTATOR, CS_WAITING };

enum
{
    G_DEMO = 0,
    G_EDITMODE,
    G_CAMPAIGN,
    G_DEATHMATCH,
    G_CTF,
    G_DTF,
    G_BOMBER,
    G_TT
};

enum
{
	G_M_NONE  = 0,
    G_M_TEAM  = 1<<0,
    G_M_INSTA = 1<<1,
    G_M_MEDI  = 1<<2,
    G_M_BALLI = 1<<3,
    G_M_DUEL  = 1<<4,
    G_M_SURV  = 1<<5,
    G_M_ARENA = 1<<6,
    G_M_ONSLO = 1<<7,
    G_M_JET   = 1<<8,
    G_M_VAMP  = 1<<9,
    G_M_EXP   = 1<<10,
    G_M_REZ   = 1<<11,
    G_M_MM1   = 1<<12,
    G_M_MM2   = 1<<13,
    G_M_MM3   = 1<<14,
};

#define G_MO_NUM  7

#define G_MU_NUM  15


static struct
{
    int type; const wxChar *name;
}

gametype[] =
{
    { G_DEMO,		wxT("Demo")					},
    { G_EDITMODE,	wxT("Editing")				},
    { G_CAMPAIGN,	wxT("Campaign")				},
    { G_DEATHMATCH,	wxT("Deathmatch")			},
    { G_CTF,		wxT("Capture")				},
    { G_DTF,		wxT("Defend")				},
    { G_BOMBER,		wxT("Bomber-Ball")			},
    { G_TT,			wxT("Time Trial")			},
},

mutstype[] =
{
	{ G_M_NONE, 	wxT("")						},
	{ G_M_TEAM, 	wxT("Team")					},
	{ G_M_INSTA, 	wxT("Instagib")				},
	{ G_M_MEDI, 	wxT("Medievil")				},
	{ G_M_BALLI, 	wxT("Ballistic")			},
	{ G_M_DUEL, 	wxT("Duel")					},
	{ G_M_SURV, 	wxT("Survivor")				},
	{ G_M_ARENA, 	wxT("Arena")				},
	{ G_M_ONSLO, 	wxT("Onslaught")			},
	{ G_M_JET, 		wxT("Jetpack")				},
	{ G_M_VAMP, 	wxT("Vampire")				},
	{ G_M_EXP, 		wxT("Expert")				},
	{ G_M_REZ, 		wxT("Resize")				},
	{ G_M_MM1, 		wxT("Game specific 1")		},
	{ G_M_MM2, 		wxT("Game specific 2")		},
	{ G_M_MM3, 		wxT("Game specific 3")		},
};


CslRedEclipse::CslRedEclipse()
{
    m_name=CSL_DEFAULT_NAME_RE;
    m_defaultMasterConnection=CslMasterConnection(CSL_DEFAULT_MASTER_RE,CSL_DEFAULT_MASTER_PORT_RE);
    m_capabilities=CSL_CAPABILITY_EXTINFO | CSL_CAPABILITY_CUSTOM_CONFIG |
                   CSL_CAPABILITY_CONNECT_PASS | CSL_CAPABILITY_CONNECT_ADMIN_PASS;
#ifdef __WXMAC__
    m_clientSettings.ConfigPath=::wxGetHomeDir();
    m_clientSettings.ConfigPath<<wxT("/Library/Application Support/redeclipse");
#elif __WXGTK__
    m_clientSettings.ConfigPath=::wxGetHomeDir()+wxT("/.redeclipse");
#endif
    m_icon16=BitmapFromData(wxBITMAP_TYPE_PNG,re_16_png,sizeof(re_16_png));
    m_icon24=BitmapFromData(wxBITMAP_TYPE_PNG,re_24_png,sizeof(re_24_png));
}

CslRedEclipse::~CslRedEclipse()
{
}

wxString CslRedEclipse::GetModeName(wxInt32 n,wxInt32 m) const
{
    if (n<0 || m<0 || (size_t)n>=sizeof(gametype)/sizeof(gametype[0]))
        return wxString(wxT("unknown"));

    wxString mode=gametype[n].name;

    wxString addition;

	loopi(G_MU_NUM)
	{
		if (mutstype[i].type & m)
		{
			if (!addition.IsEmpty())
				addition << wxT("/");
			addition << wxString(mutstype[i].name);
		}
	}

	if (!addition.IsEmpty())
		mode << wxT(" (") << addition << wxT(")");

    return mode;
}

const wxChar* CslRedEclipse::GetVersionName(wxInt32 prot) const
{
    static const wxChar* versions[] =
    {
        wxT("Supernova"),wxT("Ides")
    };

    wxInt32 v=CSL_LAST_PROTOCOL_RE-prot;

    if (v<0 || (size_t)v>=sizeof(versions)/sizeof(versions[0]))
    {
        static wxString version=wxString::Format(wxT("%d"),prot);
        return version.c_str();
    }
    else
        return versions[v];
}

const wxChar* CslRedEclipse::GetWeaponName(wxInt32 n) const
{
    static const wxChar* weapons[] =
    {
        _("Melee"),_("Pistol"),_("Sword"),_("Shotgun"),
        _("Smg"),_("Flamer"),_("Plasma"),_("Rifle"),
        _("Grenade"),_("Rocket")
    };

    return (n>=0 && (size_t)n<sizeof(weapons)/sizeof(weapons[0])) ?
           weapons[n] : T2C(_("unknown"));
}

void CslRedEclipse::GetPlayerstatsDescriptions(vector<wxString>& desc) const
{
    desc.add(_("Player"));
    desc.add(_("Team"));
    desc.add(_("Frags"));
    desc.add(_("Deaths"));
    desc.add(_("Teamkills"));
    desc.add(_("Ping"));
    desc.add(_("Accuracy"));
    desc.add(_("Health"));
    desc.add(_("Spree"));
    desc.add(_("Weapon"));
}

wxInt32 CslRedEclipse::GetBestTeam(CslTeamStats& stats,wxInt32 prot) const
{
    wxInt32 i,best=-1;

    if (stats.TeamMode && stats.m_stats.length()>0 && stats.m_stats[0]->Ok)
    {
        best=0;

        for (i=1;i<stats.m_stats.length() && stats.m_stats[i]->Ok;i++)
        {
            if (stats.m_stats[i]->Score>=stats.m_stats[best]->Score)
                best=i;
        }
    }

    return best;
}

bool CslRedEclipse::ParseDefaultPong(ucharbuf& buf,CslServerInfo& info) const
{
    vector<int>attr;
    wxUint32 l,numattr;
    char text[_MAXDEFSTR];
    bool wasfull=info.IsFull();

    l=getint(buf);
    if (info.HasRegisteredEvent(CslServerEvents::EVENT_EMPTY) && info.Players>0 && !l)
        info.SetEvents(CslServerEvents::EVENT_EMPTY);
    else if (info.HasRegisteredEvent(CslServerEvents::EVENT_NOT_EMPTY) && !info.Players && l>0)
        info.SetEvents(CslServerEvents::EVENT_NOT_EMPTY);
    info.Players=l;

    numattr=getint(buf);
    loopj(numattr) attr.add(getint(buf));
    if (numattr>=1)
    {
        info.Protocol=attr[0];
        info.Version=GetVersionName(info.Protocol);
    }
    if (numattr>=2)
        info.GameMode=GetModeName(attr[1],attr[2]);
    if (numattr>=4)
        info.TimeRemain=attr[3];
    if (numattr>=5)
    {
        info.PlayersMax=attr[4];

        if (info.HasRegisteredEvent(CslServerEvents::EVENT_FULL) && !wasfull && info.IsFull())
            info.SetEvents(CslServerEvents::EVENT_FULL);
        else if (info.HasRegisteredEvent(CslServerEvents::EVENT_NOT_FULL) && wasfull && !info.IsFull())
            info.SetEvents(CslServerEvents::EVENT_NOT_FULL);
    }

    l=info.MM;
    info.MM=CSL_SERVER_OPEN;
    info.MMDescription.Empty();

    if (numattr>=6)
    {
        if (attr[5]==MM_PASSWORD)
        {
            info.MMDescription<<wxT("PASS");
            info.MM|=CSL_SERVER_PASSWORD;
        }
        else
        {
            info.MMDescription=wxString::Format(wxT("%d"),attr[5]);

            if (attr[5]==MM_OPEN)
                info.MMDescription<<wxT(" (O)");
            else if (attr[5]==MM_VETO)
            {
                info.MMDescription<<wxT(" (V)");
                info.MM=CSL_SERVER_VETO;
            }
            else if (attr[5]==MM_LOCKED)
            {
                if (info.HasRegisteredEvent(CslServerEvents::EVENT_LOCKED) &&
                    CSL_MM_IS_VALID(l) && !CSL_SERVER_IS_LOCKED(l))
                    info.SetEvents(CslServerEvents::EVENT_LOCKED);
                info.MMDescription<<wxT(" (L)");
                info.MM=CSL_SERVER_LOCKED;
            }
            else if (attr[5]==MM_PRIVATE)
            {
                if (info.HasRegisteredEvent(CslServerEvents::EVENT_PRIVATE) &&
                    CSL_MM_IS_VALID(l) && !CSL_SERVER_IS_PRIVATE(l))
                    info.SetEvents(CslServerEvents::EVENT_PRIVATE);
                info.MMDescription<<wxT(" (P)");
                info.MM=CSL_SERVER_PRIVATE;
            }
        }
    }

    getstring(text,buf);
    info.Map=A2U(text);
    getstring(text,buf);
    l=(wxInt32)strlen(text);
    FixString(text,&l,1);
    info.SetDescription(A2U(text));

    return !buf.overread();
}

bool CslRedEclipse::ParsePlayerPong(wxUint32 protocol,ucharbuf& buf,CslPlayerStatsData& info) const
{
    char text[_MAXDEFSTR];

    info.ID=getint(buf);
    if (protocol>=104)
        info.Ping=getint(buf);
    getstring(text,buf);
    info.Name=A2U(text);
    getstring(text,buf);
    info.Team=A2U(text);
    info.Frags=getint(buf);
    if (protocol>=104)
        info.Flagscore=getint(buf);
    info.Deaths=getint(buf);
    info.Teamkills=getint(buf);
    if (protocol>=103)
        info.Accuracy=getint(buf);
    info.Health=getint(buf);
    info.Armour=getint(buf);
    info.Weapon=getint(buf);
    info.Privileges=getint(buf);
    switch (getint(buf))
    {
        case CS_ALIVE:     info.State=CSL_PLAYER_STATE_ALIVE; break;
        case CS_DEAD:      info.State=CSL_PLAYER_STATE_DEAD; break;
        case CS_SPAWNING:  info.State=CSL_PLAYER_STATE_SPAWNING; break;
        case CS_EDITING:   info.State=CSL_PLAYER_STATE_EDITING; break;
        case CS_SPECTATOR: info.State=CSL_PLAYER_STATE_SPECTATOR; break;
        case CS_WAITING:   info.State=CSL_PLAYER_STATE_WAITING; break;
        default:           info.State=CSL_PLAYER_STATE_UNKNOWN; break;
    }

    return !buf.overread();
}

bool CslRedEclipse::ParseTeamPong(wxUint32 protocol,ucharbuf& buf,CslTeamStatsData& info) const
{
    wxInt32 i;
    char text[_MAXDEFSTR];

    getstring(text,buf);
    info.Name=A2U(text);
    info.Score=getint(buf);
    i=getint(buf);
    if (i>0)
        while (i--)
            info.Bases.add(getint(buf));

    return !buf.overread();
}

void CslRedEclipse::SetClientSettings(const CslGameClientSettings& settings)
{
    CslGameClientSettings set=settings;

#ifdef __WXMAC__
    bool isApp=set.Binary.EndsWith(wxT(".app"));

    if (set.Binary.IsEmpty() || isApp ? !::wxDirExists(set.Binary) : !::wxFileExists(set.Binary))
        return;
    if (set.GamePath.IsEmpty() || !::wxDirExists(set.GamePath))
    {
        if (!isApp)
            return;
        set.GamePath=set.Binary+wxT("/Contents/gamedata/");
        if (!::wxDirExists(set.GamePath))
            return;
    }
    if (set.ConfigPath.IsEmpty() || !::wxDirExists(set.ConfigPath))
        set.ConfigPath=set.GamePath;
    if (isApp)
        set.Binary<<wxT("/Contents/gamedata/redeclipse.app/Contents/MacOS/redeclipse");
    if (set.Options.IsEmpty())
        set.Options=wxT("-rinit.cfg");
#else
    if (set.GamePath.IsEmpty() || !::wxDirExists(set.GamePath))
        return;
    if (set.ConfigPath.IsEmpty() || !::wxDirExists(set.ConfigPath))
        set.ConfigPath=set.GamePath;
#endif
    if (set.Binary.IsEmpty() || !::wxFileExists(set.Binary))
        return;

    m_clientSettings=set;
}

wxString CslRedEclipse::GameStart(CslServerInfo *info,wxUint32 mode,wxString& error)
{
    wxString address,password,path,script;
    wxString bin=m_clientSettings.Binary;
    wxString opts=m_clientSettings.Options;
    wxString preScript=m_clientSettings.PreScript;
    wxString postScript=m_clientSettings.PostScript;

    if (m_clientSettings.Binary.IsEmpty() || !::wxFileExists(m_clientSettings.Binary))
    {
        error=_("Client binary for game Red Eclipse not found!\nCheck your settings.");
        return wxEmptyString;
    }
    if (m_clientSettings.GamePath.IsEmpty() || !::wxDirExists(m_clientSettings.GamePath))
    {
        error=_("Game path for game Red Eclipse not found!\nCheck your settings.");
        return wxEmptyString;
    }
    if (m_clientSettings.ConfigPath.IsEmpty() || !::wxDirExists(m_clientSettings.ConfigPath))
    {
        error=_("Config path for game Red Eclipse not found!\nCheck your settings.");
        return wxEmptyString;
    }

    path=m_clientSettings.ConfigPath;
#ifdef __WXMSW__
    //binary must be surrounded by quotes if the path contains spaces
    bin=wxT("\"")+m_clientSettings.Binary+wxT("\"");
    // use Prepend() and do not use opts+= here, since -h<path> must be before -r
    opts.Prepend(wxT("\"-h")+path.RemoveLast()+wxT("\" "));
#else
    CmdlineEscapeSpaces(bin);
    CmdlineEscapeSpaces(path);
    // use Prepend() and do not use opts+= here, since -h<path> must be before -r
    opts.Prepend(wxT("-h")+path+wxT(" "));
#endif //__WXMSW__

    ProcessScript(*info,mode,preScript);
    ProcessScript(*info,mode,postScript);

    if (!preScript.IsEmpty())
    {
        preScript<<wxT(";");
        CmdlineEscapeQuotes(preScript);
    }
    if (!postScript.IsEmpty())
    {
        postScript.Prepend(wxT(";"));
        CmdlineEscapeQuotes(postScript);
    }

    address=info->Host;

    password<<wxT("\"");
    password<<(mode==CslServerInfo::CSL_CONNECT_PASS ? info->Password :
               mode==CslServerInfo::CSL_CONNECT_ADMIN_PASS ?
               info->PasswordAdmin : wxString(wxEmptyString));
    password<<wxT("\"");

    CmdlineEscapeQuotes(password);

    if (GetDefaultGamePort()!=info->GamePort || mode==CslServerInfo::CSL_CONNECT_PASS)
        address<<wxString::Format(wxT(" %d %d"),info->GamePort,info->InfoPort);

    script=wxString::Format(wxT("%sconnect %s %s%s"),
                            preScript.c_str(),
                            address.c_str(),password.c_str(),
                            postScript.c_str());
#ifdef __WXMSW__
    opts<<wxT(" \"-x")<<script<<wxT("\"");
#else
    opts<<wxT(" -x")<<CmdlineEscapeSpaces(script);
#endif

    bin<<wxT(" ")<<opts;

    LOG_DEBUG("start client: %s\n",U2A(bin));

    return bin;
}

wxInt32 CslRedEclipse::GameEnd(wxString& error)
{
    return CSL_ERROR_NONE;
}
