/*
 * Copyright (C) 2019 Daniel Nicoletti <dantti12@gmail.com>
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
#include "users.h"

#include <QLoggingCategory>

#include <Cutelyst/Plugins/Authentication/authentication.h>
#include <Cutelyst/Plugins/Authentication/credentialpassword.h>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QSqlQuery>
#include <QSqlError>

using namespace Cutelyst;

Users::Users(QObject *parent)
    : Controller(parent)
{
}

void Users::index(Context *c)
{
    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT id, username "
                               "FROM users "
                               "ORDER BY username"),
                QStringLiteral("virtlyst"));
    if (query.exec()) {
        c->setStash(QStringLiteral("users"), Sql::queryToList(query));
    } else {
        qDebug() << "error users" << query.lastError().text();
        c->response()->setStatus(Response::InternalServerError);
    }
}

void Users::create(Context *c)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        c->setStash(QStringLiteral("user"), params);

        const QString password = params[QStringLiteral("password")];
        if (password.size() < 10) {
            c->setStash(QStringLiteral("error_msg"), QStringLiteral("Password too short"));
            return;
        }
        const QString pass = CredentialPassword::createPassword(password);

        QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password) "),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":username"), params.value(QStringLiteral("username")));
        query.bindValue(QStringLiteral(":password"), pass);
        if (query.exec()) {
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
        } else {
            qDebug() << "error create user" << query.lastError().text();
            c->response()->setStatus(Response::InternalServerError);
        }
    }
    c->setStash(QStringLiteral("user"), ParamsMultiMap{
                    {QStringLiteral("active"), QStringLiteral("on")},
                });
}

void Users::edit(Context *c, const QString &id)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("UPDATE users "
                                   "SET "
                                   "username=:username "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":username"), params.value(QStringLiteral("username")));
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
        } else {
            qDebug() << "error users" << query.lastError().text();
            c->response()->setStatus(Response::InternalServerError);
        }
    } else {
        QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("SELECT username "
                                   "FROM users "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {
            c->setStash(QStringLiteral("user"), Sql::queryToHashObject(query));
        } else {
            qDebug() << "error users" << query.lastError().text();
            c->response()->setStatus(Response::InternalServerError);
        }
    }
    c->setStash(QStringLiteral("user_edit"), true);
    c->setStash(QStringLiteral("template"), QStringLiteral("users/create.html"));
}

void Users::change_password(Context *c, const QString &id)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        const QString password = params[QStringLiteral("password")];
        if (password != params[QStringLiteral("password_conf")]) {
            c->setStash(QStringLiteral("error_msg"), QStringLiteral("Password confirmation does not match"));
            return;
        }
        const QString pass = CredentialPassword::createPassword(password);

        QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("UPDATE users "
                                   "SET password=:password "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":password"), pass);
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
        } else {
            qDebug() << "error users" << query.lastError().text();
            c->response()->setStatus(Response::InternalServerError);
        }
    }

    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT name "
                               "FROM users "
                               "WHERE id=:id"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    if (query.exec()) {
        c->setStash(QStringLiteral("user"), Sql::queryToHashObject(query));
    } else {
        qDebug() << "error users" << query.lastError().text();
        c->response()->setStatus(Response::InternalServerError);
    }
}
