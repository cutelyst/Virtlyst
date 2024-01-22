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
#include "root.h"

#include "virtlyst.h"

#include <Cutelyst/Plugins/Authentication/authentication.h>
#include <Cutelyst/Plugins/StatusMessage>
#include <libvirt/libvirt.h>

#include <QLoggingCategory>

using namespace Cutelyst;

Root::Root(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

Root::~Root()
{
}

void Root::index(Context *c)
{
    c->response()->redirect(c->uriForAction(QStringLiteral("/server/index")));
}

void Root::login(Context *c)
{
    Request *req = c->request();
    if (req->isPost()) {
        const ParamsMultiMap params = req->bodyParams();
        const QString username      = params.value(QStringLiteral("username"));
        const QString password      = params.value(QStringLiteral("password"));
        if (!username.isEmpty() && !password.isEmpty()) {
            // Authenticate
            if (Authentication::authenticate(c, params)) {
                qDebug() << Q_FUNC_INFO << username << "is now Logged in";
                c->res()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
                return;
            } else {
                c->setStash(QStringLiteral("error_msg"), c->tr("Wrong password or username"));
                qDebug() << Q_FUNC_INFO << username << "user or password invalid";
            }
        } else {
            qWarning() << "Empty username and password";
        }
        c->res()->setStatus(Response::Forbidden);
    }

    c->setStash(QStringLiteral("template"), QStringLiteral("login.html"));
}

void Root::logout(Context *c)
{
    Authentication::logout(c);
    c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
}

void Root::defaultPage(Context *c)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("404.html"));
    c->response()->setStatus(404);
}

bool Root::Auto(Context *c)
{
    StatusMessage::load(c);

    if (c->action() == CActionFor(QStringLiteral("login"))) {
        return true;
    }

    if (!Authentication::userExists(c)) {
        c->res()->redirect(c->uriFor(CActionFor(QStringLiteral("login"))));
        return false;
    }

    c->setStash(QStringLiteral("user"), Authentication::user(c));
    c->setStash(QStringLiteral("time_refresh"), 8000);

    m_virtlyst->updateConnections();

    return true;
}
