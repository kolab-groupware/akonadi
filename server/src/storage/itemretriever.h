/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef ITEMRETRIEVER_H
#define ITEMRETRIEVER_H

#include "exception.h"
#include "entities.h"

#include "libs/imapset_p.h"

#include <QStringList>

AKONADI_EXCEPTION_MAKE_INSTANCE( ItemRetrieverException );

namespace Akonadi {
  class AkonadiConnection;
  class QueryBuilder;

/**
  Helper class for retrieving missing items parts from remote resources.

  @todo make usable for Fetch by allowing to share queries
*/
class ItemRetriever
{
  public:
    ItemRetriever( AkonadiConnection *connection );

    void setRetrievePart( const QStringList &parts );
    void setRetrieveFullPayload( bool fullPayload );
    void setItemSet( const ImapSet &set, const Collection &collection = Collection() );
    void setItemSet( const ImapSet &set, bool isUid );
    void setItem( const Akonadi::Entity::Id &id );

    void exec();

  private:
    QueryBuilder buildItemQuery() const;
    QueryBuilder buildPartQuery() const;

  private:
    ImapSet mItemSet;
    Collection mCollection;
    AkonadiConnection* mConnection;
    QStringList mParts;
    bool mFullPayload;
};

}

#endif
