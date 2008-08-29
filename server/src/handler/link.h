/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_LINK_H
#define AKONADI_LINK_H

#include "handler.h"

namespace Akonadi {

/**
 * @ingroup akonadi_server_handler
 *
 * Handler for the LINK and UNLINK commands.
 *
 * These commands are used to add and remove references of a set of items to a
 * virtual collection.
 *
 * <h4>Syntax</h4>
 *
 * Request:
 * @verbatim
 * link-request = tag " LINK " collection-id " " item-set
 * unlink-request = tag " UNLINK " collection-id " " item-set
 * @endverbatim
 *
 * There is only the usual status response indicating success or failure of the
 * LINK and UNLINK commands.
 */
class Link : public Handler
{
  public:
    /**
     * @param create @c true adds references, @c false removes them
     */
    Link( bool create );
    bool handleLine( const QByteArray &line );

  private:
    bool mCreateLinks;
};

}

#endif