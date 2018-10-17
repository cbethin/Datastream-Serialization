/*
    AUTHOR: Charles Bethin. 2018. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#ifndef DATASTREAM_H__
#define DATASTREAM_H__

struct datastream { // List of fields
    struct field *firstField;
    struct field *lastField;
    int n_fields;
};

struct field { // List of characters
    char *data;
    struct field *next;
    struct field *prior;
    int32_t size;
};

struct dataObj {
    char *endOfDataStream;
    char *dataStream;
};

// These constants can be changed for specific build needs 
#define PORT 9000
#define INT_BYTE_SIZE 4
#define MAX_PACKET_SIZE 8000
#define MAX_FIELD_SIZE 200
#define INIT_N_FIELDS 5
#define MAX_PACKETS 1000

struct datastream *ds; 
struct datastream *instream;

// Datastream Object Operators
void print_datastream_object(struct datastream *d);
void add_field_to_datastream_object(char *text, int size_text, struct datastream *d);
void clear_datastream_object(struct datastream *d);
void remove_first_field(struct datastream *d);
void datastream_to_byte_array(unsigned char *buf, int size_buf);
void byte_array_to_datastream(char *buf, int size_buf, struct datastream *d);

// Global datastream functions
void initialize_datastream();
void print_datastream();
void add_field_to_datastream(char *text, int size_text);
void clear_datastream();

// Sending datastream
void send_and_clear_datastream(char *dest_addr);
void handle_response(struct datastream *instream);

// Helper functions
void int_to_fourbyte_array(int n, unsigned char *buf);
int char_array_to_int(unsigned char *buf);

#endif /* DATASTREAM_H__ */