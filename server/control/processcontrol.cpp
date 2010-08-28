/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "processcontrol.h"
#include "akdebug.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDebug>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <signal.h>
#endif

#ifdef Q_OS_WINCE
#include <windows.h>
#endif

using namespace Akonadi;

ProcessControl::ProcessControl( QObject *parent )
  : QObject( parent ), mFailedToStart( false ), mCrashCount( 0 ),
  mRestartOnceOnExit( false )
{
  connect( &mProcess, SIGNAL( error( QProcess::ProcessError ) ),
           this, SLOT( slotError( QProcess::ProcessError ) ) );
  connect( &mProcess, SIGNAL( finished( int, QProcess::ExitStatus ) ),
           this, SLOT( slotFinished( int, QProcess::ExitStatus ) ) );
  mProcess.setProcessChannelMode( QProcess::ForwardedChannels );
}

ProcessControl::~ProcessControl()
{
  stop();
}

void ProcessControl::start( const QString &application, const QStringList &arguments, CrashPolicy policy )
{
  mFailedToStart = false;

  mApplication = application;
  mArguments = arguments;
  mPolicy = policy;

  start();
}

void ProcessControl::setCrashPolicy( CrashPolicy policy )
{
  mPolicy = policy;
}

void ProcessControl::stop()
{
  if ( mProcess.state() != QProcess::NotRunning ) {
    mProcess.waitForFinished( 10000 );
    mProcess.terminate();
  }
}

void ProcessControl::slotError( QProcess::ProcessError error )
{
  switch ( error ) {
    case QProcess::Crashed:
      // do nothing, we'll respawn in slotFinished
      break;
    case QProcess::FailedToStart:
    default:
        mFailedToStart = true;
      break;
  }

  qDebug( "ProcessControl: Application '%s' stopped unexpected (%s)",
          qPrintable( mApplication ), qPrintable( mProcess.errorString() ) );
}

void ProcessControl::slotFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
  if ( exitStatus == QProcess::CrashExit ) {
    if ( mPolicy == RestartOnCrash ) {
       // don't try to start an unstartable application
      if ( !mFailedToStart && --mCrashCount >= 0 ) {
        qDebug( "Application '%s' crashed! %d restarts left.", qPrintable( mApplication ), mCrashCount );
        start();
      } else {
        if ( mFailedToStart ) {
          qDebug( "Application '%s' failed to start!", qPrintable( mApplication ) );
        } else {
          qDebug( "Application '%s' crashed to often. Giving up!", qPrintable( mApplication ) );
        }
        mPolicy = StopOnCrash;
        emit unableToStart();
        return;
      }
    } else {
      qDebug( "Application '%s' crashed. No restart!", qPrintable( mApplication ) );
    }
  } else {
    if ( exitCode != 0 ) {
      qDebug( "ProcessControl: Application '%s' returned with exit code %d (%s)",
              qPrintable( mApplication ), exitCode, qPrintable( mProcess.errorString() ) );
#ifdef Q_OS_WINCE
	  // If Akonadi server crashed clean the registry key for the local socket.
	  // Qt insertes a regitsry key for each local socket opened and checks if a new
	  // socket is opened if some other socket is already bind, if a appication
	  // crashes the registru key is not deleted and no local socket can be opened
	  // so try to delete the key if it fails once
	  if ( mApplication == "akonadiserver" && exitCode == 1) {
	    HKEY hKey;
        long rc = RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Trolltech\\Qt\\QLocalServer"), 0, KEY_ALL_ACCESS, &hKey);
	    if(rc != ERROR_SUCCESS ) {
	      qWarning() << mApplication << "Tried to open HKCU\\Software\\Trolltech\\Qt\\QLocalServer to delete Key Akonadi";
	    } else {
	      rc =  RegDeleteValue( hKey, TEXT("Akonadi") );
          if(rc != ERROR_SUCCESS ) {
            qWarning() << mApplication << "Could not delete HKCU\\Software\\Trolltech\\Qt\\QLocalServer\\Akonadi";
		  }
	    }
	  }
#endif
      if ( mPolicy == RestartOnCrash ) {
        if ( mCrashCount > 2 ) {
          qWarning() << mApplication << "crashed too often and will not be restarted!";
          mPolicy = StopOnCrash;
          emit unableToStart();
          return;
        }
        ++mCrashCount;
        QTimer::singleShot( 60000, this, SLOT(resetCrashCount()) );
        if ( !mFailedToStart ) // don't try to start an unstartable application
          start();
      }
    } else {
      if ( mRestartOnceOnExit ) {
        mRestartOnceOnExit = false;
        qDebug( "Restarting application '%s'.", qPrintable( mApplication ) );
        start();
      } else {
        qDebug( "Application '%s' exited normally...", qPrintable( mApplication ) );
      }
    }
  }
}

void ProcessControl::start()
{
#ifdef Q_OS_UNIX
  QString agentValgrind = getEnv( "AKONADI_VALGRIND" );
  if ( !agentValgrind.isEmpty() && mApplication.contains( agentValgrind ) ) {

    mArguments.prepend( mApplication );
    const QString originalArguments = mArguments.join( QString::fromLocal8Bit( " " ) );
    mApplication = QString::fromLocal8Bit( "valgrind" );

    const QString valgrindSkin = getEnv( "AKONADI_VALGRIND_SKIN", QString::fromLocal8Bit( "memcheck" ) );
    mArguments.prepend( QLatin1String( "--tool=" ) + valgrindSkin );

    const QString valgrindOptions = getEnv( "AKONADI_VALGRIND_OPTIONS" );
    if ( !valgrindOptions.isEmpty() )
      mArguments = valgrindOptions.split( ' ', QString::SkipEmptyParts ) << mArguments;

    qDebug();
    qDebug() << "============================================================";
    qDebug() << "ProcessControl: Valgrinding process" << originalArguments;
    if ( !valgrindSkin.isEmpty() )
      qDebug() << "ProcessControl: Valgrind skin:" << valgrindSkin;
    if ( !valgrindOptions.isEmpty() )
      qDebug() << "ProcessControl: Additional Valgrind options:" << valgrindOptions;
    qDebug() << "============================================================";
    qDebug();
  }
#endif

  mProcess.start( mApplication, mArguments );
  if ( !mProcess.waitForStarted( ) ) {
    qDebug( "ProcessControl: Unable to start application '%s' (%s)",
            qPrintable( mApplication ), qPrintable( mProcess.errorString() ) );
    return;
  }

#ifdef Q_OS_UNIX
  else {
    QString agentDebug = QString::fromLocal8Bit( qgetenv( "AKONADI_DEBUG_WAIT" ) );
    pid_t pid = mProcess.pid();
    if ( !agentDebug.isEmpty() && mApplication.contains( agentDebug ) ) {
      qDebug();
      qDebug() << "============================================================";
      qDebug() << "ProcessControl: Suspending process" << mApplication;
      qDebug() << "'gdb" << pid << "' to debug";
      qDebug() << "'kill -SIGCONT" << pid << "' to continue";
      qDebug() << "============================================================";
      qDebug();
      kill( pid, SIGSTOP );
    }
  }
#endif
}

void ProcessControl::resetCrashCount()
{
  mCrashCount = 0;
}

#include "processcontrol.moc"
