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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/platinfo.h>
#include <wx/regex.h>
#include "CslVersion.h"
#include "CslTools.h"

wxString g_basePath;

#ifdef __WXDEBUG__
void Debug_Printf(const char *DbgFunc, const char *FmtStr,...)
{
    va_list ArgList;
    va_start(ArgList,FmtStr);
    fprintf(stdout,"%s(): ",DbgFunc);
    vfprintf(stdout,FmtStr,ArgList);
    fflush(stdout);
    va_end(ArgList);
}
#endif

char* StripColours(char *s,wxUint32 *l,wxUint32 count)
{
    wxUint32 i=0;

    if (!*l)
        return NULL;

    while (i<*l)
    {
        if (s[i]==0xc)
        {
            *l-=count;
            memmove((void*)&s[i],(void*)&s[i+count],*l-i);
        }
        else if (s[i]=='\r' || s[i]=='\n')
        {
            *l-=1;
            memmove((void*)&s[i],(void*)&s[i+count],*l-i);
        }
        else
            i++;
    }

    s[*l]=0;

    return s;
}

bool IsIP(const wxString& s)
{
    const wxChar* dot=wxT(".");
    wxString digit=wxT("0*[0-9]{1,3}");
    wxString exp=wxT("^")+digit+dot+digit+dot+digit+dot+digit+wxT("$");
    wxRegEx regex;

    regex.Compile(exp);
    return regex.Matches(s);
}

bool IP2Int(const wxString& s,wxUint32 *ip)
{
    long unsigned int l;
    wxUint32 mult=0x1000000;
    wxUint32 len=s.Len();
    wxUint32 i=0,v=0;
    wxString m;

    for (;i<=len;i++)
    {
        if (i<len && s.Mid(i,1).IsNumber())
            m+=s.Mid(i,1);
        else
        {
            m.ToULong(&l,10);
            if (l>255)
                return false;
            v+=l*mult;
            mult>>=8;
            m.Empty();
        }
    }

    *ip=v;
    return true;
}

wxString FormatBytes(wxUint64 size)
{
    if (size>0x100000)
    {
#ifdef USE_FLOAT_PRECISION
        float s=(float)size/0x100000;
        return wxString::Format(wxT("%.2f MiB"),s);
#else
        wxUint64 m=size/0x100000;
        wxUint64 k=(size%0x100000)*100/0x100000;
        return wxString::Format(wxT("%lli.%-2.2lli MiB"),m,k);
#endif

    }
    else if (size>0x400)
        return wxString::Format(wxT("%lli KiB"),size/0x400);
    else
        return wxString::Format(wxT("%lli B"),size);
}

wxString FormatSeconds(wxUint32 time,bool space,bool full)
{
    wxString s;
    wxUint32 dy,hr,mn,sc;
    wxUint32 rest=time;

    dy=rest/86400;
    rest%=86400;
    hr=rest/3600;
    rest%=3600;
    mn=rest/60;
    sc=rest%60;

    if (dy)
    {
        s+=s.Format(wxT("%d"),dy);
        if (space)
            s+=wxT(" ");
        if (full)
        {
            if (dy==1)
                s+=_("day");
            else
                s+=_("days");
        }
        else
            s+=_("d");
        s+=wxT(" ");
    }
    if (hr)
    {
        s+=s.Format(wxT("%d"),hr);
        if (space)
            s+=wxT(" ");
        if (full)
        {
            if (hr==1)
                s+=_("hour");
            else
                s+=_("hours");
        }
        else
            s+=_("h");
        s+=wxT(" ");
    }
    if (mn)
    {
        s+=s.Format(wxT("%d"),mn);
        if (space)
            s+=wxT(" ");
        if (full)
        {
            if (mn==1)
                s+=_("minute");
            else
                s+=_("minutes");
        }
        else
            s+=_("min");
        s+=wxT(" ");
    }
    if (sc)
    {
        s+=s.Format(wxT("%d"),sc);
        if (space)
            s+=wxT(" ");
        if (full)
        {
            if (sc==1)
                s+=_("second");
            else
                s+=_("seconds");
        }
        else
            s+=_("sec");
    }

    return s;
}


#define CSL_USE_WIN32_TICK

#ifdef __WXMSW__
#ifndef CSL_USE_WIN32_TICK
#include <time.h>
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
struct timeval
{
    long tv_sec;
    long tv_usec;
};
#endif
int gettimeofday(struct timeval* tv,void *dummy)
{
    union
    {
        long long ns100;
        FILETIME ft;
    } now;

    GetSystemTimeAsFileTime(&now.ft);
    tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
    tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL);
    return 0;
}
#endif //USE_WIN32_TICK
#else
#include <sys/time.h>
#endif //__WXMSW__

wxUint32 GetTicks()
{
    static wxUint32 initTicks=0;
    wxUint32 ticks;
#if defined(__WXMSW__) && defined(CSL_USE_WIN32_TICK)
    ticks=GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv,NULL);
    wxUint32 r=tv.tv_usec/1000;
    wxUint64 v=tv.tv_sec*1000;
    ticks=v+r-initTicks;
#endif
    if (!initTicks)
    {
        initTicks=ticks;
        return 0;
    }
    return ticks;
}

#ifdef __WXMSW__
wxBitmap AdjustIconSize(const char **data,const wxIcon& icon,
                        const wxSize& size,const wxPoint& origin)
{
    wxBitmap bitmap;
    if (data)
        bitmap=wxBitmap(data);
    else
        bitmap=wxBitmap(icon);
    wxImage image=bitmap.ConvertToImage();
    image.Resize(size,origin);
    return wxBitmap(image);
}
#endif

wxString GetHttpAgent()
{
    wxPlatformInfo pinfo;
    wxOperatingSystemId id=pinfo.GetOperatingSystemId();
    wxString agent=wxT("Csl/");
    agent+=CSL_VERSION_LONG_STR;
    agent+=wxT("(")+pinfo.GetOperatingSystemFamilyName(id)+wxT(")");
    return agent;
}

wxInt32 WriteTextFile(const wxString& filename,const wxString& data,const wxFile::OpenMode mode)
{
    wxFile file(filename,mode);

    if (!file.IsOpened())
        return CSL_ERROR_FILE_OPERATION;

    wxUint32 l=data.Length();
    wxUint32 w=file.Write(U2A(data),l);
    file.Close();

    return l!=w ? CSL_ERROR_FILE_OPERATION:CSL_ERROR_NONE;
}