/***************************************************************************************
* ioQIC - Qt irc bot that also sends rcon commands to UrbanTerror game server          *
* Copyright (C) 2010, woki                                                             *
*                                                                                      *
* Connection.cpp is part of ioQIC                                                      *
*                                                                                      *
* ioQIC is free software: you can redistribute it and/or modify it under the           *
* terms of the GNU General Public License as published by the Free Software Foundation,*
* either version 3 of the License, or (at your option) any later version.              *
*                                                                                      *
* ioQIC is distributed in the hope that it will be useful, but WITHOUT ANY             *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.            *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with this    *
* program.  If not, see <http://www.gnu.org/licenses/>.                                *
****************************************************************************************/

#include "Connection.h"

#include <iostream>
#include <QAbstractSocket>
#include <QDir>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

Connection::Connection( QAbstractSocket::SocketType type/*, const QString &ip, int port */)
    : m_port( 0 )
    , m_socket( 0 )
    , m_settings( 0 )
    , m_ip( 0 )
//    , m_socketType( 0 )
{
    if( type == QAbstractSocket::TcpSocket )
        m_socket = new QTcpSocket();
    else if( type == QAbstractSocket::UdpSocket )
        m_socket = new QUdpSocket();

    //set config file
    m_settings = new QSettings( QDir::toNativeSeparators( "cfg/config" ), QSettings::IniFormat );
    loadSettings();
}

Connection::~Connection()
{
    delete m_socket;
}

void Connection::loadSettings()
{
    if( !m_settings ) {
        qWarning( "\e[1;31m Connection::loadSettings FAILed to load settings. Check your config file" );
        return;
    }

    if( m_socket->socketType() == QAbstractSocket::TcpSocket ) {    //load irc settings
        qDebug( "Connection::loadSettings IRC SETTINGS" );
        int values = m_settings->beginReadArray( "IRC" );
        //if( values < 0 )
        qDebug() << values;

        qDebug() << m_settings->value( "ip" );
        m_ip = m_settings->value( "ip" ).toString();
        m_port = m_settings->value( "port" ).toInt();
    }
    else if( m_socket->socketType() == QAbstractSocket::UdpSocket ) {   //load game settings

    }
    else {
        qWarning( "Connection::loadSettings Unknown socket type, not loading settings" );
        return;
    }
}

void Connection::startConnect()
{
    qDebug( "Connection::startConnect");
    if( !m_socket ) {
        qWarning( "Connection::connect no socket initialized for connection" );
        return;
    }

    //connection notification
    connect( m_socket, SIGNAL( connected() ), this, SLOT( connectNotify() ) );

    //connection error
    connect( m_socket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( handleSocketErrors( QAbstractSocket::SocketError ) ) );

    //disconnect notification
    connect( m_socket, SIGNAL( disconnected() ), this, SLOT( disconnectNotify()) );

    m_socket->connectToHost( m_ip, m_port, QIODevice::ReadWrite );
}

QAbstractSocket *Connection::socket()
{
    if( !m_socket ) {
        qWarning( "Connection::socket no socket to return! (created empty one) " );
        return 0;
//        if( m_type == "tcp" )
//           return new QTcpSocket();
//        else if( m_type == "udp" )
//            return new QUdpSocket();
    }
    else
        return m_socket;
}


/*******************
*      SLOTS       *
********************/

void Connection::connectNotify()
{
    //std::cout << "Connected to host " << m_ip << ":" << m_port << std::endl;
    qDebug() << "Connected to host " << m_ip << ":" << m_port;
}

void Connection::disconnectNotify()
{
    qDebug( "Connection::disconnectNotify" );
}

void Connection::handleSocketErrors( QAbstractSocket::SocketError error )
{
    qWarning()<< "Connection::handleSocketErrors \e[1;31m ERR:" << m_socket->errorString() << "\e[0m";
    m_socket->disconnectFromHost(); // or "abort()"?

    switch ( error ) {
        case QAbstractSocket::ConnectionRefusedError: {
            std::cout << "\e[0;33m reconnecting..\e[0m" << std::endl;
            QTimer::singleShot( 5000, this, SLOT( reconnect() ) );
            //what to do?
            break;
        }
        case QAbstractSocket::RemoteHostClosedError: {
            //host closed connection
                //reconnect
            break;
        }
        case QAbstractSocket::HostNotFoundError: {
            std::cout << "\e[0;33mPlease check that all info was inserted correctly\e[0m" << std::endl;
            break;
        }
        case QAbstractSocket::SocketAccessError: {
            std::cout << "\e[0;33 the application lacks the required privileges\e[0m" << std::endl;
            break;
        }
        case QAbstractSocket::SocketTimeoutError: {
            std::cout << "\e[0;33 reconnecting.." << std::endl;
            break;
        }
    }
}

void Connection::reconnect()
{
    QAbstractSocket *aux = m_socket;
    m_socket = 0;

    if( aux->socketType() == QAbstractSocket::TcpSocket )
        m_socket = new QTcpSocket();
    else if( aux->socketType() == QAbstractSocket::UdpSocket )
        m_socket = new QUdpSocket();

    delete aux;
    qDebug( "Connection::reconnect" );
    startConnect();
}
