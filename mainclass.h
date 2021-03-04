#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QDataStream>

#include "KGEEWLIBS_global.h"
#include "kgeewlibs.h"

#define SEEW2WSOCK_VERSION 0.1

class MainClass : public QObject
{
    Q_OBJECT
public:
    explicit MainClass(QString conFile = nullptr, QObject *parent = nullptr);

private:
    _CONFIGURE cfg;
    RecvEEWMessage *rvEEW_Thread;

    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;

    projPJ pj_eqc;
    projPJ pj_longlat;
    void initProj();

private slots:
    void onNewConnection();
    void socketDisconnected();
    void rvEEWInfo(_EEWINFO);
};

#endif // MAINCLASS_H
