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
#ifndef SERVER_H
#define SERVER_H

#include <Cutelyst/Controller>

#include <QObject>

using namespace Cutelyst;

class Virtlyst;
class Server : public Cutelyst::Controller
{
    Q_OBJECT
public:
    explicit Server(Virtlyst *parent = nullptr);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c);

private:
    void createServer(int type,
                      const QString &name,
                      const QString &hostname,
                      const QString &login,
                      const QString &password);
    void updateServer(int id,
                      const QString &name,
                      const QString &hostname,
                      const QString &login,
                      const QString &password);
    void deleteServer(int id);

    Virtlyst *m_virtlyst;
};

#endif // SERVER_H
