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

#ifndef AKONADISTORE_H
#define AKONADISTORE_H

#include <handler.h>

#include "storage/entity.h"
#include "libs/imapset_p.h"

namespace Akonadi {

class PimItem;

/**
  @ingroup akonadi_server_handler

  Handler for the store command.
 */
class Store : public Handler
{
  Q_OBJECT

  public:
    Store( bool isUid = false );
    ~Store();


    enum Operation
    {
      Replace,
      Add,
      Delete
    };

    bool parseStream();

  private:
    void parseCommand( const QByteArray &line );
    void parseCommandStream();

    bool replaceFlags( const PimItem &item, const QList<QByteArray> &flags );
    bool addFlags( const PimItem &item, const QList<QByteArray> &flags );
    bool deleteFlags( const PimItem &item, const QList<QByteArray> &flags );

  private:
    ImapSet mItemSet;
    int mPos;
    qint64 mPreviousRevision;
    qint64 mSize;
    bool mUidStore;
};

}

#endif
