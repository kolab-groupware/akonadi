/***************************************************************************
 *   Copyright (C) 2014 by Christian Mollekopf <mollekopf@kolabsys.com>    *
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

#ifndef AKONADIFETCHRELATION_H
#define AKONADIFETCHRELATION_H

#include "handler.h"
#include "scope.h"

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the RELATIONFETCH command.
 */
class RelationFetch : public Handler
{
    Q_OBJECT
public:
    RelationFetch(Scope::SelectionScope scope);
    ~RelationFetch();

    bool parseStream();

    static QByteArray relationToByteArray(qint64 leftId, qint64 rightId, const QByteArray &type, const QByteArray &rid);

private:
    Scope mScope;
};

} // namespace Server
} // namespace Akonadi

#endif
