///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021 Mihai Ursu                                                 //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

/*
main.cpp

This file contains the main application for monitoring data delivered by the
ViaSat SurfBeam 2 modem. It parses the strings from the following two URLs:

- Indoor unit  =>   192.168.100.1/index.cgi?page=modemStatusData
- Outdoor unit =>   192.168.100.1/index.cgi?page=triaStatusData
*/

#include "SurfBeam2.h"
#include <QApplication>

//!************************************************************************
//! Main application
//!
//! @returns: code returned by QApplication exec
//!************************************************************************
int main
    (
    int     argc,   //!< argument count
    char*   argv[]  //!< argument vector
    )
{
    QApplication a( argc, argv );
    SurfBeam2 w;
    w.show();
    return a.exec();
}
