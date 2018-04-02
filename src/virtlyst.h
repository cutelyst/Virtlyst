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
#ifndef VIRTLYST_H
#define VIRTLYST_H

#include <Cutelyst/Application>

using namespace Cutelyst;

class Virtlyst : public Application
{
    Q_OBJECT
    CUTELYST_APPLICATION(IID "Virtlyst")
public:
    Q_INVOKABLE explicit Virtlyst(QObject *parent = 0);
    ~Virtlyst();

    bool init() override;

    bool postFork() override;

    static QString prettyKibiBytes(quint64 kibiBytes);

private:
    bool createDB();

    QString m_dbPath;
};

#endif //VIRTLYST_H

