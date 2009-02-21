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

#ifndef AKONADI_EXCEPTION_H

#include <QByteArray>
#include <exception>

namespace Akonadi {

/**
  Base class for execpetion used internally by the Akonadi server.
*/
class Exception : public std::exception
{
  public:
    Exception( const char *what ) throw() : mWhat( what ) {}
    Exception( const Exception &other ) throw() : mWhat( other.what() ) {}
    virtual ~Exception() throw() {}
    const char* what() const throw() { return mWhat.constData(); }
    virtual const char* type() const throw() { return "General Exception"; }
  private:
    QByteArray mWhat;
};

#define AKONADI_EXCEPTION_MAKE_INSTANCE( classname ) \
class classname : public Akonadi::Exception \
{ \
  public: \
    classname ( const char *what ) throw() : Akonadi::Exception( what ) {} \
    const char* type() const throw() { return "" #classname; } \
}

}

#endif
