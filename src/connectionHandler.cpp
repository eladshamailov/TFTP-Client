#include <connectionHandler.h>
#include <packets/ACK.h>
#include <packets/DATA.h>
#include <packets/WRQ.h>
#include <packets/ERROR.h>
#include <bytesAndShortConvertor.h>
#include <packets/Packet.h>
#include <packets/DISC.h>
#include <packets/DIRQ.h>
#include <packets/WRQ.h>
#include <packets/ACK.h>
#include <packets/LOGRQ.h>
#include <packets/DELRQ.h>
#include <packets/RRQ.h>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

ConnectionHandler::ConnectionHandler(string host, short port) :lastPacketSent(0), host_(host), port_(port),shouldTerminate(false), io_service_(),
                                                                socket_(io_service_),encdec(messageEncoderDecoder()), convertor() {



}

ConnectionHandler::~ConnectionHandler() {
    close();
}

bool ConnectionHandler::connect() {
    std::cout << "Starting connect to "
              << host_ << ":" << port_ << std::endl;
    try {
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(host_), port_); // the server endpoint
        boost::system::error_code error;
        socket_.connect(endpoint, error);
        if (error)
            throw boost::system::system_error(error);
    }
    catch (std::exception &e) {
        std::cerr << "Connection failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::getBytes(char bytes[], unsigned int bytesToRead) {
    size_t tmp = 0;
    boost::system::error_code error;
    try {
        while (!error && bytesToRead > tmp) {
            tmp += socket_.read_some(boost::asio::buffer(bytes + tmp, bytesToRead - tmp), error);
        }
        if (error)
            throw boost::system::system_error(error);
    } catch (std::exception &e) {
        std::cerr << "recv failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::sendBytes(const char bytes[], int bytesToWrite) {
    int tmp = 0;
    boost::system::error_code error;
    try {
        while (!error && bytesToWrite > tmp) {
            tmp += socket_.write_some(boost::asio::buffer(bytes + tmp, bytesToWrite - tmp), error);
        }
        if (error)
            throw boost::system::system_error(error);
    } catch (std::exception &e) {
        std::cerr << "recv failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::getLine(std::string &line) {
    return getFrameAscii(line, '\n');
}

bool ConnectionHandler::sendLine(std::string &line) {
    //  char* encodedData= (encdec.encode(line)).data();
    return sendFrameAscii(line, '\n');
}

bool ConnectionHandler::getFrameAscii(std::string &frame, char delimiter) {
    char ch;
    // Stop when we encounter the null character.
    // Notice that the null character is not appended to the frame string.
    try {
        do {
            getBytes(&ch, 1);
            frame.append(1, ch);
        } while (delimiter != ch);
    } catch (std::exception &e) {
        std::cerr << "recv failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::sendFrameAscii(const std::string &frame, char delimiter) {
    bool result = sendBytes(frame.c_str(), frame.length());
    if (!result) return false;
    return sendBytes(&delimiter, 1);
}

// Close down the connection properly.
void ConnectionHandler::close() {
    try {
        socket_.close();
    } catch (...) {
        std::cout << "closing failed: connection already closed" << std::endl;
    }
}

bool ConnectionHandler::sendPacket(Packet *packet) {
    this->lastPacketSent = packet->getOpCode();
    char encodeArray[1 << 10];
    encdec.encode(packet, encodeArray);
    if (packet->getOpCode() == 7) {
        return sendBytes(encodeArray, ((LOGRQ *) packet)->getLength());
    } else if (packet->getOpCode() == 5) {
        return sendBytes(encodeArray, ((ERROR *) packet)->getLength());
    } else if (packet->getOpCode() == 4) {
        return sendBytes(encodeArray, ((ACK *) packet)->getLength());
    } else if (packet->getOpCode() == 10) {
        return sendBytes(encodeArray, ((DISC *) packet)->getLength());
    }
    return false;

}

void ConnectionHandler::run() {
    while (!ifShouledTerminate()) {
        char c=' ';
        short opCode = opCodeSender();
        Packet *packet = getline(c, opCode);
        if (packet != nullptr) {
            Packet *response = process(*packet);
            delete packet;
            if (response != nullptr) {
                sendPacket(response);
                delete response;
            }
        } else
            throw;
    }
}

Packet *ConnectionHandler::process(Packet &packet) {
    if (packet.getOpCode() == 4) {//ACK
        ACK &ackPacket = (ACK &) (packet);//TODO:check if the cast works good or we need to use dynamic_cast
        cout << "ACK " << ackPacket.getBlock() << endl;
        if(lastPacketSent == 10)
            shouldTerminate = true;
        return nullptr;
    } else if (packet.getOpCode() == 5) {//ERROR
        cout << "Error " << ((ERROR &) packet).getErrorCode() << endl;
    }
    return nullptr;
}

Packet *ConnectionHandler::getline(char c, short opCode) {
    //TODO:enter first lines in run
    string line;
    bool found = false;
    if ((opCode == 4) && !found) {
        getBytes(&c, 1);
        line.append(1, c);
        getBytes(&c, 1);
        line.append(1, c);
        return new ACK(convertor.bytesToShort((char *) line.c_str()));
    } else if ((opCode == 5) && !found) {
        for (int i = 0; i < 3; i++) {
            getBytes(&c, 1);
            line.append(1, c);
            getBytes(&c, 1);
            line.append(1, c);
        }
        while (c != '\0') {
            getBytes(&c, 1);
            line.append(1, c);
        }
        char *errorCode = (char *) line.substr(0, 2).c_str();
        return new ERROR(convertor.bytesToShort(errorCode));
    }
    return nullptr;
}

bool ConnectionHandler::ifShouledTerminate() {
    return shouldTerminate;
}

short ConnectionHandler::opCodeSender() {
    char c;
    string line;
    short opCode;
    try {
        getBytes(&c, 1);
        line.append(1, c);
        getBytes(&c, 1);
        line.append(1, c);
    }
    catch (exception exp) {
        cout << exp.what() << endl;
    }
    opCode = convertor.bytesToShort((char *) line.c_str());
    return opCode;
}