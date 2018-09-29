# Datastream-Serialization
A C library written to help reduce the need to serialize data before sending over a socket connection 
 
### Concepts & Use

The premise of this library is that the need to serialize simple structured data into a standardized binary form can be
reduced by considering your structured data as a series of "fields" to be sent across a socket. These fields are simply
strings, a datatype which most all datatypes can be converted to. 

The fields are grouped into datastream objects, a doubly-linked list of fields (each field containing the data itself and the datasize).

The library contains two datatstreams: the output datastream (simply called "datastream") and the receiving datastream (called "instream"). To send a message across a socket, first add text fields to a datastream using the add_field_to_datastream(struct datastream) function. As many fields as needed can be added (though modification of the macros in datastream.h may need to be modified). When ready to send, simply call the send_and_clear_datastream(char *) function to send the datastream to a passed address. 

When receiving messages (which in this code only occurs as a response to messages sent out) are collected into an instream datatstream object. This is again a doubly linked list of fields which can be easily walked through to retreive received data. 

#### Output Datastream Use

The output stream is formated as an alternating series ABCBC... format. For every field C, a 4-byte array B is created to indicate the size of the field (encoded in Big Endian byte order). The fields and their size arrays are then ordered in the BCBC... format. When the list of fields is converted into this format, a third 4-byte array A is created which dictates the size of the entire datastream, and is appended to the front of the buffer that will be sent over the socket.

#### Input Datastream Use

When extracting on the receiving end, an application can extract a list of fields by first extracting the size of the entire datastream contained in the first four bytes. Then, for each field until the size of the datastream is covered, the application can extract the size of the field, and, using that field size, extract the corresponding amount of bytes from the input buffer.
