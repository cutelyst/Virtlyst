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
#include "root.h"

#include <libvirt/libvirt.h>

using namespace Cutelyst;

Root::Root(QObject *parent) : Controller(parent)
{
}

Root::~Root()
{
}

void Root::index(Context *c)
{
    c->response()->body() = "Welcome to Cutelyst!";

    virConnectPtr conn;
    char *host;

    conn = virConnectOpen("qemu:///system");
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    host = virConnectGetHostname(conn);
    fprintf(stdout, "Hostname:%s\n", host);
    free(host);

    int vcpus = virConnectGetMaxVcpus(conn, NULL);
    fprintf(stdout, "Maximum support virtual CPUs: %d\n", vcpus);

    int i;
    int numDomains;
    int *activeDomains;

    numDomains = virConnectNumOfDomains(conn);

    activeDomains = new int[numDomains];
    numDomains = virConnectListDomains(conn, activeDomains, numDomains);

    printf("Active domain IDs:\n");
    for (i = 0 ; i < numDomains ; i++) {
        printf("  %d\n", activeDomains[i]);
    }
    delete [] activeDomains;

    virConnectClose(conn);
    return;
}

void Root::login(Context *c)
{

}

void Root::defaultPage(Context *c)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("404.html"));
    c->response()->setStatus(404);
}

