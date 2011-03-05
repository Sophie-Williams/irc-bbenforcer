/*****************************************************************************************
 * ioQIC-BBEnforcer - Qt irc bot that also sends rcon commands to UrbanTerror game server*
 * Copyright (C) 2010 - 2011, (n)woki                                                    *
 *                                                                                       *
 * gamecontroller.cpp is part of ioQIC-BBEnforcer                                        *
 *                                                                                       *
 * ioQIC-BBEnforcer is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software Foundation, *
 * either version 3 of the License, or (at your option) any later version.               *
 *                                                                                       *
 * ioQIC-BBEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY   *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A       *
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.             *
 *                                                                                       *
 * You should have received a copy of the GNU General Public License along with this     *
 * program.  If not, see <http://www.gnu.org/licenses/>.                                 *
 ****************************************************************************************/

#include "dbcontroller.h"
#include "gamecontroller.h"

#include <QDir>
#include <QSettings>

#define RCON_START "\xff\xff\xff\xffrcon "


GameController::GameController( DbController *db )
    : QObject( 0 )
    , m_db( db )
    , m_socket( new QUdpSocket() )
    , m_ip( QString() )
    , m_rconPass( QString() )
    , m_port( 0 )
{
    loadSettings();

    connect( m_socket, SIGNAL( connected() ), this, SLOT( connectNotify() ) );

    // connect
    m_socket->connectToHost( m_ip, m_port, QIODevice::ReadWrite );
}


QUdpSocket *GameController::connectionSocket() const
{
    return m_socket;
}


void GameController::gameCommandParser( const QByteArray& user, const QByteArray& msg, const QByteArray& ip )
{
    qDebug() << "MESSAGE I GET IS -> " << msg;
    QList< QByteArray > msgList = msg.split( ' ' ); // split message

    QString cmd = msgList.at( 0 );              // command given by user

    if( cmd == "@bigtext" )                     // send bigtext to server
        bigText( user, ip, msgList );
    else if( cmd == "@gravity" )                // set server gravity
        gravity( user, ip, msgList );
    else if( cmd == "@nextmap" )                // set server nextmap
        nextMap( user, ip, msgList );
    else if( cmd == "@map" )                    // set server current map
        map( user, ip, msgList );
    else if( cmd == "@status" )                 // send "status" command to server
        status( user, ip );
}


QByteArray GameController::nextUserInLine()
{
    QByteArray user;
    if( !m_userList.isEmpty() ) {   // no users in line
        user = m_userList.first();
        m_userList.pop_front();     // remove user from list
    }
    return user;
}


/*******************************
 *      PRIVATE FUNCTIONS      *
 *******************************/

/*********
 * SLOTS *
 ********/
void GameController::connectNotify()
{
    qDebug() << "Connected to host " << m_ip << ":" << m_port;
}


/*****************
* game functions *
*****************/
void GameController::bigText( const QByteArray& user, const QByteArray& ip, const QList< QByteArray >& msgList )
{
    qDebug( "GameController::bigText" );

    if( msgList.count() > 1 ) {
    QByteArray bigtext;
        if( m_db->isAuthed( user, ip ) ) {  // gerate command
            QByteArray cmd( RCON_START );
            cmd.append( m_rconPass );
            cmd.append( " bigtext " );

            for( int i = 1; i < msgList.count(); i++ ) {
                bigtext.append( msgList.at( i ) );
                bigtext.append( "." );
            }

            m_socket->write( cmd + bigtext );
            emit( messageToUserSignal( user, "sent bigtext: '" + bigtext.replace( ".", " " ) + "'" ) );
        }
        else
            emit( notAuthedSignal( user ) );
    }
    else
        emit( messageToUserSignal( user, "wrong number of parameters. [usage]@bigtext <text>" ) );
}


void GameController::gravity( const QByteArray& user, const QByteArray& ip, const QList< QByteArray >& msgList )
{
    qDebug( "GameController::gravity" );

    if( msgList.count() == 2 ) {
        bool ok;

        msgList.at( 1 ).toInt( &ok );

        if( !ok ) {
            emit( messageToUserSignal( user, "the value for gravity has to be a number" ) );
            return;
        }

        if( m_db->isAuthed( user, ip ) ) {  // gerate command
            QByteArray cmd( RCON_START );
            cmd.append( m_rconPass );
            cmd.append( " set g_gravity " );
            cmd.append( msgList.at( 1 ) );  // map

            m_socket->write( cmd );
            emit( messageToUserSignal( user, "gravity set to " + msgList.at( 1 ) ) );
        }
        else
            emit( notAuthedSignal( user ) );
    }
    else
        emit( messageToUserSignal( user, "wrong number of parameters. [usage]@gravity <num>" ) );
}


void GameController::map( const QByteArray& user, const QByteArray& ip, const QList< QByteArray >& msgList )
{
    qDebug( "GameController::map" );

    if( msgList.count() == 2 ) {
        if( m_db->isAuthed( user, ip ) ) {  // gerate command
            QByteArray cmd( RCON_START );
            cmd.append( m_rconPass );
            cmd.append( " map " );
            cmd.append( msgList.at( 1 ) );  // map

            m_socket->write( cmd );
            emit( messageToUserSignal( user, "map set to: " + msgList.at( 1 ) ) );
        }
        else
            emit( notAuthedSignal( user ) );
    }
    else
        emit( messageToUserSignal( user, "wrong number of parameters. [usage]@map <map>" ) );
}


void GameController::nextMap( const QByteArray& user, const QByteArray& ip, const QList< QByteArray >& msgList )
{
    qDebug( "GameController::nextMap" );

    if( msgList.count() == 2 ) {
        if( m_db->isAuthed( user, ip ) ) {  // gerate command
            QByteArray cmd( RCON_START );
            cmd.append( m_rconPass );
            cmd.append( " set g_nextmap " );
            cmd.append( msgList.at( 1 ) );  // map

            m_socket->write( cmd );
            emit( messageToUserSignal( user, "next map set to: " + msgList.at( 1 ) ) );
        }
        else
            emit( notAuthedSignal( user ) );
    }
    else
        emit( messageToUserSignal( user, "wrong number of parameters. [usage]@nextmap <map>" ) );
}


void GameController::status( const QByteArray& user, const QByteArray &ip )
{
    qDebug( "GameController::status" );
    if( m_db->isAuthed( user, ip ) ) {  // generate status command
        QByteArray cmd( RCON_START );
        cmd.append( m_rconPass );
        cmd.append( " status" );

        m_socket->write( cmd );
        m_userList.append( user );      // add user to list
    }
    else                                // tell user he/she's not authed
        emit( notAuthedSignal( user ) );
}


/***********
 * PRIVATE *
 **********/
void GameController::loadSettings()
{
    qDebug( "GameController::loadSettings" );
    //set config file
    QSettings settings( QDir::toNativeSeparators( "cfg/config" ), QSettings::IniFormat );

    if( settings.status() == QSettings::FormatError ) {
        qWarning( "\e[1;31m[FAIL] GameController::loadSettings FAILED to load settings. Format Error, check your config file\e[0m" );
        return;
    }

    qDebug( "GameController::loadSettings GAME SERVER SETTINGS" );

    settings.beginReadArray( "GAMESERVER" );

    m_ip = settings.value( "ip" ).toString();
    if( m_ip.isEmpty() ) {
        qWarning( "\e[1;31m[FAIL]GameController::loadSettings can't load 'ip'. Check your config file\e[0m" );
        return;
    }

    bool ok;
    m_port = settings.value( "port" ).toInt( &ok );
    if( !ok ) {
        qWarning( "\e[1;31m[FAIL]GameController::loadSettings can't load 'port'. Check your config file.\e[0m" );
        return;
    }

    m_rconPass = settings.value( "rconPass" ).toString();

    if( m_rconPass.isEmpty() )
        qWarning( "\e[1;31mWARNING: empty rcon pass!" );

    // close settings file
    settings.endArray();
    qDebug() << "Settings: " << m_ip << "  " << m_port << " " << m_rconPass;
}