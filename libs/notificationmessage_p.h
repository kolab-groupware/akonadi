/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_NOTIFICATIONMESSAGE_P_H
#define AKONADI_NOTIFICATIONMESSAGE_P_H

#include "akonadiprotocolinternals_export.h"

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtDBus/QDBusArgument>

namespace Akonadi {

/**
  @internal
  Used for sending notification signals over DBus.
  DBus type: (ayiiisayisas)
*/

class AKONADIPROTOCOLINTERNALS_EXPORT NotificationMessage
{
  public:
    typedef QList<NotificationMessage> List;
    typedef qint64 Id;

    enum Type {
      InvalidType,
      Collection,
      Item
    };

    enum Operation {
      InvalidOp,
      Add,
      Modify,
      Move,
      Remove,
      Link,
      Unlink,
      Subscribe,
      Unsubscribe
    };

    NotificationMessage();
    NotificationMessage( const NotificationMessage &other );
    ~NotificationMessage();

    NotificationMessage &operator=( const NotificationMessage &other );
    bool operator==( const NotificationMessage &other ) const;

    static void registerDBusTypes();

    QByteArray sessionId() const;
    void setSessionId( const QByteArray &sessionId );

    Type type() const;
    void setType( Type type );

    Operation operation() const;
    void setOperation( Operation operation );

    Id uid() const;
    void setUid( Id uid );

    QString remoteId() const;
    void setRemoteId( const QString &remoteId );

    QByteArray resource() const;
    void setResource( const QByteArray &resource );

    Id parentCollection() const;
    void setParentCollection( Id parent );

    Id parentDestCollection() const;
    void setParentDestCollection( Id parent );

    QByteArray destinationResource() const;
    void setDestinationResource( const QByteArray &destResource );

    QString mimeType() const;
    void setMimeType( const QString &mimeType );

    QSet<QByteArray> itemParts() const;
    void setItemParts( const QSet<QByteArray> &parts );

    QString toString() const;

    /**
      Adds a new notification message to the given list and compresses notifications
      where possible.
    */
    static void appendAndCompress( NotificationMessage::List &list, const NotificationMessage &msg );
    // BIC: make the above return bool.
    static void appendAndCompress( NotificationMessage::List &list, const NotificationMessage &msg, bool *appended );

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

QDBusArgument &operator<<( QDBusArgument &arg, const Akonadi::NotificationMessage &msg );
const QDBusArgument &operator>>( const QDBusArgument &arg, Akonadi::NotificationMessage &msg );

uint qHash( const Akonadi::NotificationMessage &msg );

Q_DECLARE_TYPEINFO( Akonadi::NotificationMessage, Q_MOVABLE_TYPE );

Q_DECLARE_METATYPE( Akonadi::NotificationMessage )
Q_DECLARE_METATYPE( Akonadi::NotificationMessage::List )

// V2 is used in NotificationSource.xml interface, so it must be
// defined so that old clients that only include this header
// will compile
#include "notificationmessagev2_p.h"

#endif
