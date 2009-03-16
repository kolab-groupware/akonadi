/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

#include "handler/list.h"
#include "mockobjects.h"

#include "handlertest.h"
#include "imapstreamparser.h"

using namespace Akonadi;

void HandlerTest::initTestCase()
{
    qRegisterMetaType<Response>("Response");
}

void HandlerTest::testInit()
{
}

/// ---- List ----
void HandlerTest::testSeparatorList()
{
    Handler* l = getHandlerFor("LIST");
    QVERIFY( dynamic_cast<List*>(l) != 0 );

    QBuffer buffer;
    buffer.open( QBuffer::ReadWrite);
    ImapStreamParser parser(&buffer);

    const QByteArray line = "LIST \"\" \"\"";
    buffer.write( line );
    buffer.seek(0);

    l->setStreamParser(&parser);

    QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
    l->parseStream();
    QCOMPARE(spy.count(), 2);

    const QString expectedFirstResponse = "* LIST (\\Noselect) \"/\" \"\"";
    QVERIFY(nextResponse(spy).asString() == expectedFirstResponse);

    const QString expectedSecondResponse = "1 OK List completed";
    QVERIFY(nextResponse(spy).asString() == expectedSecondResponse);
}

void HandlerTest::testRootPercentList()
{
    QSKIP( "Does not work without MockBackend", SkipAll );
    Handler* l = getHandlerFor("LIST");
    QVERIFY( dynamic_cast<List*>(l) != 0 );

    QBuffer buffer;
    buffer.open( QBuffer::ReadWrite);
    ImapStreamParser parser(&buffer);

    const QByteArray line = "LIST \"\" \"%\"";
    buffer.write( line );
    buffer.seek(0);

    l->setStreamParser(&parser);

    QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
    l->parseStream();
    QCOMPARE(spy.count(), 2);

    const QByteArray expectedFirstResponse = "* LIST () \"/\" \"INBOX\"";
    QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

    const QByteArray expectedSecondResponse = "1 OK List completed";
    QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );
}

void HandlerTest::testRootStarList()
{
    QSKIP( "Does not work without MockBackend", SkipAll );
    Handler* l = getHandlerFor("LIST");
    QVERIFY( dynamic_cast<List*>(l) != 0 );

    const QByteArray line = "LIST \"\" \"*\"";
    QBuffer buffer;
    buffer.open( QBuffer::ReadWrite);
    ImapStreamParser parser(&buffer);

    buffer.write( line );
    buffer.seek(0);

    l->setStreamParser(&parser);

    QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
    l->parseStream();
    QCOMPARE(spy.count(), 3);

    const QByteArray expectedFirstResponse = "* LIST () \"/\" \"INBOX\"";
    QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

    const QByteArray expectedSecondResponse = "* LIST () \"/\" \"INBOX/foo\"";
    QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );

    const QByteArray expectedThirdResponse = "1 OK List completed";
    QCOMPARE(nextResponse(spy).asString(), expectedThirdResponse );
}

void HandlerTest::testInboxList()
{
    QSKIP( "Does not work without MockBackend", SkipAll );
    Handler* l = getHandlerFor("LIST");
    QVERIFY( dynamic_cast<List*>(l) != 0 );

    const QByteArray line = "LIST \"\" \"INBOX\"";
    QBuffer buffer;
    buffer.open( QBuffer::ReadWrite);
    ImapStreamParser parser(&buffer);

    buffer.write( line );
    buffer.seek(0);

    l->setStreamParser(&parser);

    QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
    l->parseStream();
    QCOMPARE(spy.count(), 3);

    const QByteArray expectedFirstResponse = "* LIST () \"/\" \"foo\"";
    QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

    const QByteArray expectedSecondResponse = "* LIST () \"/\" \"bar\"";
    QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );

    const QByteArray expectedThirdResponse = "1 OK List completed";
    QCOMPARE(nextResponse(spy).asString(), expectedThirdResponse );
}


/// ---- Fetch ----
void HandlerTest::testFetch()
{
}

// Helper
Response HandlerTest::nextResponse( QSignalSpy& spy )
{
    QList<QVariant> arguments = spy.takeFirst();
    Response r = qvariant_cast<Response>(arguments.at(0));
    //qDebug() << "Response: " << r.asString();
    return r;
}

Handler* HandlerTest::getHandlerFor(const QByteArray& command )
{
    Handler *h = Handler::findHandlerForCommandAuthenticated( command );
    if( h != 0 ) {
        h->setTag("1");
        h->setConnection( MockObjects::mockConnection() );
    }
    return h;
}

Q_DECLARE_METATYPE( Response )
QTEST_MAIN( HandlerTest )

#include "handlertest.moc"
