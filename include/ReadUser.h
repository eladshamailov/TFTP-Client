//
// Created by nir on 17/01/17.
//

#ifndef CLIENT_TFTPPROTOCOLCLIENT_H
#define CLIENT_TFTPPROTOCOLCLIENT_H
#include <stdlib.h>
#include <iostream>
#include "../include/connectionHandler.h"
#include <packets/LOGRQ.h>
#include <packets/RRQ.h>
#include <packets/ACK.h>
#include <packets/BCAST.h>
#include <packets/ERROR.h>
#include <packets/DATA.h>
#include <packets/DELRQ.h>
#include <packets/DIRQ.h>
#include <packets/DISC.h>
#include <packets/WRQ.h>


class ListenToKeyboard {

    enum class Status {
        normal,
        readingFromServer,
        dir
    };

private:
    Status status;
    ConnectionHandler connectionHandler;

public:
    ListenToKeyboard(const ConnectionHandler &connectionHandler);

    void run(Packet);

    void setStatus(const string &status);

    const string &getStatus() const;

    Packet * createNewPacketFromKeyboard() const;
};


#endif //CLIENT_TFTPPROTOCOLCLIENT_H