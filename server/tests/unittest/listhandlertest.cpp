/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <QObject>

#include <handler/list.h>
#include <imapstreamparser.h>
#include <response.h>

#include "fakeakonadiserver.h"
#include "aktest.h"
#include "akdebug.h"
#include "entities.h"
#include "dbinitializer.h"
#include <storage/storagedebugger.h>

#include <QtTest/QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

class ListHandlerTest : public QObject
{
    Q_OBJECT

public:
    ListHandlerTest()
        : QObject()
    {
        qRegisterMetaType<Akonadi::Server::Response>();

        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            akError() << "Server exception: " << e.what();
            akFatal() << "Fake Akonadi Server failed to start up, aborting test";
        }
        {
            MimeType mt(QLatin1String("mimetype1"));
            mt.insert();
        }
        {
            MimeType mt(QLatin1String("mimetype2"));
            mt.insert();
        }
        {
            MimeType mt(QLatin1String("mimetype3"));
            mt.insert();
        }
        {
            MimeType mt(QLatin1String("mimetype4"));
            mt.insert();
        }
    }

    ~ListHandlerTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

    QScopedPointer<DbInitializer> initializer;
private Q_SLOTS:

    void testList_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);
        Collection col4 = initializer->createCollection("col4");

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST 0 INF () ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col2)
                     << initializer->listResponse(col3)
                     << initializer->listResponse(col4)
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " 0 () ()"
                     << initializer->listResponse(col1)
                    << "S: 2 OK List completed";
            QTest::newRow("base list") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " 1 () ()"
                     << initializer->listResponse(col2)
                    << "S: 2 OK List completed";
            QTest::newRow("first level list") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " INF () ()"
                     << initializer->listResponse(col2)
                     << initializer->listResponse(col3)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list that filters collection") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col2.id()) + " 0 () (ANCESTORS INF)"
                     << initializer->listResponse(col2, true)
                     << "S: 2 OK List completed";
            QTest::newRow("base ancestors") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " 1 () (ANCESTORS INF)"
                     << initializer->listResponse(col2, true)
                     << "S: 2 OK List completed";
            QTest::newRow("first level ancestors") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " INF () (ANCESTORS INF)"
                     << initializer->listResponse(col2, true)
                     << initializer->listResponse(col3, true)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive ancestors") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 1 () ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col4)
                     << "S: 2 OK List completed";
            QTest::newRow("first level root list") << scenario;
        }
    }

    void testList()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testListFiltered_data()
    {
        initializer.reset(new DbInitializer);

        MimeType mtCalendar(QLatin1String("text/calendar"));
        mtCalendar.insert();

        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        col1.update();
        Collection col2 = initializer->createCollection("col2", col1);
        col2.addMimeType(mtCalendar);
        col2.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST 0 INF (MIMETYPE (text/calendar)) ()"
                     << initializer->listResponse(col1, false, false)
                     << initializer->listResponse(col2)
                     << "S: 2 OK List completed";
            QTest::newRow("recursive list to display including local override") << scenario;
        }
    }

    void testListFiltered()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testListFilterByResource()
    {
        initializer.reset(new DbInitializer);

        Resource res2;
        res2.setName(QLatin1String("testresource2"));
        QVERIFY(res2.insert());

        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        Collection col2;
        col2.setName(QLatin1String("col2"));
        col2.setRemoteId(QLatin1String("col2"));
        col2.setResource(res2);
        QVERIFY(col2.insert());

        QList<QByteArray> scenario;
        scenario << FakeAkonadiServer::defaultScenario()
                 << "C: 2 LIST 0 INF (RESOURCE \"testresource\") ()"
                 << initializer->listResponse(col1)
                 << "S: 2 OK List completed";

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();

        col2.remove();
        res2.remove();
    }

    void testListEnabled_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2", col1);
        col2.setEnabled(false);
        col2.setSyncPref(Akonadi::Server::Tristate::True);
        col2.setDisplayPref(Akonadi::Server::Tristate::True);
        col2.setIndexPref(Akonadi::Server::Tristate::True);
        col2.update();
        Collection col3 = initializer->createCollection("col3", col2);
        col3.setEnabled(true);
        col3.setSyncPref(Akonadi::Server::Tristate::False);
        col3.setDisplayPref(Akonadi::Server::Tristate::False);
        col3.setIndexPref(Akonadi::Server::Tristate::False);
        col3.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(col3.id()) + " 0 (DISPLAY  ) ()"
                    << initializer->listResponse(col3)
                    << "S: 2 OK List completed";
            //Listing a disabled collection should still work for base listing
            QTest::newRow("list base of disabled collection") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST 0 INF (DISPLAY  ) ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col2)
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list to display including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST 0 INF (SYNC  ) ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col2)
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list to sync including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST 0 INF (INDEX  ) ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col2)
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list to index including local override") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST 0 INF (ENABLED  ) ()"
                     << initializer->listResponse(initializer->collection("Search"))
                     << initializer->listResponse(col1)
                     << initializer->listResponse(col2)
                     << initializer->listResponse(col3)
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list of enabled") << scenario;
        }
    }

    void testListEnabled()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testListAttribute_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");
        Collection col2 = initializer->createCollection("col2");

        CollectionAttribute attr1;
        attr1.setType("type");
        attr1.setValue("value");
        attr1.setCollection(col1);
        attr1.insert();

        CollectionAttribute attr2;
        attr2.setType("type");
        attr2.setValue(QString::fromUtf8("Umlautäöü").toUtf8());
        attr2.setCollection(col2);
        attr2.insert();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col1.id()) + " 0 () ()"
                     << initializer->listResponse(col1, false, true)
                     << "S: 2 OK List completed";
            QTest::newRow("list attribute") << scenario;
        }

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col2.id()) + " 0 () ()"
                     << initializer->listResponse(col2, false, true)
                     << "S: 2 OK List completed";
            QTest::newRow("list attribute") << scenario;
        }
    }

    void testListAttribute()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testListAncestorAttributes_data()
    {
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");
        Collection col1 = initializer->createCollection("col1");

        CollectionAttribute attr1;
        attr1.setType("type");
        attr1.setValue("value");
        attr1.setCollection(col1);
        attr1.insert();

        Collection col2 = initializer->createCollection("col2", col1);

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                     << "C: 2 LIST " + QByteArray::number(col2.id()) + " 0 () (ANCESTORS (DEPTH INF type NAME REMOTEID))"
                     << initializer->listResponse(col2, true, true, QStringList() << QLatin1String("type") << QLatin1String("NAME") << QLatin1String("REMOTEID"))
                     << "S: 2 OK List completed";
            QTest::newRow("list ancestor attribute with fetch scope") << scenario;
        }
    }

    void testListAncestorAttributes()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

    void testIncludeAncestors_data()
    {
        //The collection we are quering contains a load of disabled collections (typical scenario with many shared folders)
        //The collection we are NOT querying contains a reasonable amount of enabled collections (to test the performance impact of the manually filtering by tree)
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");

        MimeType mtDirectory = MimeType::retrieveByName(QLatin1String("mimetype1"));

        Collection col1 = initializer->createCollection("col1");
        col1.addMimeType(mtDirectory);
        col1.update();
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);
        Collection col4 = initializer->createCollection("col4", col3);
        col4.addMimeType(mtDirectory);
        col4.update();

        QTest::addColumn<QList<QByteArray> >("scenario");

        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(0) + " INF (MIMETYPE (mimetype1)) ()"
                    << initializer->listResponse(col1)
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col3)
                    << initializer->listResponse(col4)
                    << "S: 2 OK List completed";
            QTest::newRow("ensure filtered grandparent is included") << scenario;
        }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(col1.id()) + " INF (MIMETYPE (mimetype1)) ()"
                    << initializer->listResponse(col2)
                    << initializer->listResponse(col3)
                    << initializer->listResponse(col4)
                    << "S: 2 OK List completed";
            //This also ensures col1 is excluded although it matches the mimetype filter
            QTest::newRow("ensure filtered grandparent is included with specified parent") << scenario;
        }
    }

    void testIncludeAncestors()
    {
        QFETCH(QList<QByteArray>, scenario);

        FakeAkonadiServer::instance()->setScenario(scenario);
        FakeAkonadiServer::instance()->runTest();
    }

//No point in running the benchmark everytime
#if 0

    void testListEnabledBenchmark_data()
    {
        //The collection we are quering contains a load of disabled collections (typical scenario with many shared folders)
        //The collection we are NOT querying contains a reasonable amount of enabled collections (to test the performance impact of the manually filtering by tree)
        initializer.reset(new DbInitializer);
        Resource res = initializer->createResource("testresource");

        Collection toplevel = initializer->createCollection("toplevel");

        Collection col1 = initializer->createCollection("col1", toplevel);
        Collection col2 = initializer->createCollection("col2", col1);
        Collection col3 = initializer->createCollection("col3", col2);

        Collection col4 = initializer->createCollection("col4", toplevel);
        Collection col5 = initializer->createCollection("col5", col4);
        col5.setEnabled(false);
        col5.update();
        Collection col6 = initializer->createCollection("col6", col5);
        col5.setEnabled(false);
        col5.update();

        MimeType mt1 = MimeType::retrieveByName(QLatin1String("mimetype1"));
        MimeType mt2 = MimeType::retrieveByName(QLatin1String("mimetype2"));
        MimeType mt3 = MimeType::retrieveByName(QLatin1String("mimetype3"));
        MimeType mt4 = MimeType::retrieveByName(QLatin1String("mimetype4"));

        QTime t;
        t.start();
        for (int i = 0; i < 100000; i++) {
            QByteArray name = QString::fromLatin1("col%1").arg(i+4).toLatin1();
            Collection col = initializer->createCollection(name.data(), col3);
            col.setEnabled(false);
            col.addMimeType(mt1);
            col.addMimeType(mt2);
            col.addMimeType(mt3);
            col.addMimeType(mt4);
            col.update();
        }
        for (int i = 0; i < 1000; i++) {
            QByteArray name = QString::fromLatin1("col%1").arg(i+100004).toLatin1();
            Collection col = initializer->createCollection(name.data(), col5);
            col.addMimeType(mt1);
            col.addMimeType(mt2);
            col.update();
        }
        qDebug() << "Created 100000 collections in" << t.elapsed() << "msecs";

        QTest::addColumn<QList<QByteArray> >("scenario");

        // {
        //     QList<QByteArray> scenario;
        //     scenario << FakeAkonadiServer::defaultScenario()
        //             << "C: 2 LIST " + QByteArray::number(toplevel.id()) + " INF (ENABLED  ) ()"
        //             << "S: IGNORE 1006"
        //             << "S: 2 OK List completed";
        //     QTest::newRow("recursive list of enabled") << scenario;
        // }
        // {
        //     QList<QByteArray> scenario;
        //     scenario << FakeAkonadiServer::defaultScenario()
        //             << "C: 2 LIST " + QByteArray::number(toplevel.id()) + " INF (MIMETYPE (mimetype1) RESOURCE \"testresource\") ()"
        //             // << "C: 2 LIST " + QByteArray::number(0) + " INF (RESOURCE \"testresource\") ()"
        //             << "S: IGNORE 101005"
        //             << "S: 2 OK List completed";
        //     QTest::newRow("recursive list filtered by mimetype") << scenario;
        // }
        {
            QList<QByteArray> scenario;
            scenario << FakeAkonadiServer::defaultScenario()
                    << "C: 2 LIST " + QByteArray::number(toplevel.id()) + " INF (ANCESTORS INF MIMETYPE (mimetype1) RESOURCE \"testresource\") ()"
                    << "S: IGNORE 101005"
                    << "S: 2 OK List completed";
            QTest::newRow("recursive list filtered by mimetype with ancestors") << scenario;
        }
    }

    void testListEnabledBenchmark()
    {
        QFETCH(QList<QByteArray>, scenario);
        // StorageDebugger::instance()->enableSQLDebugging(true);
        // StorageDebugger::instance()->writeToFile(QLatin1String("sqllog.txt"));

        QBENCHMARK {
            FakeAkonadiServer::instance()->setScenario(scenario);
            FakeAkonadiServer::instance()->runTest();
        }
    }

#endif

};

AKTEST_FAKESERVER_MAIN(ListHandlerTest)

#include "listhandlertest.moc"
