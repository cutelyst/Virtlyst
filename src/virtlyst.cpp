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
#include "virtlyst.h"

#include <Cutelyst/Plugins/View/Grantlee/grantleeview.h>
#include <Cutelyst/Plugins/Utils/Sql>
#include <Cutelyst/Plugins/StatusMessage>
#include <Cutelyst/Plugins/Session/Session>
#include <Cutelyst/Plugins/Authentication/credentialpassword.h>
#include <Cutelyst/Plugins/Authentication/authenticationrealm.h>
#include <grantlee/engine.h>

#include <QFile>
#include <QMutexLocker>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QStandardPaths>
#include <QDebug>

#include "infrastructure.h"
#include "instances.h"
#include "instance.h"
#include "info.h"
#include "root.h"

#include "sqluserstore.h"

using namespace Cutelyst;

static QMutex mutex;

Virtlyst::Virtlyst(QObject *parent) : Application(parent)
{
}

Virtlyst::~Virtlyst()
{
}

bool Virtlyst::init()
{
    new Root(this);
    new Infrastructure(this);
    new Instances(this);
    new Instance(this);
    new Info(this);

    bool production = config(QStringLiteral("production")).toBool();
    qDebug() << "Production" << production;

    m_dbPath = config(QStringLiteral("DatabasePath"),
                      QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QLatin1String("/sticklyst.sqlite")).toString();
    qDebug() << "Database" << m_dbPath;
    if (!QFile::exists(m_dbPath)) {
        if (!createDB()) {
            qDebug() << "Failed to create database" << m_dbPath;
            return false;
        }
        QSqlDatabase::removeDatabase(QStringLiteral("db"));
    }

    auto view = new GrantleeView(this);
    view->setCache(production);
    view->engine()->addDefaultLibrary(QStringLiteral("grantlee_i18ntags"));
//    view->addTranslator(QLocale::system(), new QTranslator(this));
    view->setIncludePaths({ pathTo(QStringLiteral("root/src")) });

    auto store = new SqlUserStore;

    auto password = new CredentialPassword;
    password->setPasswordField(QStringLiteral("password"));
    password->setPasswordType(CredentialPassword::Hashed);

    auto realm = new AuthenticationRealm(store, password);

    new Session(this);

    auto auth = new Authentication(this);
    auth->addRealm(realm);

    new StatusMessage(this);

    return true;
}

bool Virtlyst::postFork()
{
    QMutexLocker locker(&mutex);

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), Cutelyst::Sql::databaseNameThread(QStringLiteral("sticklyst")));
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qWarning() << "Failed to open database" << db.lastError().databaseText();
        return false;
    }

    qDebug() << "Database ready" << db.connectionName();
}

QString Virtlyst::prettyKibiBytes(quint64 kibiBytes)
{
    QString ret;
    const char* suffixes[6];
    suffixes[0] = " KiB";
    suffixes[1] = " MiB";
    suffixes[2] = " GiB";
    suffixes[3] = " TiB";
    suffixes[4] = " PiB";
    suffixes[5] = " EiB";
    uint s = 0; // which suffix to use
    double count = kibiBytes;
    while (count >= 1024 && s < 6) {
        count /= 1024;
        s++;
    }
    ret = QString::number(count, 'g', 3) + QLatin1String(suffixes[s]);
    return ret;
}

bool Virtlyst::createDB()
{
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("db"));
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qWarning() << "Failed to open database" << db.lastError().databaseText();
        return false;
    }

    QSqlQuery query(db);
    qDebug() << "Creating database" << m_dbPath;

    bool ret = query.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    qDebug() << "PRAGMA journal_mode = WAL" << ret << query.lastError().databaseText();

    if (!query.exec(QStringLiteral("CREATE TABLE users "
                                   "( id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT"
                                   ", username TEXT UNIQUE NOT NULL "
                                   ", password TEXT NOT NULL "
                                   ")"))) {
        qCritical() << "Error creating database" << query.lastError().text();
        return false;
    }

    if (!query.exec(QStringLiteral("INSERT INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "('admin', 'admin')"))) {
        qCritical() << "Error creating database" << query.lastError().text();
        return false;
    }

    return true;
}

