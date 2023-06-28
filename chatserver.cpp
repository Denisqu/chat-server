#include <QTcpSocket>
#include <QDebug>
#include <QString>
#include "chatserver.h"


namespace {

template<class T>
T const as_const(T&&t){return std::forward<T>(t);}
template<class T>
T const& as_const(T&t){return t;}

}

ChatServer::ChatServer(QObject *parent):QTcpServer()
{
    qDebug() << listen(QHostAddress(QString("127.0.0.1")), 1984);
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "incoming connection: " << socketDescriptor;
    // This method will get called every time a client tries to connect.
    // We create an object that will take care of the communication with this client
    ServerWorker *worker = new ServerWorker(this);
    // we attempt to bind the worker to the client
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "failed to bind worker";
        // if we fail we clean up
        worker->deleteLater();
        return;
    }
    // connect the signals coming from the object that will take care of the
    // communication with this client to the slots in the central server
    connect(worker, &ServerWorker::disconnectedFromClient, this, std::bind(&ChatServer::userDisconnected, this, worker));
    connect(worker, &ServerWorker::error, this, std::bind(&ChatServer::userError, this, worker));
    connect(worker, &ServerWorker::jsonReceived, this, std::bind(&ChatServer::jsonReceived, this, worker, std::placeholders::_1));
    connect(worker, &ServerWorker::logMessage, this, &ChatServer::logMessage);
    // we append the new worker to a list of all the objects that communicate to a single client
    m_clients.append(worker);
    // we log the event
    emit logMessage(QStringLiteral("New client Connected"));
}

void ChatServer::stopServer()
{

}

void ChatServer::sendJson(ServerWorker *destination, const QJsonObject &message)
{
    qDebug() << "sending json to " << destination << " msg = " << message;
    Q_ASSERT(destination); // make sure destination is not null
    destination->sendJson(message); // call directly the worker method
}

void ChatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
    // iterate over all the workers that interact with the clients
    for (ServerWorker *worker : as_const(m_clients)) {
        if (worker == exclude)
            continue; // skip the worker that should be excluded
        sendJson(worker, message); //send the message to the worker
    }
}

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &doc)
{
    qDebug() << "json received:" << doc;
    broadcast(doc, sender);
}

void ChatServer::userDisconnected(ServerWorker *sender)
{

}

void ChatServer::userError(ServerWorker *sender)
{

}

void ChatServer::jsonFromLoggedOut(ServerWorker *sender, const QJsonObject &doc)
{

}

void ChatServer::jsonFromLoggedIn(ServerWorker *sender, const QJsonObject &doc)
{

}
