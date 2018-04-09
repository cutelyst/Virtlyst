#ifndef STORAGEVOL_H
#define STORAGEVOL_H

#include <QObject>

class StorageVol : public QObject
{
    Q_OBJECT
public:
    explicit StorageVol(QObject *parent = nullptr);

signals:

public slots:
};

#endif // STORAGEVOL_H