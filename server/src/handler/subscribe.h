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

#ifndef AKONADI_SUBSCRIBE_H
#define AKONADI_SUBSCRIBE_H

#include <handler.h>

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the subscribe/unsubscribe commands.
  Compatible with RFC 3501 section 6.3.7 and 6.3.8.

  Locally subscribe/unsubscribe a set of collections.

  <h4>Syntax</h4>
  @verbatim
  tag "SUBSCRIBE " collection *( " " collection )
  tag "UNSUBSCRIBE " collection *( " " collection )
  @endverbatim
 */
class Subscribe : public Handler
{
  Q_OBJECT
  public:
    Subscribe( bool subscribe );
    bool parseStream();

  private:
    bool mSubscribe;
};

} // namespace Server
} // namespace Akonadi

#endif
