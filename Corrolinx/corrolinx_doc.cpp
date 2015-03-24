/////////////////////////////////////////////////////////////////////////////
// Name:        corrolinx_doc.cpp
// Purpose:     Implements document functionality for
//              View Map Data from Cor-Map and Cor-Map II, based on docview sample
// Author:      Michael Hoag
// Created:     03/19/15
// Copyright:   (c) 2015 Michael Hoag
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if !wxUSE_DOC_VIEW_ARCHITECTURE
    #error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#if wxUSE_STD_IOSTREAM
    #include "wx/ioswrap.h"
#else
    #include "wx/txtstrm.h"
#endif
#include "wx/wfstream.h"

#include "corrolinx_doc.h"
#include "corrolinx_view.h"

// ----------------------------------------------------------------------------
// DrawingDocument implementation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(DrawingDocument, wxDocument)

DocumentOstream& DrawingDocument::SaveObject(DocumentOstream& ostream)
{
#if wxUSE_STD_IOSTREAM
    DocumentOstream& stream = ostream;
#else
    wxTextOutputStream stream(ostream);
#endif

    wxDocument::SaveObject(ostream);

    const wxInt32 count = m_doodleSegments.size();
    stream << count << '\n';

    for ( int n = 0; n < count; n++ )
    {
        m_doodleSegments[n].SaveObject(ostream);
        stream << '\n';
    }

    return ostream;
}

DocumentIstream& DrawingDocument::LoadObject(DocumentIstream& istream)
{
#if wxUSE_STD_IOSTREAM
    DocumentIstream& stream = istream;
#else
    wxTextInputStream stream(istream);
#endif

    wxDocument::LoadObject(istream);

    wxInt32 count = 0;
    stream >> count;
    if ( count < 0 )
    {
        wxLogWarning("Drawing document corrupted: invalid segments count.");
#if wxUSE_STD_IOSTREAM
        istream.clear(std::ios::badbit);
#else
        istream.Reset(wxSTREAM_READ_ERROR);
#endif
        return istream;
    }

    for ( int n = 0; n < count; n++ )
    {
        DoodleSegment segment;
        segment.LoadObject(istream);
        m_doodleSegments.push_back(segment);
    }

    return istream;
}

void DrawingDocument::DoUpdate()
{
    Modify(true);
    UpdateAllViews();
}

void DrawingDocument::AddDoodleSegment(const DoodleSegment& segment)
{
    m_doodleSegments.push_back(segment);

    DoUpdate();
}

bool DrawingDocument::PopLastSegment(DoodleSegment *segment)
{
    if ( m_doodleSegments.empty() )
        return false;

    if ( segment )
        *segment = m_doodleSegments.back();

    m_doodleSegments.pop_back();

    DoUpdate();

    return true;
}

// ----------------------------------------------------------------------------
// DoodleSegment implementation
// ----------------------------------------------------------------------------

DocumentOstream& DoodleSegment::SaveObject(DocumentOstream& ostream)
{
#if wxUSE_STD_IOSTREAM
    DocumentOstream& stream = ostream;
#else
    wxTextOutputStream stream(ostream);
#endif

    const wxInt32 count = m_lines.size();
    stream << count << '\n';

    for ( int n = 0; n < count; n++ )
    {
        const DoodleLine& line = m_lines[n];
        stream
            << line.x1 << ' '
            << line.y1 << ' '
            << line.x2 << ' '
            << line.y2 << '\n';
    }

    return ostream;
}

DocumentIstream& DoodleSegment::LoadObject(DocumentIstream& istream)
{
#if wxUSE_STD_IOSTREAM
    DocumentIstream& stream = istream;
#else
    wxTextInputStream stream(istream);
#endif

    wxInt32 count = 0;
    stream >> count;

    for ( int n = 0; n < count; n++ )
    {
        DoodleLine line;
        stream
            >> line.x1
            >> line.y1
            >> line.x2
            >> line.y2;
        m_lines.push_back(line);
    }

    return istream;
}

// ----------------------------------------------------------------------------
// wxTextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxTextDocument, wxDocument)

bool wxTextDocument::OnCreate(const wxString& path, long flags)
{
    if ( !wxDocument::OnCreate(path, flags) )
        return false;

    // subscribe to changes in the text control to update the document state
    // when it's modified
    GetTextCtrl()->Connect
    (
        wxEVT_TEXT,
        wxCommandEventHandler(wxTextDocument::OnTextChange),
        NULL,
        this
    );

    return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool wxTextDocument::DoSaveDocument(const wxString& filename)
{
    return GetTextCtrl()->SaveFile(filename);
}

bool wxTextDocument::DoOpenDocument(const wxString& filename)
{
    if ( !GetTextCtrl()->LoadFile(filename) )
        return false;

    // we're not modified by the user yet
    Modify(false);

    return true;
}

bool wxTextDocument::IsModified() const
{
    wxTextCtrl* wnd = GetTextCtrl();
    return wxDocument::IsModified() || (wnd && wnd->IsModified());
}

void wxTextDocument::Modify(bool modified)
{
    wxDocument::Modify(modified);

    wxTextCtrl* wnd = GetTextCtrl();
    if (wnd && !modified)
    {
        wnd->DiscardEdits();
    }
}

void wxTextDocument::OnTextChange(wxCommandEvent& event)
{
    Modify(true);

    event.Skip();
}

// ----------------------------------------------------------------------------
// TextEditDocument implementation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(TextEditDocument, wxDocument)

wxTextCtrl* TextEditDocument::GetTextCtrl() const
{
    wxView* view = GetFirstView();
    return view ? wxStaticCast(view, TextEditView)->GetText() : NULL;
}
