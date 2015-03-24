/////////////////////////////////////////////////////////////////////////////
// Name:        corrolinx
// Purpose:     View Map Data from Cor-Map and Cor-Map II, based on docview sample
// Author:      Michael Hoag
// Created:     03/18/2015
// Copyright:   (c) 2015 Michael Hoag
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
    This sample show document/view support in wxWidgets.

    It can be run in several ways:
        * Windows -- With "--mdi" command line option to use multiple MDI child frames
          for the multiple documents (this is the default).
        * GTK -- With "--sdi" command line option to use multiple top level windows
          for the multiple documents
        * With "--single" command line option to support opening a single
          document only

    Notice that doing it like this somewhat complicates the code, you could
    make things much simpler in your own programs by using either
    wxDocParentFrame or wxDocMDIParentFrame unconditionally (and never using
    the single mode) instead of supporting all of them as this sample does.
 */

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include "wx/stockitem.h"
#endif

#if !wxUSE_DOC_VIEW_ARCHITECTURE
    #error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!
#endif

#include "wx/docview.h"
#include "wx/docmdi.h"

#include "corrolinx.h"
#include "corrolinx_doc.h"
#include "corrolinx_view.h"

#include "wx/cmdline.h"
#include "wx/config.h"

#ifdef __WXMAC__
    #include "wx/filename.h"
#endif

#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "doc.xpm"
    #include "chart.xpm"
    #include "notepad.xpm"
#endif

// ----------------------------------------------------------------------------
// MyApp implementation
// ----------------------------------------------------------------------------

IMPLEMENT_APP(MyApp)

wxBEGIN_EVENT_TABLE(MyApp, wxApp)
    EVT_MENU(wxID_ABOUT, MyApp::OnAbout)
wxEND_EVENT_TABLE()

MyApp::MyApp()
{
#if wxUSE_MDI_ARCHITECTURE
    m_mode = Mode_MDI;
#else
    m_mode = Mode_SDI;
#endif

    m_canvas = NULL;
    m_menuEdit = NULL;
}

// constants for the command line options names
namespace CmdLineOption
{

const char * const MDI = "mdi";
const char * const SDI = "sdi";

} // namespace CmdLineOption

bool MyApp::OnInit()
{
#if wxUSE_MDI_ARCHITECTURE && _WINDOWS_
    m_mode = Mode_MDI;
#else
    m_mode = Mode_SDI;
#endif

    if ( !wxApp::OnInit() )
        return false;

    ::wxInitAllImageHandlers();

    // Fill in the application information fields before creating wxConfig.
    SetVendorName("James Instruments Inc.");
    SetAppName("Corrolinx");
    SetAppDisplayName("Corrolinx Corrosion Mapping System");

    //// Create a document manager
    wxDocManager *docManager = new wxDocManager;

    //// Create a template relating drawing documents to their views
    new wxDocTemplate(docManager, "Drawing", "*.drw", "", "drw",
                      "Drawing Doc", "Drawing View",
                      CLASSINFO(DrawingDocument), CLASSINFO(DrawingView));
#if defined( __WXMAC__ )  && wxOSX_USE_CARBON
    wxFileName::MacRegisterDefaultTypeAndCreator("drw" , 'WXMB' , 'WXMA');
#endif

//    else // multiple documents mode: allow documents of different types
    {
        // Create a template relating text documents to their views
        new wxDocTemplate(docManager, "Text", "*.txt;*.text", "", "txt;text",
                          "Text Doc", "Text View",
                          CLASSINFO(TextEditDocument), CLASSINFO(TextEditView));
#if defined( __WXMAC__ ) && wxOSX_USE_CARBON
        wxFileName::MacRegisterDefaultTypeAndCreator("txt" , 'TEXT' , 'WXMA');
#endif

    }

    // create the main frame window
    wxFrame *frame;
#if wxUSE_MDI_ARCHITECTURE && _WINDOWS_
    if ( m_mode == Mode_MDI )
    {
        frame = new wxDocMDIParentFrame(docManager, NULL, wxID_ANY,
                                        GetAppDisplayName(),
                                        wxDefaultPosition,
                                        wxSize(500, 400));
    }
    else
#endif // wxUSE_MDI_ARCHITECTURE && _WINDOWS_
    {
        frame = new wxDocParentFrame(docManager, NULL, wxID_ANY,
                                     GetAppDisplayName(),
                                     wxDefaultPosition,
                                     wxSize(500, 400));
    }

    // and its menu bar
    wxMenu *menuFile = new wxMenu;

    menuFile->Append(wxID_NEW);
    menuFile->Append(wxID_OPEN);

    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    // A nice touch: a history of files visited. Use this menu.
    docManager->FileHistoryUseMenu(menuFile);
#if wxUSE_CONFIG
    docManager->FileHistoryLoad(*wxConfig::Get());
#endif // wxUSE_CONFIG

    CreateMenuBarForFrame(frame, menuFile, m_menuEdit);

    frame->SetIcon(wxICON(doc));
    frame->Centre();
    frame->Show();

    return true;
}

int MyApp::OnExit()
{
    wxDocManager * const manager = wxDocManager::GetDocumentManager();
#if wxUSE_CONFIG
    manager->FileHistorySave(*wxConfig::Get());
#endif // wxUSE_CONFIG
    delete manager;

    return wxApp::OnExit();
}

void MyApp::AppendDocumentFileCommands(wxMenu *menu, bool supportsPrinting)
{
    menu->Append(wxID_CLOSE);
    menu->Append(wxID_SAVE);
    menu->Append(wxID_SAVEAS);
    menu->Append(wxID_REVERT, _("Re&vert..."));

    if ( supportsPrinting )
    {
        menu->AppendSeparator();
        menu->Append(wxID_PRINT);
        menu->Append(wxID_PRINT_SETUP, "Print &Setup...");
        menu->Append(wxID_PREVIEW);
    }
}

wxMenu *MyApp::CreateDrawingEditMenu()
{
    wxMenu * const menu = new wxMenu;
    menu->Append(wxID_UNDO);
    menu->Append(wxID_REDO);
    menu->AppendSeparator();
    menu->Append(wxID_CUT, "&Cut last segment");

    return menu;
}

void MyApp::CreateMenuBarForFrame(wxFrame *frame, wxMenu *file, wxMenu *edit)
{
    wxMenuBar *menubar = new wxMenuBar;

    menubar->Append(file, wxGetStockLabel(wxID_FILE));

    if ( edit )
        menubar->Append(edit, wxGetStockLabel(wxID_EDIT));

    wxMenu *help= new wxMenu;
    help->Append(wxID_ABOUT);
    menubar->Append(help, wxGetStockLabel(wxID_HELP));

    frame->SetMenuBar(menubar);
}

wxFrame *MyApp::CreateChildFrame(wxView *view, bool isCanvas)
{
    // create a child frame of appropriate class for the current mode
    wxFrame *subframe;
    wxDocument *doc = view->GetDocument();
#if wxUSE_MDI_ARCHITECTURE
    if ( GetMode() == Mode_MDI )
    {
        subframe = new wxDocMDIChildFrame
                       (
                            doc,
                            view,
                            wxStaticCast(GetTopWindow(), wxDocMDIParentFrame),
                            wxID_ANY,
                            "Child Frame",
                            wxDefaultPosition,
                            wxSize(300, 300)
                       );
    }
    else
#endif // wxUSE_MDI_ARCHITECTURE && _WINDOWS_
    {
        subframe = new wxDocChildFrame
                       (
                            doc,
                            view,
                            wxStaticCast(GetTopWindow(), wxDocParentFrame),
                            wxID_ANY,
                            "Child Frame",
                            wxDefaultPosition,
                            wxSize(300, 300)
                       );

        subframe->Centre();
    }

    wxMenu *menuFile = new wxMenu;

    menuFile->Append(wxID_NEW);
    menuFile->Append(wxID_OPEN);
    AppendDocumentFileCommands(menuFile, isCanvas);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuEdit;
    if ( isCanvas )
    {
        menuEdit = CreateDrawingEditMenu();

        doc->GetCommandProcessor()->SetEditMenu(menuEdit);
        doc->GetCommandProcessor()->Initialize();
    }
    else // text frame
    {
        menuEdit = new wxMenu;
        menuEdit->Append(wxID_COPY);
        menuEdit->Append(wxID_PASTE);
        menuEdit->Append(wxID_SELECTALL);
    }

    CreateMenuBarForFrame(subframe, menuFile, menuEdit);

    subframe->SetIcon(isCanvas ? wxICON(chrt) : wxICON(notepad));

    return subframe;
}

void MyApp::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString modeName;
    switch ( m_mode )
    {
#if wxUSE_MDI_ARCHITECTURE && _WINDOWS_
        case Mode_MDI:
            modeName = "MDI";
            break;
#endif // wxUSE_MDI_ARCHITECTURE && _WINDOWS_

        case Mode_SDI:
            modeName = "SDI";
            break;

        default:
            wxFAIL_MSG( "unknown mode ");
    }

#ifdef __VISUALC6__
    const int docsCount =
        wxDocManager::GetDocumentManager()->GetDocuments().GetCount();
#else
    const int docsCount =
        wxDocManager::GetDocumentManager()->GetDocumentsVector().size();
#endif

    wxLogMessage
    (
        "This is the Corrolinx Software\n"
        "running in %s mode.\n"
        "%d open documents.\n"
        "\n"
        "Authors: Michael Hoag\n"
        "\n"
        "Usage: docview [--{mdi,sdi,single}]",
        modeName,
        docsCount
    );
}
