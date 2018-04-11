#ifndef CREATE_H
#define CREATE_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Virtlyst;
class Create : public Controller
{
    Q_OBJECT
public:
    explicit Create(Virtlyst *parent = 0);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c, const QString &hostId);

private:
    Virtlyst *m_virtlyst;
};

#endif // CREATE_H
