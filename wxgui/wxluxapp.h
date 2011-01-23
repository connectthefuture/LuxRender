/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef LUX_WXLUXAPP_H
#define LUX_WXLUXAPP_H

#include "wxluxgui.h"

// The LuxApp class.
class LuxGuiApp : public wxApp {
public:
	virtual bool OnInit();

protected:
	bool ProcessCommandLine();

	LuxGui *m_guiFrame;

	// Command line options
	int m_threads;
	bool m_useServer, m_openglEnabled, m_copyLog2Console;
	wxString m_inputFile;
};

DECLARE_APP(LuxGuiApp)

#endif // LUX_WXLUXAPP_H