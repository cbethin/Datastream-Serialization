/*
    Datastream stuff here
*/

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <functional>
#include <cerrno>
// #include <stdlib.h>

using Field = std::string;
using u_char = unsigned char;
const int MAX_PACKET_SIZE = 1500;


/* A buffer is a wrapper class for an unsigned char vector.
It allows for easier conversions of strings and integers into byte arrays,
 producing the byte arrays only when called upon. */
class Buffer {
private:
    std::vector<unsigned char> buf;
    const static int intByteArraySize = 4;
    unsigned char *charBytes = nullptr;
    int charBytesSize = 0;

    // Adds integer to the buffer in BigEndian binary format
    void addIntToByteArray(int n) {
        for (int i = 0; i < intByteArraySize; i++) {
            buf.push_back((unsigned char) ((uint32_t(n) >> (8 * (intByteArraySize-1-i))) & 0xff));
        }
    }
    
    // Add's the length of field and the field itself to the buffer
    void addFieldToByteArray(const Field& field) {
        addIntToByteArray(field.length());

        for(int i = 0; i < field.length(); i++) {
            buf.push_back(field[i]);
        }
    }

    // Converts a byte array in bigEndian binary into an integer
    static int byteArrayToInt(const unsigned char *c) {
        unsigned char *stringBuf = new unsigned char[intByteArraySize];
        for (int i = 0; i < intByteArraySize; i++) {
            sprintf((char *) stringBuf+(2*i), "%02x", c[i]);
        }

        int i = (uint32_t) strtol((char *) stringBuf, NULL, 16);
        delete[] stringBuf;
        return i;
    }

public:
    Buffer(std::vector<Field> &fields) {  
        for(std::vector<Field>::iterator i = fields.begin(); i != fields.end(); i++) {
            addFieldToByteArray(*i);
        }
    }
    Buffer(int n) {
        addIntToByteArray(n);
    }
    Buffer(const Field& f) {
        addFieldToByteArray(f);
    }
    Buffer(unsigned char *c) { // Convert from byte array to Buffer object
        deseralizeFields(c, std::bind(&Buffer::addFieldToByteArray, this, std::placeholders::_1));
    }

    ~Buffer() {
        if (charBytes != NULL) {
            delete[] charBytes;
        }
    }

    static void deseralizeFields(unsigned char* c, std::function<void(Field)> fieldsHandler) {
        int totalSize = byteArrayToInt(c);

        for (int i = intByteArraySize; i < intByteArraySize+totalSize; i++) {
            int fieldSize = byteArrayToInt(c+i);
            i += intByteArraySize;

            std::string field = "";
            for (int j = 0; j < fieldSize; j++, i++) { // don't inlcude the null terminator
                field += c[i];
            }

            // addFieldToByteArray(field);
            fieldsHandler(field);
        }
    }

    int size() { 
        return charBytesSize; 
    }

    unsigned char* bytes() {
        if (charBytes != nullptr) {
            delete[] charBytes;
        }

        int n = buf.size(); // total size of buffer 
        charBytesSize = n+intByteArraySize;

        Buffer totalSizeBuf(n);
        totalSizeBuf.append(*this);

        charBytes = new unsigned char[charBytesSize];
        int j = 0;
        for(std::vector<unsigned char>::iterator i = totalSizeBuf.buf.begin(); i != totalSizeBuf.buf.end(); i++) {
            charBytes[j] = *i;
            j++;
        }
        

        return charBytes;
    }

    void append(const Buffer& b) {
        for (auto i = b.buf.begin(); i != b.buf.end(); i++) {
            buf.push_back(*i);
        }
    }

    friend std::ostream& operator <<(std::ostream& s, Buffer b) {
        for (auto i = b.buf.begin(); i != b.buf.end(); i++) {
            s << std::hex << int(*i) << "-";
        }
        return s;
    }
};

/* A list of "Fields" (strings) that will can be easily sent out 
   You add to the datastream using "pushField(Field)", and send to
   a server using sendTo(string, int). It can be cleared using clear() */
class Datastream {
private:
    std::vector<Field> data;
    unsigned char *byteArray;

public:
    Datastream() { }

    Datastream(unsigned char *c) {
        Buffer::deseralizeFields(c, std::bind(&Datastream::pushField, this, std::placeholders::_1));
    }

    ~Datastream() {
        // std::cout << "Deconstructing " << data[0] << "\n";
    }

    Datastream sendTo(const std::string& address, int port) {
        int network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Datastream response;

        // Specify address for the socket to connect to;
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = inet_addr(address.c_str());

        // Establish connection to server
        if (connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
            std:: cout << "Error connecting.\n";
            sleep(1);
            sendTo(address, port);
            return response;
        }


        Buffer outbuf(this->data);
        if (send(network_socket, outbuf.bytes(), outbuf.size(), 0) < 0) {
            std::cout << "Error sending data.\n";
        }
        
        // receive
        int bytes_received = 0;
        int len = 0, maxlen = MAX_PACKET_SIZE;
        unsigned char *buffer = new unsigned char[MAX_PACKET_SIZE];
        unsigned char *pbuffer = buffer;
            
        while ((bytes_received = recv(network_socket, pbuffer, MAX_PACKET_SIZE, 0)) > 0) {
            pbuffer += bytes_received;
            maxlen -= bytes_received;
            len += bytes_received;

            buffer[len] = '\0';

            response = Datastream(buffer);
            close(network_socket);
            delete[] buffer;

            return response;
        }

        return response;
        // Maybe clear datastream?
    }

    void pushField(Field field) {
        data.push_back(field);
    }

    void pushNamedField(Field fieldName, Field fieldValue) {
        pushField(fieldName);
        pushField(fieldValue);
    }

    Field popField() {
        Field lastField = data.back();
        data.pop_back();
        return lastField;
    }

    void clear() {
        data.clear();
        byteArray = nullptr;
    }

    friend std::ostream& operator <<(std::ostream& s, const Datastream& d) {
        if (d.data.size() == 0) 
            return s << "Empty";

        for (auto i = d.data.begin(); i != d.data.end(); i++) {
            s << *i << " ";
        }
        return s;
    }
};
