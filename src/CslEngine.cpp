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

#include <wx/protocol/http.h>
#include <wx/platinfo.h>
#include "CslEngine.h"
#include "CslVersion.h"
#include "CslTools.h"
#include "cube_tools.h"

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(wxCSL_EVT_RESOLVE,wxID_ANY)
END_DECLARE_EVENT_TYPES()

#define CSL_EVT_RESOLVE(id,fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
                               wxCSL_EVT_RESOLVE,id,wxID_ANY, \
                               (wxObjectEventFunction)(wxEventFunction) \
                               wxStaticCastEvent(wxCommandEventFunction,&fn), \
                               (wxObject*)NULL \
                             ),

DEFINE_EVENT_TYPE(wxCSL_EVT_RESOLVE)
DEFINE_EVENT_TYPE(wxCSL_EVT_PONG)

BEGIN_EVENT_TABLE(CslEngine,wxEvtHandler)
    CSL_EVT_PING(wxID_ANY,CslEngine::OnPong)
    CSL_EVT_RESOLVE(wxID_ANY,CslEngine::OnResolve)
END_EVENT_TABLE()


void *CslResolverThread::Entry()
{
    m_mutex.Lock();

    while (!m_terminate)
    {
        if (!m_packets.length())
        {
            LOG_DEBUG("waiting\n");
            m_condition->Wait();
            LOG_DEBUG("got signal\n");
            if (m_terminate)
                break;
        }

        if (!m_packets.length())
            continue;

        m_packetSection.Enter();
        CslResolverPacket *packet=m_packets[0];
        m_packets.remove(0);
        m_packetSection.Leave();

        packet->m_domain=packet->m_address.Hostname();

        wxCommandEvent evt(wxCSL_EVT_RESOLVE);
        evt.SetClientData(packet);
        wxPostEvent(m_evtHandler,evt);
    }

    loopv(m_packets) delete m_packets[i];
    m_mutex.Unlock();
    LOG_DEBUG("exit\n");

    return NULL;
}

void CslResolverThread::AddPacket(CslResolverPacket *packet)
{
    m_packetSection.Enter();
    m_packets.add(packet);
    m_packetSection.Leave();
    //wxMutexLocker lock(m_mutex);
    m_condition->Signal();
}

void CslResolverThread::Terminate()
{
    m_terminate=true;
    wxMutexLocker lock(m_mutex);
    m_condition->Signal();
}


CslEngine::CslEngine(wxEvtHandler *evtHandler) : wxEvtHandler(),
        m_isOk(false),
        m_evtHandler(evtHandler),
        m_resolveThread(NULL),
        m_currentGame(NULL),
        m_currentMaster(NULL)
{
    if (m_evtHandler)
        SetNextHandler(m_evtHandler);
    GetTicks();
};

CslEngine::~CslEngine()
{
    DeInitEngine();
}

bool CslEngine::InitEngine(wxUint32 updateInterval)
{
    if (m_isOk)
        return false;

    SetUpdateInterval(updateInterval);
    m_pingSock=new CslUDP(this);
    m_isOk=m_pingSock->IsInit();

    if (m_isOk)
    {
        m_resolveThread=new CslResolverThread(this);
        m_resolveThread->Run();
    }

    m_firstPing=true;

    return m_isOk;
}

void CslEngine::DeInitEngine()
{
    if (m_resolveThread)
    {
        m_resolveThread->Terminate();
        m_resolveThread->Delete();
        delete m_resolveThread;
    }

    delete m_pingSock;
    loopv(m_games) delete m_games[i];
}

CslGame* CslEngine::AddMaster(CslMaster *master)
{
    wxASSERT(master);

    if (!master)
        return NULL;

    CslGame *game=FindOrCreateGame(master->GetType());
    if (game->AddMaster(master)>=0)
        return game;

    return NULL;
}

void CslEngine::DeleteMaster(CslMaster *master)
{
    wxASSERT(master);

    if (!master)
        return;

    master->GetGame()->DeleteMaster(master);
}

CslServerInfo* CslEngine::AddServer(CslServerInfo *info,wxInt32 masterid)
{
    wxMutexLocker lock(m_mutex);

    CslGame *game=FindOrCreateGame(info->m_type);
    if (!game)
        return NULL;

    CslServerInfo *ret=game->AddServer(info,masterid);
    if (ret && ret->IsFavourite())
    {
        if (m_favourites.find(ret)<0)
            m_favourites.add(ret);
    }

    if (ret)
    {
        CslResolverPacket *packet=new CslResolverPacket(ret->m_addr,ret->m_type);
        m_resolveThread->AddPacket(packet);
    }

    return ret;
}

bool CslEngine::AddServerToFavourites(CslServerInfo *info)
{
    wxMutexLocker lock(m_mutex);

    if (m_favourites.find(info)>=0)
        return false;

    info->SetFavourite();
    m_favourites.add(info);

    return true;
}

void CslEngine::RemoveServerFromMaster(CslServerInfo *info)
{
    wxMutexLocker lock(m_mutex);
    m_currentGame->RemoveServer(info,m_currentMaster);
}

void CslEngine::RemoveServerFromFavourites(CslServerInfo *info)
{
    wxMutexLocker lock(m_mutex);

    wxInt32 i=m_favourites.find(info);
    if (i>=0)
        m_favourites.remove(i);
    info->RemoveFavourite();
}

void CslEngine::DeleteServer(CslServerInfo *info)
{
    wxMutexLocker lock(m_mutex);

    if (info->IsLocked())
        return;

    if (info->IsFavourite())
    {
        wxUint32 i=m_favourites.find(info);
        if (i>=0) m_favourites.remove(i);
    }

    CslGame *game=FindGame(info->m_type);
    if (!game)
    {
        wxASSERT(game);
        return;
    }
    game->DeleteServer(info);
}

CslGame* CslEngine::FindGame(CSL_GAMETYPE type)
{
    CslGame *game;

    loopv(m_games)
    {
        game=m_games[i];
        if (game->GetType()==type)
            return game;
    }
    return NULL;
}

CslGame* CslEngine::FindOrCreateGame(CSL_GAMETYPE type)
{
    CslGame *game=FindGame(type);

    if (game)
        return game;

    game=new CslGame(type);
    m_games.add(game);

    return game;
}

bool CslEngine::SetCurrentGame(CslGame *game,CslMaster *master)
{
    wxMutexLocker lock(m_mutex);

    if (game)
        if (m_games.find(game)<0)
            return false;
    if (master)
        if (game->m_masters.find(master)<0)
            return false;

    m_currentGame=game;
    m_currentMaster=master;

    FuzzPingSends();

    return true;
}

bool CslEngine::Ping(CslServerInfo *info,bool force)
{
    CslUDPPacket *packet;
    uchar ping[16];

    wxUint32 ticks=GetTicks();
    wxUint32 interval=m_updateInterval;

    if (info->m_waiting)
    {
        interval=CSL_UPDATE_INTERVAL_WAIT;
        LOG_DEBUG("iswaiting\n");
    }

    //LOG_DEBUG("%s - ticks:%li, pingsend:%li, diff:%li\n",U2A(info->GetBestDescription()),
    //          ticks,info->m_pingSend,ticks-info->m_pingSend);
    if (!force && (ticks-info->m_pingSend)<interval)
        return false;
    if (info->m_addr.IPAddress()==wxT("255.255.255.255"))
        return false;

    info->m_pingSend=ticks;

    if ((info->m_type==CSL_GAME_SB) &&
        (info->m_ping<0 || info->m_exInfo==CSL_EXINFO_OK) &&
        (info->CanPingUptime()))
    {
        return PingExUptime(info);
    }

    if (info->m_exInfo!=CSL_EXINFO_FALSE || info->m_type!=CSL_GAME_SB)
    {
        // default ping packet
        packet=new CslUDPPacket();
        ucharbuf p(ping,sizeof(ping));
        putint(p,info->m_pingSend);
        packet->Set(info->m_addr,ping,p.length());
        m_pingSock->SendPing(packet);
    }
    else
        PingExUptime(info);

    //LOG_DEBUG("Ping %s - %d\n",U2A(info->GetBestDescription()),ticks);

    return true;
}

bool CslEngine::PingExUptime(CslServerInfo *info)
{
    //LOG_DEBUG("uptime %s - %d\n",U2A(info->GetBestDescription()),GetTicks());
    uchar ping[16];

    CslUDPPacket *packet=new CslUDPPacket();
    ucharbuf p(ping,sizeof(ping));
    putint(p,0);
    putstring(CSL_EX_CMD_UPTIME,p);
    packet->Set(info->m_addr,ping,p.length());
    m_pingSock->SendPing(packet);

    return true;
}

bool CslEngine::PingExPlayerInfo(CslServerInfo *info,wxInt32 pid)
{
    if (info->m_exInfo!=CSL_EXINFO_OK)
        return false;

    CslUDPPacket *packet;
    uchar ping[16];

    if (pid==-1)
        info->m_playerStats->Reset();

    ucharbuf p(ping,sizeof(ping));
    putint(p,0);
    putstring(CSL_EX_CMD_PLAYERSTATS,p);
    putint(p,pid);

    packet=new CslUDPPacket();
    packet->Set(info->m_addr,ping,p.length());
    return m_pingSock->SendPing(packet);
}

bool CslEngine::PingExTeamInfo(CslServerInfo *info)
{
    if (info->m_exInfo!=CSL_EXINFO_OK)
        return false;

    CslUDPPacket *packet;
    uchar ping[16];

    info->m_teamStats->Reset();

    ucharbuf p(ping,sizeof(ping));
    putint(p,0);
    putstring(CSL_EX_CMD_TEAMSTATS,p);

    packet=new CslUDPPacket();
    packet->Set(info->m_addr,ping,p.length());
    return m_pingSock->SendPing(packet);
}

wxUint32 CslEngine::PingServers()
{
    if (m_currentGame==NULL)
        return 0;

    if (m_mutex.TryLock()==wxMUTEX_BUSY)
    {
        LOG_DEBUG("wxMUTEX_BUSY\n");
        return 0;
    }

    wxUint32 pc=0;

    CslServerInfo *info;
    vector<CslServerInfo*> *servers=m_currentGame->GetServers();

    loopv(*servers)
    {
        info=servers->at(i);
        /*if (m_currentMaster)
        {
        CslMaster *master=info->m_master;
        if (master && m_currentMaster->Address()!=master->Address())
        continue;
        }*/

        bool force=m_firstPing&&i<5;
        if (Ping(info,force))
            pc++;
    }

    loopv(m_favourites)
    {
        info=m_favourites[i];
        if (Ping(info))
            pc++;
    }

//  #ifdef __WXDEBUG__
//      if (pc)
//          LOG_DEBUG("Pinged %d of %d\n",pc,servers->length()+m_favourites.length());
//  #endif

    m_mutex.Unlock();

    m_firstPing=false;

    return pc;
}

int CslEngine::UpdateMaster()
{
    int num=0;
    uchar *buf=NULL;

    if (m_currentMaster==NULL)
        return -1;

    wxPlatformInfo pinfo;
    wxOperatingSystemId id=pinfo.GetOperatingSystemId();
    wxString os=pinfo.GetOperatingSystemFamilyName(id);
    wxString agent=wxT("Csl/");
    agent+=CSL_VERSION_LONG_STR;
    agent+=wxT("(")+os+wxT(")");

    wxHTTP http;
    http.SetTimeout(10);
    if (!http.Connect(m_currentMaster->GetAddress(),80))
        return -2;
    http.SetHeader(wxT("User-Agent"),agent);
    wxInputStream *data=http.GetInputStream(m_currentMaster->GetPath());

    wxUint32 code=http.GetResponse();

    if (!data||code!=200)
        return -3;

    size_t size=data->GetSize();

    if (!size)
    {
        num=-4;
        goto finish;
    }
    // some server do not report the size of the content
    // somehow set a limit
    if (size>32767)
        size=32767;

    buf=(uchar*)calloc(1,size+1);
    if (!buf)
    {
        num=-5;
        goto finish;
    }

    data->Read((void*)buf,size);

    if (strstr((char*)buf,"<html>")||strstr((char*)buf,"<HTML>"))
        num=-4;
    else
    {
        vector <CslServerInfo*> *servers=m_currentMaster->GetServers();
        loopv(*servers) RemoveServerFromMaster(servers->at(i));

        char *p=(char*)buf;
        char *pend;
        bool end=false;
        CslServerInfo *info,*ret;

        while ((p=strstr(p,"addserver"))!=NULL)
        {
            p=strpbrk(p,"123456789 \t");
            while (*p && (*p==' ' || *p=='\t'))
                *p++;
            if (!p)
                return -3;
            pend=strpbrk(p," \t\r\n");
            if (!pend)
                end=true;
            else
                *pend=0;

            info=new CslServerInfo(A2U(p),m_currentMaster->GetType());
            ret=AddServer(info,m_currentMaster->GetID());
            if (!ret || ret!=info)
                delete info;
            num++;

            if (end)
                break;

            p=pend+1;
        }
    }

finish:
    if (buf) free(buf);
    if (data) delete data;

    FuzzPingSends();

    return num;
}

void CslEngine::FuzzPingSends()
{
    vector<CslServerInfo*> *servers;

    if (m_currentMaster)
        servers=m_currentMaster->GetServers();
    else if (m_currentGame)
        servers=m_currentGame->GetServers();
    else
        return;

    wxUint32 ticks=GetTicks();

    CslServerInfo *info;
    loopv(*servers)
    {
        info=servers->at(i);
        info->m_pingSend=ticks-(i*500)%m_updateInterval;
    }
}

void CslEngine::UpdateServerInfo(CslServerInfo *info,ucharbuf *buf,wxUint32 now)
{
    wxUint32 l;
    wxUint32 ml=127;
    char text[128];
    text[ml]=0;

    info->m_pingResp=GetTicks();
    info->m_ping=info->m_pingResp-info->m_pingSend;
    info->m_lastSeen=now;

    switch (info->m_type)
    {
        case CSL_GAME_SB:
        case CSL_GAME_BF:
        {
            info->m_players=getint(*buf);
            int numattr=getint(*buf);
            vector<int>attr;
            attr.setsize(0);
            loopj(numattr) attr.add(getint(*buf));
            if (numattr>=1)
                info->m_protocol=attr[0];
            if (numattr>=2)
            {
                info->m_gameMode=GetModeStrSB(attr[1]);
                info->m_hasBases=attr[1]>11 && attr[1]<14;
            }
            if (numattr>=3)
            {
                info->m_timeRemain=attr[2];
                if (info->m_protocol<254)
                    info->m_timeRemain++;
            }
            if (numattr>=4)
                info->m_playersMax=attr[3];
            if (numattr>=5)
                info->m_mm=attr[4];

            getstring(text,*buf,ml);
            info->m_map=A2U(text);
            getstring(text,*buf,ml);
            l=strlen(text);
            StripColours(text,&l,2);
            info->SetDescription(A2U(text));
            break;
        }

        case CSL_GAME_CB:
        {
            // this sucks!
            wxInt32 i;
            for (i=0;i<buf->maxlength();i++)
                *buf->at(i)^=0x61;
            // dont break, nearly same as AC
        }
        case CSL_GAME_AC:
        {

            wxInt32 prot=getint(*buf);
            if (info->m_type==CSL_GAME_CB && prot!=CSL_LAST_PROTOCOL_CB)
                return;
            info->m_protocol=prot;
            info->m_gameMode=info->m_type==CSL_GAME_AC ?
                             GetModeStrAC(getint(*buf)):
                             GetModeStrSB(getint(*buf));
            info->m_players=getint(*buf);
            info->m_timeRemain=getint(*buf);
            getstring(text,*buf,ml);
            info->m_map=A2U(text);
            getstring(text,*buf,ml);
            l=strlen(text);
            StripColours(text,&l,2);
            info->SetDescription(A2U(text));
            if (info->m_type==CSL_GAME_AC)
                info->m_playersMax=getint(*buf);
            break;
        }

        default:
            wxASSERT(info->m_type);
    }
}

void CslEngine::ParsePongCmd(CslServerInfo *info,CslUDPPacket *packet,wxUint32 now)
{
    wxInt32 vi;
    wxUint32 exVersion;
    wxUint32 vu=0;
    wxUint32 ml=127;
    char text[128];
    text[ml]=0;
    bool extended=false;
    ucharbuf p((uchar*)packet->Data(),packet->Size());

#ifdef __WXDEBUG__
    const wxChar *dbg_type;
#endif

    vu=getint(p);
    if (!vu)
        extended=true;

    while (packet->Size() > (wxUint32)p.length())
    {
        switch (info->m_type)
        {
            case CSL_GAME_SB:
            case CSL_GAME_BF:
            {
                if (extended)
                {
                    getstring(text,p,ml);

                    if (strcmp(text,CSL_EX_CMD_UPTIME)==0)
                    {
#ifdef __WXDEBUG__
                        dbg_type=wxT(CSL_EX_CMD_UPTIME);
#endif
                        vu=p.length();
                        if (getint(p)!=-1)  // check ack
                        {
                            info->m_exInfo=CSL_EXINFO_FALSE;
                            p.len=vu;
                            UpdateServerInfo(info,&p,now);
                            break;
                        }

                        exVersion=getint(p);
                        // check protocol
                        if (exVersion<CSL_EX_VERSION_MIN || exVersion>CSL_EX_VERSION_MAX)
                        {
                            LOG_DEBUG("%s (%s) ERROR: prot=%d\n",dbg_type,
                                      U2A(info->GetBestDescription()),vi);
                            info->m_exInfo=CSL_EXINFO_ERROR;
                            Ping(info,true);
                            return;
                        }

                        info->m_uptime=getint(p);

                        if (p.overread())
                        {
                            LOG_DEBUG("%s (%s) OVERREAD!\n",dbg_type,U2A(info->GetBestDescription()));
                            info->m_exInfo=CSL_EXINFO_ERROR;
                            Ping(info,true);
                            return;
                        }

                        info->m_exInfo=CSL_EXINFO_OK;
                        info->CreatePlayerStats();
                        info->CreateTeamStats();
                        LOG_DEBUG("%s (%s) %s\n",dbg_type,U2A(info->GetBestDescription()),
                                  U2A(FormatSeconds(info->m_uptime)));

                        Ping(info,true);
                    }
                    else if (strcmp(text,CSL_EX_CMD_PLAYERSTATS)==0)
                    {
                        CslPlayerStats *stats=info->m_playerStats;
#ifdef __WXDEBUG__
                        dbg_type=wxT(CSL_EX_CMD_PLAYERSTATS);
#endif
                        wxInt32 rid=getint(p);  // resent id or -1

                        if (getint(p)!=-1)  // check ack
                        {
                            LOG_DEBUG("%s (%s) ERROR: missing ACK\n",dbg_type,
                                      U2A(info->GetBestDescription()));
                            info->m_exInfo=CSL_EXINFO_FALSE;
                            return;
                        }

                        exVersion=getint(p);
                        // check protocol
                        if (exVersion<CSL_EX_VERSION_MIN || exVersion>CSL_EX_VERSION_MAX)
                        {
                            LOG_DEBUG("%s (%s) ERROR: prot=%d\n",dbg_type,
                                      U2A(info->GetBestDescription()),vi);
                            info->m_exInfo=CSL_EXINFO_ERROR;
                            return;
                        }

                        vi=getint(p);
                        if (vi>0)  // check error
                        {
                            LOG_DEBUG("%s (%s) ERROR: received - id=%d\n",dbg_type,
                                      U2A(info->GetBestDescription()),rid);
                            if (rid>-1)  // check for resend error, client doesn't exist anymore
                            {
                                stats->RemoveId(rid);

                                wxCommandEvent evt(wxCSL_EVT_PONG);
                                evt.SetClientData((void*)new CslPongPacket(info,CSL_PONG_TYPE_PLAYERSTATS));
                                wxPostEvent(m_evtHandler,evt);
                                return;
                            }
                            return;
                        }

                        vi=getint(p);
                        if (vi==-10) // check for following ids
                        {
                            while (!p.overread())
                            {
                                vi=getint(p);
                                if (p.overread())
                                    break;
                                if (rid==-1 && !stats->AddId(vi))
                                {
                                    stats->Reset();
                                    LOG_DEBUG("%s (%s) ERROR: AddId(%d)\n",dbg_type,
                                              U2A(info->GetBestDescription()),vi);
                                    return;
                                }
                            }
                            stats->SetWaitForStats();
                            LOG_DEBUG("%s (%s) ids=%d\n",dbg_type,U2A(info->GetBestDescription()),
                                      stats->m_ids.length());
                            break;
                        }
                        else if (vi!=-11)  // check for following stats
                            return;

                        CslPlayerStatsData *data=stats->GetNewStatsPtr();

                        data->m_id=getint(p);
                        getstring(text,p,ml);
                        data->m_player=A2U(text);
                        getstring(text,p,ml);
                        data->m_team=A2U(text);
                        data->m_frags=getint(p);
                        data->m_deaths=getint(p);
                        data->m_teamkills=getint(p);
                        data->m_health=getint(p);
                        data->m_armour=getint(p);
                        data->m_weapon=getint(p);
                        data->m_priv=getint(p);
                        data->m_state=getint(p);

                        if (p.overread())
                        {
                            LOG_DEBUG("%s (%s) OVERREAD!\n",dbg_type,
                                      U2A(info->GetBestDescription()));
                            return;
                        }

                        //LOG_DEBUG("add stats id=%d\n",data->m_id);

                        if (!stats->AddStats(data))
                        {
                            stats->RemoveStats(data);
                            stats->Reset();
                            LOG_DEBUG("%s (%s) ERROR: AddStats()\n",dbg_type,
                                      U2A(info->GetBestDescription()));
                            break;
                        }

                        /*LOG_DEBUG("%s (%s): player:%s, team:%s, frags:%d, deaths:%d, tk:%d,\n",
                                  dbg_type,U2A(info->GetBestDescription()),
                                  U2A(data->m_player),U2A(data->m_team),
                                  data->m_frags,data->m_deaths,data->m_teamkills);*/

                        wxCommandEvent evt(wxCSL_EVT_PONG);
                        evt.SetClientData((void*)new CslPongPacket(info,CSL_PONG_TYPE_PLAYERSTATS));
                        wxPostEvent(m_evtHandler,evt);
                    }
                    else if (strcmp(text,CSL_EX_CMD_TEAMSTATS)==0)
                    {
                        CslTeamStats *stats=info->m_teamStats;
#ifdef __WXDEBUG__
                        dbg_type=wxT(CSL_EX_CMD_TEAMSTATS);
#endif
                        if (getint(p)!=-1)  // check ack
                        {
                            LOG_DEBUG("%s(%s) ERROR: missing ACK\n",dbg_type,
                                      U2A(info->GetBestDescription()));
                            info->m_exInfo=CSL_EXINFO_FALSE;
                            return;
                        }

                        exVersion=getint(p);
                        // check protocol
                        if (exVersion<CSL_EX_VERSION_MIN || exVersion>CSL_EX_VERSION_MAX)
                        {
                            LOG_DEBUG("%s (%s) ERROR: prot=%d\n",dbg_type,
                                      U2A(info->GetBestDescription()),vi);
                            info->m_exInfo=CSL_EXINFO_ERROR;
                            return;
                        }

                        stats->m_teamplay=!getint(p);  // error
                        stats->m_remain=getint(p);  // remaining time

                        if (!stats->m_teamplay)  // check error (no teammode)
                        {
                            stats->Reset();
                            wxCommandEvent evt(wxCSL_EVT_PONG);
                            evt.SetClientData((void*)new CslPongPacket(info,CSL_PONG_TYPE_TEAMSTATS));
                            wxPostEvent(m_evtHandler,evt);
                            LOG_DEBUG("%s (%s) ERROR: received - no teammode\n",
                                      dbg_type,U2A(info->GetBestDescription()));
                            return;
                        }

                        while (!p.overread())
                        {
                            CslTeamStatsData *data=stats->GetNewStatsPtr();

                            getstring(text,p,ml);
                            data->m_team=A2U(text);
                            data->m_score=getint(p);
                            if (exVersion>=102)
                            {
                                vi=getint(p);
                                while (vi--)
                                    data->m_bases.add(getint(p));
                            }

                            stats->AddStats(data);

                            if (p.overread())
                            {
                                data->Reset();
                                break;
                            }
                            LOG_DEBUG("%s (%s): team:%s, score:%d, bases:%d, remain:%d\n",
                                      dbg_type,U2A(info->GetBestDescription()),
                                      U2A(data->m_team),data->m_score,data->m_bases.length(),stats->m_remain);
                        }

                        wxCommandEvent evt(wxCSL_EVT_PONG);
                        evt.SetClientData((void*)new CslPongPacket(info,CSL_PONG_TYPE_TEAMSTATS));
                        wxPostEvent(m_evtHandler,evt);
                    }
                    else
                    {
#ifdef __WXDEBUG__
                        dbg_type=wxT("unknown");
#endif
                        break;
                    }
                }
                else
                    UpdateServerInfo(info,&p,now);

                break;
            }

            case CSL_GAME_AC:
            case CSL_GAME_CB:
                UpdateServerInfo(info,&p,now);
                break;

            default:
                break;
        }
#ifdef __WXDEBUG__
        if (p.length()<(wxInt32)packet->Size())
            LOG_DEBUG("%s: %d bytes left (type=%s)\n",U2A(info->GetBestDescription()),
                      packet->Size()-p.length(),U2A(dbg_type));
#endif
    }
}

void CslEngine::OnPong(wxCommandEvent& event)
{
    CslUDPPacket *packet=(CslUDPPacket*)event.GetClientData();
    if (!packet)
        return;

    if (!packet->Size() || !m_currentGame)
    {
        delete packet;
        return;
    }

    wxMutexLocker lock(m_mutex);

    bool skip=false;
    CslServerInfo *info=NULL;

    wxDateTime now=wxDateTime::Now();
    wxUint32 ticks=now.GetTicks();

    if ((info=m_currentGame->FindServerByAddr(packet->Address()))!=NULL)
    {
        skip=true;
        ParsePongCmd(info,packet,ticks);
    }

    if (!skip)
    {
        loopv(m_favourites)
        {
            info=m_favourites[i];
            if (info->m_addr.IPAddress()==packet->Address().IPAddress() &&
                info->m_addr.Service()==packet->Address().Service())
            {
                skip=true;
                ParsePongCmd(info,packet,ticks);
                break;
            }
        }
    }

    delete packet;

    if (skip && m_evtHandler)
    {
        event.SetClientData((void*)info);
        event.Skip();
    }
}

void CslEngine::OnResolve(wxCommandEvent& event)
{
    CslServerInfo *info;
    CslGame *game=NULL;
    CslResolverPacket *packet=(CslResolverPacket*)event.GetClientData();

    if (packet->m_domain.IsEmpty())
        goto finish;

    loopv(m_games)
    {
        game=m_games[i];
        if (game->GetType()==packet->m_type)
        {
            wxMutexLocker lock(m_mutex);

            if ((info=game->FindServerByAddr(packet->m_address))!=NULL)
                info->m_domain=packet->m_domain;
            break;
        }
    }

finish:
    delete packet;
}
