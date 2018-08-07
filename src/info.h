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
#ifndef INFO_H
#define INFO_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Virtlyst;
class Info : public Controller
{
    Q_OBJECT
public:
    explicit Info(Virtlyst *parent = nullptr);

    C_ATTR(hostusage, :Local :AutoArgs)
    void hostusage(Context *c, const QString &hostId);

    C_ATTR(insts_status, :Local :AutoArgs)
    void insts_status(Context *c, const QString &hostId);

    C_ATTR(inst_status, :Local :AutoArgs)
    void inst_status(Context *c, const QString &hostId, const QString &name);

    C_ATTR(instusage, :Local :AutoArgs)
    void instusage(Context *c, const QString &hostId, const QString &name);

private Q_SLOTS:
    void End(Context *c) { Q_UNUSED(c); }

private:
    Virtlyst *m_virtlyst;
};

#endif // INFO_H
