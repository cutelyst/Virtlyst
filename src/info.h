#ifndef INFO_H
#define INFO_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Info : public Controller
{
    Q_OBJECT
public:
    explicit Info(QObject *parent = 0);

    C_ATTR(insts_status, :Local :AutoArgs)
    void insts_status(Context *c, const QString &hostId);

    C_ATTR(instusage, :Local :AutoArgs)
    void instusage(Context *c, const QString &hostId, const QString &name);

private Q_SLOTS:
};

#endif // INFO_H
