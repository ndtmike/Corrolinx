/////////////////////////////////////////////////////////////////////////////
// Name:        corrolinx_view.cpp
// Purpose:     Implements view functionality for
//              View Map Data from Cor-Map and Cor-Map II, based on docview sample
// Author:      Michael Hoag
// Created:     03/19/15
// Copyright:   (c) 2015 Michael Hoag
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#if !wxUSE_DOC_VIEW_ARCHITECTURE
    #error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!
#endif

#include "corrolinx.h"
#include "corrolinx_doc.h"
#include "corrolinx_view.h"

// ----------------------------------------------------------------------------
// DrawingView implementation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(DrawingView, wxView)

wxBEGIN_EVENT_TABLE(DrawingView, wxView)
    EVT_MENU(wxID_CUT, DrawingView::OnCut)
wxEND_EVENT_TABLE()

// What to do when a view is created. Creates actual
// windows for displaying the view.
bool DrawingView::OnCreate(wxDocument *doc, long flags)
{
    if ( !wxView::OnCreate(doc, flags) )
        return false;

    MyApp& app = wxGetApp();
    wxFrame* frame = app.CreateChildFrame(this, true);
    wxASSERT(frame == GetFrame());
    m_canvas = new MyCanvas(this);
    frame->Show();

    return true;
}

// Sneakily gets used for default print/preview as well as drawing on the
// screen.
void DrawingView::OnDraw(wxDC *dc)
{
    dc->SetPen(*wxBLACK_PEN);

    // simply draw all lines of all segments
    const DoodleSegments& segments = GetDocument()->GetSegments();
    for ( DoodleSegments::const_iterator i = segments.begin();
          i != segments.end();
          ++i )
    {
        const DoodleLines& lines = i->GetLines();
        for ( DoodleLines::const_iterator j = lines.begin();
              j != lines.end();
              ++j )
        {
            const DoodleLine& line = *j;

            dc->DrawLine(line.x1, line.y1, line.x2, line.y2);
        }
    }
}

DrawingDocument* DrawingView::GetDocument()
{
    return wxStaticCast(wxView::GetDocument(), DrawingDocument);
}

void DrawingView::OnUpdate(wxView* sender, wxObject* hint)
{
    wxView::OnUpdate(sender, hint);
    if ( m_canvas )
        m_canvas->Refresh();
}

// Clean up windows used for displaying the view.
bool DrawingView::OnClose(bool deleteWindow)
{
    if ( !wxView::OnClose(deleteWindow) )
        return false;

    Activate(false);

    if ( deleteWindow )
    {
        GetFrame()->Destroy();
        SetFrame(NULL);
    }
    return true;
}

void DrawingView::OnCut(wxCommandEvent& WXUNUSED(event) )
{
    DrawingDocument * const doc = GetDocument();

    doc->GetCommandProcessor()->Submit(new DrawingRemoveSegmentCommand(doc));
}

// ----------------------------------------------------------------------------
// TextEditView implementation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(TextEditView, wxView)

wxBEGIN_EVENT_TABLE(TextEditView, wxView)
    EVT_MENU(wxID_COPY, TextEditView::OnCopy)
    EVT_MENU(wxID_PASTE, TextEditView::OnPaste)
    EVT_MENU(wxID_SELECTALL, TextEditView::OnSelectAll)
wxEND_EVENT_TABLE()

bool TextEditView::OnCreate(wxDocument *doc, long flags)
{
    if ( !wxView::OnCreate(doc, flags) )
        return false;

    wxFrame* frame = wxGetApp().CreateChildFrame(this, false);
    wxASSERT(frame == GetFrame());
    m_text = new wxTextCtrl(frame, wxID_ANY, "",
                            wxDefaultPosition, wxDefaultSize,
                            wxTE_MULTILINE);
    frame->Show();

    return true;
}

void TextEditView::OnDraw(wxDC *WXUNUSED(dc))
{
    // nothing to do here, wxTextCtrl draws itself
}

bool TextEditView::OnClose(bool deleteWindow)
{
    if ( !wxView::OnClose(deleteWindow) )
        return false;

    Activate(false);

    if ( deleteWindow )
    {
        GetFrame()->Destroy();
        SetFrame(NULL);
    }
    return true;
}

// ----------------------------------------------------------------------------
// MyCanvas implementation
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(MyCanvas, wxScrolledWindow)
    EVT_MOUSE_EVENTS(MyCanvas::OnMouseEvent)
wxEND_EVENT_TABLE()

// Define a constructor for my canvas
MyCanvas::MyCanvas(wxView *view, wxWindow *parent)
    : wxScrolledWindow(parent ? parent : view->GetFrame())
{
    m_view = view;
    m_currentSegment = NULL;
    m_lastMousePos = wxDefaultPosition;

    SetCursor(wxCursor(wxCURSOR_PENCIL));

    // this is completely arbitrary and is done just for illustration purposes
    SetVirtualSize(1000, 1000);
    SetScrollRate(20, 20);

    SetBackgroundColour(*wxWHITE);
}

MyCanvas::~MyCanvas()
{
    delete m_currentSegment;
}

// Define the repainting behaviour
void MyCanvas::OnDraw(wxDC& dc)
{
    if ( m_view )
        m_view->OnDraw(& dc);
}

// This implements a tiny doodling program. Drag the mouse using the left
// button.
void MyCanvas::OnMouseEvent(wxMouseEvent& event)
{
    if ( !m_view )
        return;

    wxClientDC dc(this);
    PrepareDC(dc);

    dc.SetPen(*wxBLACK_PEN);

    const wxPoint pt(event.GetLogicalPosition(dc));

    // is this the end of the current segment?
    if ( m_currentSegment && event.LeftUp() )
    {
        if ( !m_currentSegment->IsEmpty() )
        {
            // We've got a valid segment on mouse left up, so store it.
            DrawingDocument * const
                doc = wxStaticCast(m_view->GetDocument(), DrawingDocument);

            doc->GetCommandProcessor()->Submit(
                new DrawingAddSegmentCommand(doc, *m_currentSegment));

            doc->Modify(true);
        }

        wxDELETE(m_currentSegment);
    }

    // is this the start of a new segment?
    if ( m_lastMousePos != wxDefaultPosition && event.Dragging() )
    {
        if ( !m_currentSegment )
            m_currentSegment = new DoodleSegment;

        m_currentSegment->AddLine(m_lastMousePos, pt);

        dc.DrawLine(m_lastMousePos, pt);
    }

    m_lastMousePos = pt;
}
