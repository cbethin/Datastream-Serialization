/*
    AUTHOR: Charles Bethin. 2018. 

    -- CONCEPTS --
    The premise of this library is that the need to serialize simple structured data into a standardized binary form can be 
    reduced by considering your structured data as a series of "fields" to be sent across a socket. These fields are simply
    strings, a datatype which most all datatypes can be converted to. 

    The fields are grouped into datastream objects, a doubly-linked list of fields (each field containing the data itself and the datasize).

    The library contains two datatstreams: the output datastream (simply called "datastream") and the receiving datastream
    (called "instream"). To send a message across a socket, first add text fields to a datastream using the add_field_to_datastream(struct datastream)
    function. As many fields as needed can be added (though modification of the macros in datastream.h may need to be modified). When
    ready to send, simply call the send_and_clear_datastream(char *) function to send the datastream to a passed address. 

    When receiving messages (which in this code only occurs as a response to messages sent out) are collected into an instream datatstream object.
    This is again a doubly linked list of fields which can be easily walked through to retreive received data. 

    -- OUTPUT STREAM --
    The output stream is formated as an alternating series ABCBC... format. For every field C, a 4-byte array B is created to indicate the size of the field
    (encoded in Big Endian byte order). The fields and their size arrays are then ordered in the BCBC... format. When the list of fields is converted
    into this format, a third 4-byte array A is created which dictates the size of the entire datastream (in number of fields), and is appended to the front of the buffer 
    that will be sent over the socket.

    -- INPUT BUFFER -- 
    When extracting on the receiving end, an application can extract a list of fields by first extracting the size of the entire datastream (in number of fields) contained in
    the first four bytes. Then, for each field until the size of the datastream is covered, the application can extract the size of the field, and, using
    that field size, extract the corresponding amount of bytes from the input buffer.
*/

#include "datastream.h"

// DATASTREAM OBJECT OPERATORS

// Prints a given datastream object
void print_datastream_object(struct datastream *d) {
    struct field *p = d->firstField;

    while (p != NULL) {
        for (int j = 0; j < p->size; j++) {
            printf("%c", p->data[j]);
        }
        printf(" - ");

        p = p->next;
    }

    printf("\n");
}

// Adds field to a given datastream object d, taking the field as a character array (including the size of the char array)
void add_field_to_datastream_object(char *text, int size_text, struct datastream *d) {
    struct field *newField = malloc(sizeof(struct field));
    newField->size = size_text;
    newField->next = NULL;
    newField->prior = NULL;

    // Allocate memory for data and give it to the field
    char *dataPtr = malloc(size_text);
    newField->data = dataPtr;

    // Copy text into data field;
    for (int i = 0; i < size_text; i++) {
        dataPtr[i] = text[i];
    }

    // Add field to end of datastream
    if (d->firstField == NULL) {
        d->firstField = newField;
        d->lastField = newField;
    } else {
        newField->prior = d->lastField;
        d->lastField->next = newField;
        d->lastField = newField;
    }

    // printf("Added field: %p -- %p\n", ds->lastField, ds->lastField->next);
    d->n_fields++;
}

// Clears a datastream object
void clear_datastream_object(struct datastream *d) {
    struct field *p = d->lastField->prior;
    
    while(p != NULL) {
        free(p->next->data);
        p = p->prior;
    }

    d->firstField = NULL;
    d->lastField = NULL;
    d->n_fields = 0;
}

// Removes the first field of a datastream object. Will make the second field the new first field
void remove_first_field(struct datastream *d) {
    struct field *tmp = d->firstField;

    if (d->firstField == NULL) {
        return;
    } else if (d->firstField->next == NULL) {
        d->firstField = NULL;
        d->lastField = NULL;
        free(tmp->data);
        free(tmp);
        return; 
    }

    d->firstField = d->firstField->next;
    free(tmp->data);
    free(tmp);
}

// Converts datastream into a byte array encoded with format ABAB... where
// A is a 4 bye array giving the size of a field, which is contained in B
void datastream_to_byte_array(unsigned char *buf, int size_buf) { // feed it a max buf size
    int n = 0; // master index
    struct field *p = ds->firstField; // just a pointer to the first field

    //Convert n_fields into byte array
    unsigned char intbuf[INT_BYTE_SIZE];
    memset(buf, 0, sizeof(intbuf));
    int_to_fourbyte_array(ds->n_fields, (unsigned char *) intbuf);
    for (int i = 0; i < INT_BYTE_SIZE; i++, n++) {
        buf[n] = intbuf[i];
    }

    while (p != NULL) {
        //Convert size into byte array
        unsigned char intbuf[INT_BYTE_SIZE];
        memset(intbuf, 0, sizeof(intbuf));
        int_to_fourbyte_array(p->size, (unsigned char *) intbuf);

        // Copy size byte array into buffer
        for (int i = 0; i < INT_BYTE_SIZE; i++, n++) {
            buf[n] = intbuf[i];
        }

        // Copy p's field into buffer
        for (int i = 0; i < p->size; i++, n++) {
            buf[n] = p->data[i];
        }

        // Move onto next field 
        p = p->next;
    }
}

// Converts a byte array in the format mentioned above to a datastream
// Is the inverse of datastream_to_byte_array()
void byte_array_to_datastream(char *buf, int size_buf, struct datastream *d) {
    int n = 0;

    // printf("DATA: \n");
    // for (int i = 0; i < size_buf; i++) {
    //     printf("%x-", buf[i]);
    // }
    // printf("\n");

    unsigned char n_fields_slice[INT_BYTE_SIZE];
    for (int j = 0; j < INT_BYTE_SIZE; j++, n++) {
        n_fields_slice[j] = buf[n];
    }
    int n_fields = char_array_to_int(n_fields_slice);

    // If n_fields is > MAX_PACKETS then something wrong, just break out and ignore this packet
    if (n_fields > MAX_PACKETS) {
        return;
    }

    for (int i = 0; i < n_fields; i++) {

        // Extract size of field 
        unsigned char buf_slice[INT_BYTE_SIZE];
        for (int j = 0; j < INT_BYTE_SIZE; j++, n++) {
            buf_slice[j] = buf[n];
        }

        int field_size = char_array_to_int(buf_slice);

        // If field size is empty, we finished, return
        if (field_size == 0 || field_size > MAX_FIELD_SIZE) {
            return;
        }

        // Extract data into array
        char *data = malloc(field_size+1);
        for (int j = 0; j < field_size; j++, n++) {
            data[j] = buf[n];
        }
        memset(&data[field_size], 0, 1); // make sure it's a null-terminated string

        add_field_to_datastream_object(data, field_size+1, d);
    }
}

// GLOBAL DATSTREAM FUNCTIONS 

// Initializes the output and input datastreams
void initialize_datastream() {
    ds = malloc(sizeof(struct datastream));
    ds->firstField = NULL;
    ds->lastField = NULL;
    ds->n_fields = 0;

    instream = malloc(sizeof(struct datastream));
    instream->firstField = NULL;
    instream->lastField = NULL;
    instream->n_fields = 0;
}

// Prints the output datastream
void print_datastream() {
    print_datastream_object(ds);
}

// Adds a field to the output datastream. Exists so that a programmer using this library can add fields without needing to directly call
// upon the output datastream object 
void add_field_to_datastream(char *text, int size_text) {
    add_field_to_datastream_object(text, size_text, ds);
}

// Clears the output datastream.
void clear_datastream() {
    clear_datastream_object(ds);
}

// SENDING DATASTREAMS
void send_and_clear_datastream(char *dest_addr) {
    int network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Specify address for the socket to connect to;
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr(dest_addr);

    // Establish connection to server
    if (connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        printf("Error connecting.\n");
        sleep(1);
        send_and_clear_datastream(dest_addr);
        return;
    }

    // Convert datastream to our output buffer
    int outbuf_size = MAX_PACKET_SIZE;
    unsigned char *outbuf = malloc(outbuf_size);
    datastream_to_byte_array(outbuf, outbuf_size); // We pass copies of the object just in case

    if (send(network_socket, outbuf, outbuf_size, 0) < 0) {
        printf("Error: %s\n", strerror(errno));
    }
    
    // receive
    int bytes_received = 0;
    int len = 0, maxlen = MAX_PACKET_SIZE;
    char buffer[MAX_PACKET_SIZE];
    char *pbuffer = buffer;
        
    while ((bytes_received = recv(network_socket, pbuffer, MAX_PACKET_SIZE, 0)) > 0) {
        pbuffer += bytes_received;
        maxlen -= bytes_received;
        len += bytes_received;

        buffer[len] = '\0';
        // printf("Response: %c\n", buffer[8]);
        byte_array_to_datastream(buffer, len, instream);
        handle_response(instream);
    }

    close(network_socket);
    clear_datastream();
    free(outbuf);
}

// HELPER FUNCTIONS

// Converts an integer to a byte array (of size INT_BYTE_SIZE) representation of the integer in binary form, using Big-Endian Byte order
void int_to_fourbyte_array(int n, unsigned char *buf) {
    // converts an integer into a 4 byte array
    uint32_t value = (uint32_t) n;

    for (int i = 0; i < INT_BYTE_SIZE; i++) {
        buf[i] = (value >> (8 * (INT_BYTE_SIZE-1-i))) & 0xff;
    };
}

// Converts a char array of INT_BYTE_SIZE, assumed to be a binary representaiton of an integer in binary, Big-Endian form, into an integer
int char_array_to_int(unsigned char *buf) {
    unsigned char *string_buf = malloc(INT_BYTE_SIZE);
    for (int i = 0; i < INT_BYTE_SIZE; i++) {
        sprintf((char *) string_buf+(2*i), "%02x", buf[i]);
    }

    uint32_t i = (uint32_t) strtol((char *) string_buf, NULL, 16);

    free(string_buf);
    return i;
}