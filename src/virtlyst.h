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

#include <QHash>
#include <QSharedPointer>
#include <QUrl>

using namespace Cutelyst;

class Connection;
class ServerConn : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QString hostname MEMBER hostname CONSTANT)
    Q_PROPERTY(QString login MEMBER login CONSTANT)
    Q_PROPERTY(QString password MEMBER password CONSTANT)
    Q_PROPERTY(int type MEMBER type CONSTANT)
    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(bool alive READ alive CONSTANT)
public:
    enum ServerType {
        ConnTCP = 1,
        ConnSSH,
        ConnTLS,
        ConnSocket,
    };
    ServerConn(QObject *parent) : QObject(parent) {}
    ~ServerConn() {}

    bool alive();
    ServerConn *clone(QObject *parent);

    int id;
    QString name;
    QString hostname;
    QString login;
    QString password;
    int type;
    QUrl url;
    Connection *conn = nullptr;
};

class QSqlQuery;
class Virtlyst : public Application
{
    Q_OBJECT
    CUTELYST_APPLICATION(IID "Virtlyst")
public:
    Q_INVOKABLE explicit Virtlyst(QObject *parent = nullptr);
    ~Virtlyst() override;

    bool init() override;

    bool postFork() override;

    QVector<ServerConn *> servers(QObject *parent);

    Connection *connection(const QString &id, QObject *parent);

    static QString prettyKibiBytes(quint64 kibiBytes);

    static QStringList keymaps();

    static bool createDbFlavor(QSqlQuery &query, const QString &label, int memory, int vcpu, int disk);

    void updateConnections();

private:
    bool createDB();

    QMap<QString, ServerConn *> m_connections;
    QString m_dbPath;
};

#endif //VIRTLYST_H

