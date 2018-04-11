#include "create.h"

#include "virtlyst.h"
#include "lib/connection.h"

using namespace Cutelyst;

Create::Create(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Create::index(Context *c, const QString &hostId)
{
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);
    c->setStash(QStringLiteral("template"), QStringLiteral("create.html"));

}
