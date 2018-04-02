/***************************************************************************
 *   Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/
#include "sqluserstore.h"

#include <Cutelyst/Context>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QRegularExpression>

#include <QJsonDocument>
#include <QJsonObject>

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include <QLoggingCategory>

using namespace Cutelyst;

SqlUserStore::SqlUserStore(QObject *parent) : AuthenticationStore(parent)
{

}

Cutelyst::AuthenticationUser SqlUserStore::findUser(Cutelyst::Context *c, const Cutelyst::ParamsMultiMap &userinfo)
{
    Q_UNUSED(c)
    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT * FROM users WHERE email = :email"),
                QStringLiteral("cmlyst"));
    query.bindValue(QStringLiteral(":email"), userinfo.value(QStringLiteral("email")));
    if (query.exec() && query.next()) {
        QVariant userId = query.value(QStringLiteral("id"));

        AuthenticationUser user(userId.toString());

        int columns = query.record().count();
        // send column headers
        QStringList cols;
        const QSqlRecord record = query.record();
        for (int j = 0; j < columns; ++j) {
            cols.append(record.fieldName(j));
        }

        for (int j = 0; j < columns; ++j) {
            user.insert(cols.at(j), query.value(j));
        }

        return user;
    }

    return AuthenticationUser();
}

QString SqlUserStore::addUser(const ParamsMultiMap &user, bool replace)
{
    QSqlQuery query;
    if (replace) {
        query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT OR REPLACE INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password)"),
                    QStringLiteral("cmlyst"));
    } else {
        query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password)"),
                    QStringLiteral("cmlyst"));
    }

    query.bindValue(QStringLiteral(":email"), user.value(QStringLiteral("username")));
    query.bindValue(QStringLiteral(":password"), user.value(QStringLiteral("password")));

    if (!query.exec()) {
        qDebug() << "Failed to add new user:" << query.lastError().databaseText() << user;
        return QString();
    }
    return user.value(QStringLiteral("username"));
}
