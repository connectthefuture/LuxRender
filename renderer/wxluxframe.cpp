///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wxluxframe.h"

///////////////////////////////////////////////////////////////////////////
using namespace lux;

LuxMainFrame::LuxMainFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_menubar = new wxMenuBar( 0 );
	m_file = new wxMenu();
	wxMenuItem* m_open;
	m_open = new wxMenuItem( m_file, wxID_OPEN, wxString( wxT("&Open") ) + wxT('\t') + wxT("CTRL+O"), wxT("Open a scene"), wxITEM_NORMAL );
	m_file->Append( m_open );
	
	m_file->AppendSeparator();
	
	wxMenuItem* m_exit;
	m_exit = new wxMenuItem( m_file, wxID_EXIT, wxString( wxT("&Exit") ) + wxT('\t') + wxT("ALT+F4"), wxT("Exit LuxRender"), wxITEM_NORMAL );
	m_file->Append( m_exit );
	
	m_menubar->Append( m_file, wxT("&File") );
	
	m_render = new wxMenu();
	wxMenuItem* m_resume;
	m_resume = new wxMenuItem( m_render, ID_RESUMEITEM, wxString( wxT("&Resume") ) + wxT('\t') + wxT("CTRL+S"), wxT("Resume rendering"), wxITEM_NORMAL );
	m_render->Append( m_resume );
	
	wxMenuItem* m_stop;
	m_stop = new wxMenuItem( m_render, ID_STOPITEM, wxString( wxT("S&top") ) + wxT('\t') + wxT("CTRL+T"), wxT("Stop current rendering"), wxITEM_NORMAL );
	m_render->Append( m_stop );
	
	m_menubar->Append( m_render, wxT("&Render") );
	
	m_help = new wxMenu();
	wxMenuItem* m_about;
	m_about = new wxMenuItem( m_help, wxID_ABOUT, wxString( wxT("&About") ) + wxT('\t') + wxT("F1"), wxT("Show about dialog"), wxITEM_NORMAL );
	m_help->Append( m_about );
	
	m_menubar->Append( m_help, wxT("Help") );
	
	this->SetMenuBar( m_menubar );
	
	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer( wxVERTICAL );
	
	m_auinotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT );
	m_renderPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRenderSizer;
	bRenderSizer = new wxBoxSizer( wxVERTICAL );
	
	m_renderToolBar = new wxToolBar( m_renderPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_TEXT ); 
	m_renderToolBar->AddTool( ID_RESUMETOOL, wxT("Resume"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxT("Resume rendering"), wxT("Resume (CTRL-S)") );
	m_renderToolBar->AddTool( ID_STOPTOOL, wxT("Stop"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxT("Stop (CTRL-T)"), wxT("Stop current rendering") );
	m_renderToolBar->Realize();
	
	bRenderSizer->Add( m_renderToolBar, 0, wxEXPAND, 5 );
	
	m_renderPage->SetSizer( bRenderSizer );
	m_renderPage->Layout();
	bRenderSizer->Fit( m_renderPage );
	m_auinotebook->AddPage( m_renderPage, wxT("Render"), true, wxNullBitmap );
	m_logPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLogSizer;
	bLogSizer = new wxBoxSizer( wxVERTICAL );
	
	m_logTextCtrl = new wxTextCtrl( m_logPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_WORDWRAP );
	bLogSizer->Add( m_logTextCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_logPage->SetSizer( bLogSizer );
	m_logPage->Layout();
	bLogSizer->Fit( m_logPage );
	m_auinotebook->AddPage( m_logPage, wxT("Log"), false, wxNullBitmap );
	m_outputPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bOutputSizer;
	bOutputSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText = new wxStaticText( m_outputPage, wxID_ANY, wxT("Output tab not implemented yet."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	bOutputSizer->Add( m_staticText, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_outputPage->SetSizer( bOutputSizer );
	m_outputPage->Layout();
	bOutputSizer->Fit( m_outputPage );
	m_auinotebook->AddPage( m_outputPage, wxT("Output"), false, wxNullBitmap );
	
	bSizer->Add( m_auinotebook, 1, wxEXPAND | wxALL, 5 );
	
	this->SetSizer( bSizer );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	m_toolBar2 = this->CreateToolBar( wxTB_HORIZONTAL|wxTB_TEXT, wxID_ANY ); 
	m_toolBar2->AddTool( wxID_ANY, wxT("tool"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_toolBar2->Realize();
	
	
	// Connect Events
	this->Connect( m_open->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnOpen ) );
	this->Connect( m_exit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnExit ) );
	this->Connect( m_resume->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_stop->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_about->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
}

LuxMainFrame::~LuxMainFrame()
{
	// Disconnect Events
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnOpen ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnExit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
}