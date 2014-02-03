/*
    Copyright (c) 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_SEARCHREQUEST_H
#define AKONADI_SEARCHREQUEST_H

#include <QObject>
#include <QVector>
#include <QSet>
#include <QStringList>

namespace Akonadi
{

class AkonadiConnection;

class SearchRequest: public QObject
{
    Q_OBJECT

  public:
    SearchRequest( const QByteArray &connectionId );
    ~SearchRequest();

    void setQuery( const QString &query );
    QString query() const;
    void setCollections( const QVector<qint64> &collections );
    QVector<qint64> collections() const;
    void setMimeTypes( const QStringList &mimeTypes );
    QStringList mimeTypes() const;
    void setRemoteSearch( bool remote );
    bool remoteSearch() const;

    /**
     * Whether resultsAvailable should be emitted whenever new results are available
     * (default behavior) or whether results should be cached and will be retrieved
     * at once via results()
     */
    void setEmitResults( bool emitResults );

    QByteArray connectionId() const;

    void exec();

    QSet<qint64> results() const;

  Q_SIGNALS:
    void resultsAvailable( const QSet<qint64> &results );

  private:
    void searchPlugins();
    void emitResults( const QSet<qint64> &results );

    QByteArray mConnectionId;
    QString mQuery;
    QVector<qint64> mCollections;
    QStringList mMimeTypes;
    bool mRemoteSearch;
    bool mEmitResults;
    QSet<qint64> mResults;

};

}

#endif // AKONADI_SEARCHREQUEST_H
