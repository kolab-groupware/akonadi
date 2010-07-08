/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef DBCONFIGVIRTUOSO_H
#define DBCONFIGVIRTUOSO_H

#include "dbconfig.h"

class QProcess;

class DbConfigVirtuoso : public DbConfig
{
  public:
    DbConfigVirtuoso();

    /**
     * Returns the name of the used driver.
     */
    virtual QString driverName() const;

    /**
     * Returns the database name.
     */
    virtual QString databaseName() const;

    /**
     * This method is called whenever the Akonadi server is started
     * and before the initial database connection is set up.
     *
     * At this point the default settings should be determined, merged
     * with the given @p settings and written back.
     */
    virtual bool init( QSettings &settings );

    /**
     * This method applies the configured settings to the QtSql @p database
     * instance.
     */
    virtual void apply( QSqlDatabase &database );

    /**
     * Returns whether an internal server needs to be used.
     */
    virtual bool useInternalServer() const;

  private:
    QString mDatabaseName;
    QString mHostName;
    QString mUserName;
    QString mPassword;
    QString mConnectionOptions;
};

#endif
