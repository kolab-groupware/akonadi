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

#ifndef AKONADIUID_H
#define AKONADIUID_H

#include <QtCore/QPointer>

#include <handler.h>
#include <scope.h>

namespace Akonadi {

/**
  @ingroup akonadi_server_handler

  Proxy handler for uid/rid commands.
 */
class Uid : public Handler
{
  Q_OBJECT
  public:
    Uid( Scope::SelectionScope scope );
    ~Uid();

    bool parseStream();

  private:
    QPointer<Handler> mSubHandler;
    Scope::SelectionScope mScope;
};

}

#endif
