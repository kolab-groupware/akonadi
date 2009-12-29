/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "akonadi.h"
#include "akonadiconnection.h"
#include "serveradaptor.h"
#include "akdebug.h"

#include "cachecleaner.h"
#include "intervalcheck.h"
#include "storage/dbconfig.h"
#include "storage/datastore.h"
#include "notificationmanager.h"
#include "resourcemanager.h"
#include "tracer.h"
#include "xesammanager.h"
#include "nepomukmanager.h"
#include "debuginterface.h"
#include "storage/itemretrievalthread.h"
#include "preprocessormanager.h"

#include "libs/xdgbasedirs_p.h"
#include "libs/protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>

using namespace Akonadi;

static AkonadiServer *s_instance = 0;

AkonadiServer::AkonadiServer( QObject* parent )
    : QLocalServer( parent )
    , mCacheCleaner( 0 )
    , mIntervalChecker( 0 )
    , mItemRetrievalThread( 0 )
    , mDatabaseProcess( 0 )
    , mAlreadyShutdown( false )
{
    DbConfig::init();
    if ( DbConfig::useInternalServer() )
      startDatabaseProcess();
    else
      createDatabase();

    s_instance = this;

    const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
    QSettings settings( serverConfigFile, QSettings::IniFormat );

    const QString connectionSettingsFile = XdgBaseDirs::akonadiConnectionConfigFile( XdgBaseDirs::WriteOnly );
    QSettings connectionSettings( connectionSettingsFile, QSettings::IniFormat );

#ifdef Q_OS_WIN
    QString namedPipe = settings.value( QLatin1String( "Connection/NamedPipe" ), QLatin1String( "Akonadi" ) ).toString();
    if ( !listen( namedPipe ) )
      akFatal() << "Unable to listen on Named Pipe" << namedPipe;

    connectionSettings.setValue( QLatin1String( "Data/Method" ), QLatin1String( "NamedPipe" ) );
    connectionSettings.setValue( QLatin1String( "Data/NamedPipe" ), namedPipe );
#else
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    QString socketDir = settings.value( QLatin1String( "Connection/SocketDirectory" ), defaultSocketDir ).toString();
    if ( socketDir[0] != QLatin1Char( '/' ) ) {
      QDir::home().mkdir( socketDir );
      socketDir = QDir::homePath() + QLatin1Char( '/' ) + socketDir;
    }

    const QString socketFile = socketDir + QLatin1String( "/akonadiserver.socket" );
    unlink( socketFile.toUtf8().constData() );
    if ( !listen( socketFile ) )
      akFatal() << "Unable to listen on Unix socket" << socketFile;

    connectionSettings.setValue( QLatin1String( "Data/Method" ), QLatin1String( "UnixPath" ) );
    connectionSettings.setValue( QLatin1String( "Data/UnixPath" ), socketFile );
#endif

    // initialize the database
    DataStore *db = DataStore::self();
    if ( !db->database().isOpen() )
      akFatal() << "Unable to open database" << db->database().lastError().text();
    if ( !db->init() )
      akFatal() << "Unable to initialize database.";

    NotificationManager::self();
    Tracer::self();
    new DebugInterface( this );
    ResourceManager::self();

    // Initialize the preprocessor manager
    PreprocessorManager::init();

    // Forcibly disable it if configuration says so
    if ( settings.value( QLatin1String( "General/DisablePreprocessing" ), false ).toBool() )
      PreprocessorManager::instance()->setEnabled( false );

    if ( settings.value( QLatin1String( "Cache/EnableCleaner" ), true ).toBool() ) {
      mCacheCleaner = new CacheCleaner( this );
      mCacheCleaner->start( QThread::IdlePriority );
    }

    mIntervalChecker = new IntervalCheck( this );
    mIntervalChecker->start( QThread::IdlePriority );

    mItemRetrievalThread = new ItemRetrievalThread( this );
    mItemRetrievalThread->start( QThread::HighPriority );

    const QString searchManager = settings.value( QLatin1String( "Search/Manager" ), QLatin1String( "Nepomuk" ) ).toString();
    if ( searchManager == QLatin1String( "Nepomuk" ) )
      mSearchManager = new NepomukManager( this );
    else
      mSearchManager = new DummySearchManager;

    new ServerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Server" ), this );

    const QByteArray dbusAddress = qgetenv( "DBUS_SESSION_BUS_ADDRESS" );
    if ( !dbusAddress.isEmpty() ) {
      connectionSettings.setValue( QLatin1String( "DBUS/Address" ), QLatin1String( dbusAddress ) );
    }

    connect( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
             SLOT(serviceOwnerChanged(QString,QString,QString)) );

    // Unhide all the items that are actually hidden.
    // The hidden flag was probably left out after an (abrupt)
    // server quit. We don't attempt to resume preprocessing
    // for the items as we don't actually know at which stage the
    // operation was interrupted...
    db->unhideAllPimItems();
}

AkonadiServer::~AkonadiServer()
{
}

template <typename T> static void quitThread( T & thread )
{
  if ( !thread )
    return;
  thread->quit();
  thread->wait();
  delete thread;
  thread = 0;
}

void AkonadiServer::quit()
{
    if ( mAlreadyShutdown )
      return;
    mAlreadyShutdown = true;

    qDebug() << "terminating service threads";
    quitThread( mCacheCleaner );
    quitThread( mIntervalChecker );
    quitThread( mItemRetrievalThread );

    delete mSearchManager;
    mSearchManager = 0;

    qDebug() << "terminating connection threads";
    for ( int i = 0; i < mConnections.count(); ++i )
      quitThread( mConnections[ i ] );
    mConnections.clear();

    // Terminate the preprocessor manager before the database but after all connections are gone
    PreprocessorManager::done();

    DataStore::self()->close();
    Q_ASSERT( QSqlDatabase::connectionNames().isEmpty() );

    qDebug() << "stopping db process";
    stopDatabaseProcess();

    QSettings settings( XdgBaseDirs::akonadiServerConfigFile(), QSettings::IniFormat );
    const QString connectionSettingsFile = XdgBaseDirs::akonadiConnectionConfigFile( XdgBaseDirs::WriteOnly );

#ifndef Q_OS_WIN
    QSettings connectionSettings( connectionSettingsFile, QSettings::IniFormat );
    const QString defaultSocketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString socketDir = settings.value( QLatin1String( "Connection/SocketDirectory" ), defaultSocketDir ).toString();

    if ( !QDir::home().remove( socketDir + QLatin1String( "/akonadiserver.socket" ) ) )
        akError() << "Failed to remove Unix socket";
#endif
    if ( !QDir::home().remove( connectionSettingsFile ) )
        akError() << "Failed to remove runtime connection config file";

    QTimer::singleShot( 0, this, SLOT( doQuit() ) );
}

void AkonadiServer::doQuit()
{
    QCoreApplication::exit();
}

void AkonadiServer::incomingConnection( quintptr socketDescriptor )
{
    if ( mAlreadyShutdown )
      return;
    QPointer<AkonadiConnection> thread = new AkonadiConnection( socketDescriptor, this );
    connect( thread, SIGNAL( finished() ), thread, SLOT( deleteLater() ) );
    mConnections.append( thread );
    thread->start();
}


AkonadiServer * AkonadiServer::instance()
{
    if ( !s_instance )
        s_instance = new AkonadiServer();
    return s_instance;
}

void AkonadiServer::startDatabaseProcess()
{
  if ( !DbConfig::useInternalServer() )
    return;

  const QString serverPath = DbConfig::serverPath();
  if ( serverPath.isEmpty() )
    akFatal() << "No path to external sql server set in server configuration!";

  // create the database directories if they don't exists
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString akDir   = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/" ) );
  const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
  const QString fileDataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) );

  if ( DbConfig::driverName() == QLatin1String( "QMYSQL" ) )
    startMysqlDatabaseProcess();
  else if ( DbConfig::driverName() == QLatin1String( "QPSQL" ) )
    startPostgresqlDatabaseProcess();
}

void AkonadiServer::startPostgresqlDatabaseProcess()
{
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString socketDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );

  if ( !QFile::exists( QString::fromLatin1( "%1/PG_VERSION" ).arg( dataDir ) ) ) {
    // postgre data directory not initialized yet, so call initdb on it

    const QStringList postgresSearchPath = QStringList()
        << QLatin1String("/usr/sbin")
        << QLatin1String("/usr/local/sbin")
        << QLatin1String("/usr/lib/postgresql/8.4/bin");

    const QString initDbPath = XdgBaseDirs::findExecutableFile( QLatin1String( "initdb" ), postgresSearchPath );

    // call 'initdb -D/home/user/.local/share/akonadi/data_db'
    const QString command = QString::fromLatin1( "%1 -D%2" ).arg( initDbPath ).arg( dataDir );
    QProcess::execute( command );

    const QString configFileName = dataDir + QDir::separator() + QLatin1String( "postgresql.conf" );
    QFile configFile( configFileName );
    configFile.open( QIODevice::ReadOnly );

    QString content = QString::fromUtf8( configFile.readAll() );
    configFile.close();

    // avoid binding to tcp port
    content.replace( QLatin1String( "#listen_addresses = 'localhost'" ),
                     QLatin1String( "listen_addresses = ''" ) );

    // set the directory for unix domain socket communication
    content.replace( QLatin1String( "#unix_socket_directory = ''" ),
                     QString::fromLatin1( "unix_socket_directory = '%1'" ).arg( socketDir ) );

    configFile.open( QIODevice::WriteOnly );
    configFile.write( content.toUtf8() );
    configFile.close();
  }

  // synthesize the postgres command
  QStringList arguments;
  arguments << QString::fromLatin1( "-D%1" ).arg( dataDir );

  mDatabaseProcess = new QProcess( this );
  mDatabaseProcess->start( DbConfig::serverPath(), arguments );
  if ( !mDatabaseProcess->waitForStarted() ) {
    akError() << "Could not start database server!";
    akError() << "executable:" << DbConfig::serverPath();
    akError() << "arguments:" << arguments;
    akFatal() << "process error:" << mDatabaseProcess->errorString();
  }

  const QLatin1String initCon( "initConnection" );
  {
    QSqlDatabase db = QSqlDatabase::addDatabase( DbConfig::driverName(), initCon );
    DbConfig::configure( db );

    // use the dummy database that is always available
    db.setDatabaseName( QLatin1String( "template1" ) );

    if ( !db.isValid() )
      akFatal() << "Invalid database object during database server startup";

    bool opened = false;
    for ( int i = 0; i < 120; ++i ) {
      opened = db.open();
      if ( opened )
        break;

      if ( mDatabaseProcess->waitForFinished( 500 ) ) {
        akError() << "Database process exited unexpectedly during initial connection!";
        akError() << "executable:" << DbConfig::serverPath();
        akError() << "arguments:" << arguments;
        akError() << "stdout:" << mDatabaseProcess->readAllStandardOutput();
        akError() << "stderr:" << mDatabaseProcess->readAllStandardError();
        akError() << "exit code:" << mDatabaseProcess->exitCode();
        akFatal() << "process error:" << mDatabaseProcess->errorString();
      }
    }

    if ( opened ) {
      {
        QSqlQuery query( db );

        // check if the 'akonadi' database already exists
        query.exec( QString::fromLatin1( "SELECT * FROM pg_catalog.pg_database WHERE datname = '%1'" ).arg( DbConfig::databaseName() ) );

        // if not, create it
        if ( !query.first() ) {
          if ( !query.exec( QString::fromLatin1( "CREATE DATABASE %1" ).arg( DbConfig::databaseName() ) ) ) {
            akError() << "Failed to create database";
            akError() << "Query error:" << query.lastError().text();
            akFatal() << "Database error:" << db.lastError().text();
          }
        }
      } // make sure query is destroyed before we close the db
      db.close();
    }
  }

  QSqlDatabase::removeDatabase( initCon );
}

void AkonadiServer::startMysqlDatabaseProcess()
{
  const QString mysqldPath = DbConfig::serverPath();

  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_data" ) );
  const QString akDir   = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/" ) );
  const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
  const QString fileDataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) );

  // generate config file
  const QString globalConfig = XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/mysql-global.conf" ) );
  const QString localConfig  = XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/mysql-local.conf" ) );
  const QString actualConfig = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) ) + QLatin1String("/mysql.conf");
  if ( globalConfig.isEmpty() )
    akFatal() << "Did not find MySQL server default configuration (mysql-global.conf)";

  bool confUpdate = false;
  QFile actualFile ( actualConfig );
  // update conf only if either global (or local) is newer than actual
  if ( (QFileInfo( globalConfig ).lastModified() > QFileInfo( actualFile ).lastModified()) ||
       (QFileInfo( localConfig ).lastModified()  > QFileInfo( actualFile ).lastModified()) )
  {
    QFile globalFile( globalConfig );
    QFile localFile ( localConfig );
    if ( globalFile.open( QFile::ReadOnly ) && actualFile.open( QFile::WriteOnly ) ) {
      actualFile.write( globalFile.readAll() );
      if ( !localConfig.isEmpty() ) {
        if ( localFile.open( QFile::ReadOnly ) ) {
          actualFile.write( localFile.readAll() );
          localFile.close();
        }
      }
      globalFile.close();
      actualFile.close();
      confUpdate = true;
    } else {
      akError() << "Unable to create MySQL server configuration file.";
      akError() << "This means that either the default configuration file (mysql-global.conf) was not readable";
      akFatal() << "or the target file (mysql.conf) could not be written.";
    }
  }

  // MySQL doesn't like world writeable config files (which makes sense), but
  // our config file somehow ends up being world-writable on some systems for no
  // apparent reason nevertheless, so fix that
  const QFile::Permissions allowedPerms = actualFile.permissions()
      & ( QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther );
  if ( allowedPerms != actualFile.permissions() )
    actualFile.setPermissions( allowedPerms );

  if ( dataDir.isEmpty() )
    akFatal() << "Akonadi server was not able not create database data directory";

  if ( akDir.isEmpty() )
    akFatal() << "Akonadi server was not able not create database log directory";

  if ( miscDir.isEmpty() )
    akFatal() << "Akonadi server was not able not create database misc directory";

  // the socket path must not exceed 103 characters, so check for max dir length right away
  if ( miscDir.length() >= 90 )
      akFatal() << "MySQL cannot deal with a socket path this long. Path was: " << miscDir;

  // move mysql error log file out of the way
  const QFileInfo errorLog( dataDir + QDir::separator() + QString::fromLatin1( "mysql.err" ) );
  if ( errorLog.exists() ) {
    QFile logFile( errorLog.absoluteFilePath() );
    QFile oldLogFile( dataDir + QDir::separator() + QString::fromLatin1( "mysql.err.old" ) );
    if ( logFile.open( QFile::ReadOnly ) && oldLogFile.open( QFile::Append ) ) {
      oldLogFile.write( logFile.readAll() );
      oldLogFile.close();
      logFile.close();
      logFile.remove();
    } else {
      akError() << "Failed to open MySQL error log.";
    }
  }

  // clear mysql ib_logfile's in case innodb_log_file_size option changed in last confUpdate
  if ( confUpdate ) {
      QFile(dataDir + QDir::separator() + QString::fromLatin1( "ib_logfile0" )).remove();
      QFile(dataDir + QDir::separator() + QString::fromLatin1( "ib_logfile1" )).remove();
  }

  // synthesize the mysqld command
  QStringList arguments;
  arguments << QString::fromLatin1( "--defaults-file=%1/mysql.conf" ).arg( akDir );
  arguments << QString::fromLatin1( "--datadir=%1/" ).arg( dataDir );
  arguments << QString::fromLatin1( "--socket=%1/mysql.socket" ).arg( miscDir );

  mDatabaseProcess = new QProcess( this );
  mDatabaseProcess->start( mysqldPath, arguments );
  if ( !mDatabaseProcess->waitForStarted() ) {
    akError() << "Could not start database server!";
    akError() << "executable:" << mysqldPath;
    akError() << "arguments:" << arguments;
    akFatal() << "process error:" << mDatabaseProcess->errorString();
  }

  const QLatin1String initCon( "initConnection" );
  {
    QSqlDatabase db = QSqlDatabase::addDatabase( DbConfig::driverName(), initCon );
    DbConfig::configure( db );
    db.setDatabaseName( QString() ); // might not exist yet, then connecting to the actual db will fail
    if ( !db.isValid() )
      akFatal() << "Invalid database object during database server startup";

    bool opened = false;
    for ( int i = 0; i < 120; ++i ) {
      opened = db.open();
      if ( opened )
        break;
      if ( mDatabaseProcess->waitForFinished( 500 ) ) {
        akError() << "Database process exited unexpectedly during initial connection!";
        akError() << "executable:" << mysqldPath;
        akError() << "arguments:" << arguments;
        akError() << "stdout:" << mDatabaseProcess->readAllStandardOutput();
        akError() << "stderr:" << mDatabaseProcess->readAllStandardError();
        akError() << "exit code:" << mDatabaseProcess->exitCode();
        akFatal() << "process error:" << mDatabaseProcess->errorString();
      }
    }

    if ( opened ) {
      {
        QSqlQuery query( db );
        if ( !query.exec( QString::fromLatin1( "USE %1" ).arg( DbConfig::databaseName() ) ) ) {
          akDebug() << "Failed to use database" << DbConfig::databaseName();
          akDebug() << "Query error:" << query.lastError().text();
          akDebug() << "Database error:" << db.lastError().text();
          akDebug() << "Trying to create database now...";
          if ( !query.exec( QLatin1String( "CREATE DATABASE akonadi" ) ) ) {
            akError() << "Failed to create database";
            akError() << "Query error:" << query.lastError().text();
            akFatal() << "Database error:" << db.lastError().text();
          }
        }
      } // make sure query is destroyed before we close the db
      db.close();
    }
  }

  QSqlDatabase::removeDatabase( initCon );
}

void AkonadiServer::createDatabase()
{
  const QLatin1String initCon( "initConnection" );
  QSqlDatabase db = QSqlDatabase::addDatabase( DbConfig::driverName(), initCon );
  DbConfig::configure( db );
  db.setDatabaseName( DbConfig::databaseName() );
  if ( !db.isValid() )
    akFatal() << "Invalid database object during initial database connection";

  if ( db.open() )
    db.close();
  else {
    akDebug() << "Failed to use database" << DbConfig::databaseName();
    akDebug() << "Database error:" << db.lastError().text();
    akDebug() << "Trying to create database now...";

    db.close();
    db.setDatabaseName( QString() );
    if ( db.open() ) {
      {
        QSqlQuery query( db );
        if ( !query.exec( QLatin1String( "CREATE DATABASE akonadi" ) ) ) {
          akError() << "Failed to create database";
          akError() << "Query error:" << query.lastError().text();
          akFatal() << "Database error:" << db.lastError().text();
        }
      } // make sure query is destroyed before we close the db
      db.close();
    }
  }
  QSqlDatabase::removeDatabase( initCon );
}

void AkonadiServer::stopDatabaseProcess()
{
  if ( !mDatabaseProcess )
    return;

  // first, try the nicest approach
  const QString shutdownCmd = DbConfig::cleanServerShutdownCommand();
  if ( !shutdownCmd.isEmpty() ) {
      QProcess::execute( shutdownCmd );
      if ( mDatabaseProcess->waitForFinished(3000) )
          return;
  }
  mDatabaseProcess->terminate();
  const bool result = mDatabaseProcess->waitForFinished(3000);
  // We've waited nicely for 3 seconds, to no avail, let's be rude.
  if ( !result )
    mDatabaseProcess->kill();
}

void AkonadiServer::serviceOwnerChanged(const QString & name, const QString & oldOwner, const QString & newOwner)
{
  Q_UNUSED( oldOwner );
  if ( name == QLatin1String( AKONADI_DBUS_CONTROL_SERVICE ) && newOwner.isEmpty() ) {
    akError() << "Control process died, committing suicide!";
    quit();
  }
}

#include "akonadi.moc"
