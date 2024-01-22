/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "server.h"

#include "virtlyst.h"

#include <Cutelyst/Plugins/StatusMessage>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

Server::Server(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Server::index(Context *c)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("servers.html"));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("host_tcp_add"))) {
            const QString hostname = params.value(u"hostname"_qs);
            const QString name     = params.value(u"name"_qs);
            const QString login    = params.value(u"login"_qs);
            const QString password = params.value(u"password"_qs);

            createServer(ServerConn::ConnTCP, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_ssh_add"))) {
            const QString hostname = params.value(u"hostname"_qs);
            const QString name     = params.value(u"name"_qs);
            const QString login    = params.value(u"login"_qs);

            createServer(ServerConn::ConnSSH, name, hostname, login, QString());
        } else if (params.contains(QStringLiteral("host_tls_add"))) {
            const QString hostname = params.value(u"hostname"_qs);
            const QString name     = params.value(u"name"_qs);
            const QString login    = params.value(u"login"_qs);
            const QString password = params.value(u"password"_qs);

            createServer(ServerConn::ConnTLS, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_socket_add"))) {
            const QString name = params.value(u"name"_qs);

            createServer(ServerConn::ConnSocket,
                         name,
                         QStringLiteral("localhost"),
                         QLatin1String(""),
                         QString());
        } else if (params.contains(QStringLiteral("host_edit"))) {
            const int hostId       = params.value(u"host_id"_qs).toInt();
            const QString hostname = params.value(u"hostname"_qs);
            const QString name     = params.value(u"name"_qs);
            const QString login    = params.value(u"login"_qs);
            const QString password = params.value(u"password"_qs);

            updateServer(hostId, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_del"))) {
            const int hostId = params.value(u"host_id"_qs).toInt();

            deleteServer(hostId);
        }
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
        return;
    }

    c->setStash(QStringLiteral("servers"), QVariant::fromValue(m_virtlyst->servers(c)));
}

void Server::createServer(int type,
                          const QString &name,
                          const QString &hostname,
                          const QString &login,
                          const QString &password)
{
    QSqlQuery query =
        CPreparedSqlQueryThreadForDB(QStringLiteral("INSERT INTO servers_compute "
                                                    "(type, name, hostname, login, password) "
                                                    "VALUES "
                                                    "(:type, :name, :hostname, :login, :password)"),
                                     QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":type"), type);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":hostname"), hostname);
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":password"), password);
    if (!query.exec()) {
        qWarning() << "Failed to add connection" << query.lastError().databaseText();
    }
}

void Server::updateServer(int id,
                          const QString &name,
                          const QString &hostname,
                          const QString &login,
                          const QString &password)
{
    QSqlQuery query = CPreparedSqlQueryThreadForDB(QStringLiteral("UPDATE servers_compute "
                                                                  "SET "
                                                                  "name = :name, "
                                                                  "hostname = :hostname, "
                                                                  "login = :login, "
                                                                  "password = :password "
                                                                  "WHERE id = :id"),
                                                   QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":hostname"), hostname);
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":password"), password);
    if (!query.exec()) {
        qWarning() << "Failed to update connection" << query.lastError().databaseText();
    }
}

void Server::deleteServer(int id)
{
    QSqlQuery query = CPreparedSqlQueryThreadForDB(
        QStringLiteral("DELETE FROM servers_compute WHERE id = :id"), QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        qWarning() << "Failed to delete connection" << query.lastError().databaseText();
    }
}
