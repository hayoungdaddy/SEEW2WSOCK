#include "mainclass.h"

MainClass::MainClass(QString configFileName, QObject *parent) : QObject(parent)
{
    activemq::library::ActiveMQCPP::initializeLibrary();

    cfg = readCFG(configFileName);

    qRegisterMetaType<_EEWINFO>("_EEWINFO");

    initProj();

    writeLog(cfg.logDir, cfg.processName, "======================================================");

    if(cfg.eew_amq_topic != "")
    {
        QString eewFailover = "failover:(tcp://" + cfg.eew_amq_ip + ":" + cfg.eew_amq_port + ")";

        rvEEW_Thread = new RecvEEWMessage;
        if(!rvEEW_Thread->isRunning())
        {
            rvEEW_Thread->setup(eewFailover, cfg.eew_amq_user, cfg.eew_amq_passwd, cfg.eew_amq_topic, true, false);
            connect(rvEEW_Thread, SIGNAL(_rvEEWInfo(_EEWINFO)), this, SLOT(rvEEWInfo(_EEWINFO)));
            rvEEW_Thread->start();
        }
    }

    writeLog(cfg.logDir, cfg.processName, "SEEW2SOCK Started");

    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("SEEW2SOCK"), QWebSocketServer::NonSecureMode,  this);

    if(m_pWebSocketServer->listen(QHostAddress::Any, cfg.websocketPort))
    {
        writeLog(cfg.logDir, cfg.processName, "Listening on port : " + QString::number(cfg.websocketPort));

        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &MainClass::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed,
                this, &QCoreApplication::quit);
    }
}

void MainClass::initProj()
{
    if (!(pj_longlat = pj_init_plus("+proj=longlat +ellps=WGS84")) )
    {
        qDebug() << "Can't initialize projection.";
        exit(1);
    }

    if (!(pj_eqc = pj_init_plus("+proj=eqc +ellps=WGS84")) )
    {
        qDebug() << "Can't initialize projection.";
        exit(1);
    }
}

void MainClass::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    connect(pSocket, &QWebSocket::disconnected, this, &MainClass::socketDisconnected);
    m_clients << pSocket;

    connect(pSocket, &QWebSocket::disconnected, this, &MainClass::socketDisconnected);
}

void MainClass::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());

    if(pClient){
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void MainClass::rvEEWInfo(_EEWINFO eewInfo)
{
    ll2xy4Large(pj_longlat, pj_eqc, eewInfo.longitude, eewInfo.latitude, &eewInfo.lmapX, &eewInfo.lmapY);
    ll2xy4Small(pj_longlat, pj_eqc, eewInfo.longitude, eewInfo.latitude, &eewInfo.smapX, &eewInfo.smapY);

    writeLog(cfg.logDir, cfg.processName, "EEW Received. : " +
               QString::number(eewInfo.eew_evid) + " " + QString::number(eewInfo.origintime) + " " +
               QString::number(eewInfo.latitude, 'f', 4) + " " + QString::number(eewInfo.longitude, 'f', 4) + " " +
               QString::number(eewInfo.magnitude, 'f', 1));

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.writeRawData((char*)&eewInfo, sizeof(_EEWINFO));

    for(int i=0;i<m_clients.size();i++)
    {
        QWebSocket *pClient = m_clients.at(i);
        pClient->sendBinaryMessage(data);
    }
}
