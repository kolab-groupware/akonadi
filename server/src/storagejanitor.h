/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#ifndef STORAGEJANITOR_H
#define STORAGEJANITOR_H

#include <QThread>
#include <qdbusmacros.h>
#include <QtDBus/QDBusConnection>

namespace Akonadi {
class Collection;
}

class StorageJanitorThread : public QThread
{
  Q_OBJECT
  public:
    explicit StorageJanitorThread( QObject * parent = 0 );
  protected:
    virtual void run();
};

/**
 * Various database checking/maintenance features.
 */
class StorageJanitor : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.Janitor" )
  public:
    explicit StorageJanitor(QObject* parent = 0);
    ~StorageJanitor();

  public slots:
    /** Triggers a consistency check of the internal storage. */
    Q_SCRIPTABLE Q_NOREPLY void check();
    /** Triggers a vacuuming of the database, that is compacting of unused space. */
    Q_SCRIPTABLE Q_NOREPLY void vacuum();

  signals:
    /** Sends informational messages to a possible UI for this. */
    Q_SCRIPTABLE void information( const QString &msg );

  private:
    void inform( const char *msg );
    void inform( const QString &msg );

    /**
     * Look for collections belonging to non-existent resources.
     */
    void findOrphanedCollections();

    /**
     * Verifies there is a path from @p col to the root of the collection tree
     * and that that everything along that path belongs to the same resource.
     */
    void checkPathToRoot( const Akonadi::Collection &col );

    /**
     * Look for items belonging to non-existing collections.
     */
    void findOrphanedItems();

    /**
     * Look for parts belonging to non-existing items.
     */
    void findOrphanedParts();

    /**
     * Look for parts refering to the same external file.
     */
    void findOverlappingParts();

    /**
     * Verify fs and db part state.
     */
    void verifyExternalParts();

    /**
     * Look for dirty objects.
     */
    void findDirtyObjects();

  private:
    QDBusConnection m_connection;
};

#endif // STORAGEJANITOR_H