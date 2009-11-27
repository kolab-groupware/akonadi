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

#ifndef AKONADI_QUERYBUILDER_H
#define AKONADI_QUERYBUILDER_H

#include "query.h"
#include "../akonadiprivate_export.h"

#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlQuery>

#ifdef QUERYBUILDER_UNITTEST
class QueryBuilderTest;
#endif

namespace Akonadi {

/**
  Helper class to construct arbitrary SQL queries.
*/
class AKONADIPRIVATE_EXPORT QueryBuilder
{
  public:
    enum QueryType {
      Select,
      Insert,
      Update,
      Delete
    };

    enum DatabaseType {
      Unknown,
      Sqlite,
      MySQL,
      PostgreSQL
    };

    /**
      Creates a new query builder.
    */
    explicit QueryBuilder( QueryType type = Select );

    /**
      Sets the database which should execute the query. Unfortunately the SQL "standard"
      is not interpreted in the same way everywhere...
    */
    void setDatabaseType( DatabaseType type );

    /**
      Add a table to the FROM part of the query.
      @param table The table name.
    */
    void addTable( const QString &table );

    /**
      Adds the given columns to a select query.
      @param cols The columns you want to select.
    */
    void addColumns( const QStringList &cols );

    /**
      Adds the given column to a select query.
      @param col The column to add.
    */
    void addColumn( const QString &col );

    /**
      Add a WHERE condition which compares a column with a given value.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
    */
    void addValueCondition( const QString &column, Query::CompareOperator op, const QVariant &value );

    /**
      Add a WHERE condition which compares a column with another column.
      @param column The column that should be compared.
      @param op The operator used for comparison.
      @param column2 The column @p column is compared to.
    */
    void addColumnCondition( const QString &column, Query::CompareOperator op, const QString &column2 );

    /**
      Add a WHERE condition. Use this to build hierarchical conditions.
    */
    void addCondition( const Query::Condition &condition );

    /**
      Define how WHERE condition are combined.
      @todo Give this method a better name.
    */
    void setSubQueryMode( Query::LogicOperator op );

    /**
      Add sort column.
      @param column Name of the column to sort.
      @param order Sort order
    */
    void addSortColumn( const QString &column, Query::SortOrder order = Query::Ascending );

    /**
      Sets a column to the given value (only valid for INSERT and UPDATE queries).
      @param column Column to change.
      @param value The value @p column should be set to.
    */
    void setColumnValue( const QString &column, const QVariant &value );

    /**
     * Specify whether duplicates should be included in the result.
     * @param distinct @c true to remove duplicates, @c false is the default
     */
    void setDistinct( bool distinct );

    /**
      Returns the query, only valid after exec().
    */
    QSqlQuery& query();

    /**
      Executes the query, returns true on success.
    */
    bool exec();

    /**
      Returns the ID of the newly created record (only valid for INSERT queries)
      @returns -1 if invalid
    */
    qint64 insertId();

    /**
      Converts Qt database driver names into database types.
    */
    static DatabaseType qsqlDriverNameToDatabaseType( const QString &driverName );

  private:
    QString bindValue( const QVariant &value );
    QString buildWhereCondition( const Query::Condition &cond );

  private:
    DatabaseType mDatabaseType;
    Query::Condition mRootCondition;
    QStringList mTables;
    QSqlQuery mQuery;
    QueryType mType;
    QStringList mColumns;
    QList<QVariant> mBindValues;
    QList<QPair<QString, Query::SortOrder> > mSortColumns;
    QList<QPair<QString, QVariant> > mColumnValues;
    bool mDistinct;
#ifdef QUERYBUILDER_UNITTEST
    QString mStatement;
    friend class ::QueryBuilderTest;
#endif
};

}

#endif
