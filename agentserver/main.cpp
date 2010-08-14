/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "agentserver.h"

#include "shared/akapplication.h"
#include "shared/akdebug.h"

#include "libs/protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>

#ifndef _WIN32_WCE
namespace po = boost::program_options;
#endif

int main( int argc, char ** argv )
{
    AkApplication app( argc, argv );
    app.setDescription( QLatin1String( "Akonadi Agent Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi." ) );

#if !defined(NDEBUG) && !defined(_WIN32_WCE)
    po::options_description debugOptions( "Debug options (use with care)" );
    debugOptions.add_options()
        ( "start-without-control", "Allow to start the Akonadi server even without the Akonadi control process being available" );
    app.addCommandLineOptions( debugOptions );
#endif

    app.parseCommandLine();

    //Needed for wince build
    #undef interface

#ifndef _WIN32_WCE
   if ( !app.commandLineArguments().count( "start-without-control" ) &&
#else
   if (
#endif
        !QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String(AKONADI_DBUS_CONTROL_SERVICE_LOCK) ) ) {
     akError() << "Akonadi control process not found - aborting.";
     akFatal() << "If you started akonadi_agent_server manually, try 'akonadictl start' instead.";
   }

    new Akonadi::AgentServer;

    if ( !QDBusConnection::sessionBus().registerService( QLatin1String(AKONADI_DBUS_AGENTSERVER_SERVICE) ) )
      akFatal() << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();

    const int result = app.exec();
    return result;
}
