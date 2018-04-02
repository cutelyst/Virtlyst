#ifndef INSTANCE_H
#define INSTANCE_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Instance : public Controller
{
    Q_OBJECT
public:
    explicit Instance(QObject *parent = 0);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c, const QString &hostId, const QString &name);

private Q_SLOTS:
};

#endif // INSTANCE_H
