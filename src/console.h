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
#ifndef CONSOLE_H
#define CONSOLE_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Virtlyst;
class Console : public Controller
{
    Q_OBJECT
public:
    explicit Console(Virtlyst *parent = nullptr);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c, const QString &hostId, const QString &uuid);

    C_ATTR(ws_vnc, :Local :AutoArgs)
    void ws_vnc(Context *c, const QString &hostId, const QString &uuid);

private:
    Virtlyst *m_virtlyst;
};

#endif // CONSOLE_H
