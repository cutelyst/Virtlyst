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
#ifndef ROOT_H
#define ROOT_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Virtlyst;
class Root : public Controller
{
    Q_OBJECT
    C_NAMESPACE("")
public:
    explicit Root(Virtlyst *parent = nullptr);
    ~Root();

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c);

    C_ATTR(login, :Local :AutoArgs)
    void login(Context *c);

    C_ATTR(logout, :Local :AutoArgs)
    void logout(Context *c);

    C_ATTR(defaultPage, :Path)
    void defaultPage(Context *c);

private Q_SLOTS:
    bool Auto(Context *c);

private:
    C_ATTR(End, :ActionClass("RenderView"))
    void End(Context *c) { Q_UNUSED(c); }

    Virtlyst *m_virtlyst;
};

#endif //ROOT_H

