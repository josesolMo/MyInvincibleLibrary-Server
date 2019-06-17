//
// Created by jose on 08/06/19.
//
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <json-c/json.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <chrono>

#include <thread>
#include <string>


#include "ServerController.h"

#define PORT 3550
#define BACKLOG 4
#define MAXDATASIZE 1000

using namespace std;

static ServerController* serverController;

static string JSONReturnGen;
static json_object *JSONReturnConsole;


///Creacion del JSON por enviar
json_object *jObj = json_object_new_object();


/**
 *
 * @param jObj
 * @param destinatary
 * @return
 */
int sendJSON(json_object *jObj, string destinatary) {

    ///Guarda el ip que se utilizara para la comunicacion
    string ipAddress;

    if (destinatary == "RAID") {
        ipAddress = "192.168.100.4";
    } else if (destinatary == "MetadataDB") {
        ipAddress = "192.168.100.2";
    }


    char* str;
    int fd, numbytes;
    struct sockaddr_in client;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    char sendBuff[MAXDATASIZE];
    char recvBuff[MAXDATASIZE];

    struct hostent *he;

    if (fd < 0)
    {
        printf("Error : Could not create socket\n");
        return 1;
    }
    else
    {
        client.sin_family = AF_INET;
        client.sin_port = htons(PORT);
        ///Toma el ipAddress dependiendo del destinatary
        client.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        memset(client.sin_zero, '\0', sizeof(client.sin_zero));
    }

    if (::connect(fd, (const struct sockaddr *)&client, sizeof(client)) < 0)
    {
        printf("ERROR connecting to server\n");
        return 1;
    }


    ///Se agrega el objeto JSON estatico


    if (strcpy(sendBuff, json_object_to_json_string(jObj)) == NULL) {
        printf("ERROR strcpy()");
        exit(-1);
    }

    if (write(fd, sendBuff, strlen(sendBuff)) == -1)
    {
        printf("ERROR write()");
        exit(-1);
    }

    if ((numbytes=recv(fd,recvBuff,MAXDATASIZE,0)) < 0){

        printf("Error en recv() \n");
        exit(-1);
    }


    ///Manejo de KEYS


    ///KEY: NEWGALLERY
    struct json_object *tempNewGallery;
    json_object *parsed_jsonNewGallery = json_tokener_parse(recvBuff);
    json_object_object_get_ex(parsed_jsonNewGallery, "NEWGALLERY", &tempNewGallery);
    if (json_object_get_string(tempNewGallery) != nullptr){
        JSONReturnGen = json_object_get_string(tempNewGallery);
    }

    ///KEY: NEWIMAGE
    struct json_object *tempNewImage;
    json_object *parsed_jsonNewImage = json_tokener_parse(recvBuff);
    json_object_object_get_ex(parsed_jsonNewImage, "NEWIMAGE", &tempNewImage);
    if (json_object_get_string(tempNewImage) != nullptr){
        JSONReturnGen = json_object_get_string(tempNewImage);
    }

    ///KEY: CONSOLE
    struct json_object *tempConsole;
    json_object *parsed_jsonNewConsole = json_tokener_parse(recvBuff);
    json_object_object_get_ex(parsed_jsonNewConsole, "CONSOLE", &tempConsole);
    if (json_object_get_string(tempConsole) != nullptr){
        //JSONReturnGen = json_object_get_string(tempConsole);
        JSONReturnConsole = tempConsole;
    }

    ///KEY: INITIAL
    struct json_object *tempInitial;
    json_object *parsed_jsonInitial = json_tokener_parse(recvBuff);
    json_object_object_get_ex(parsed_jsonInitial, "INITIAL", &tempInitial);
    if (json_object_get_string(tempInitial) != 0) {
        ///Variable por guardar o funcion por llamar
        cout << "Initial Galleries" << endl;
        JSONReturnConsole = tempInitial;
    }



/*
    ///KEY:
    struct json_object *temp;
    json_object *parsed_json = json_tokener_parse(recvBuff);
    json_object_object_get_ex(parsed_json, "KEY", &temp);
    if (json_object_get_string(temp) != 0) {
        ///Variable por guardar o funcion por llamar
        JSONReturnGen = json_object_get_string(temp);
    }
*/



    ///Se limpian los Buffers
    memset(recvBuff, 0, MAXDATASIZE);
    memset(sendBuff, 0, MAXDATASIZE);

    ::close(fd);

}



/**
 * Envia un JSON a la base de datos para verificar la disponibilidad de el nombre de la galeria entrante.
 * @param newGallery
 * @param newGalleryKey
 * @return JSON
 */
string sendNewGallery(string newGallery, string newGalleryKey) {

    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();

    ///Toma la data que le entra como parametro en la funcion
    json_object *jstringNewGallery = json_object_new_string(newGallery.c_str());
    ///Toma el key que le entra como parametro en la funcion
    json_object_object_add(jObj,newGalleryKey.c_str(), jstringNewGallery);

    sendJSON(jObj, "MetadataDB");

    ///Preparacion del JSON con el resultado de la disponibilidad
    ///del nombre de la imagen por agregar.

    if (JSONReturnGen == "1" || JSONReturnGen == "0") {
        cout << "JSONReturnGen: " << JSONReturnGen << endl;
        json_object *jobjSendNewGallery = json_object_new_object();
        json_object *jstringSendNewGallery = json_object_new_string(
                JSONReturnGen.c_str()); /// "1" si se agrega o "0" si no
        json_object_object_add(jobjSendNewGallery, "NEWGALLERY", jstringSendNewGallery);
        return json_object_to_json_string(jobjSendNewGallery);
    }
    else {
        json_object *jobjError = json_object_new_object();
        json_object *jstringError = json_object_new_string("ERROR: NEWGALLERY");
        json_object_object_add(jobjError, "NEWGALLERY", jstringError);
        return json_object_to_json_string(jobjError);

    }

}

/**
 * Envia un JSON a la base de datos para verificar la disponibilidad de el nombre de la imagen entrante.
 * @param newImage
 * @param newImageKey
 * @param gallery
 * @param galleryKey
 * @param binaryData
 * @param binaryDataKey
 * @return JSON
 */
string sendNewImage(string newImage,  string newImageKey, string gallery, string galleryKey,
        string binaryData, string binaryDataKey) {

    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();

    ///Toma el nombre de la nueva imagen
    json_object *jstringNewImage = json_object_new_string(newImage.c_str());
    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,newImageKey.c_str(), jstringNewImage);

    ///Toma el nombre del gallery donde se guardara la imagen
    json_object *jstringGallery = json_object_new_string(gallery.c_str());
    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,galleryKey.c_str(), jstringGallery);

    sendJSON(jObj, "MetadataDB");


    ///Se crea el objeto JSON que se devolvera al cliente
    json_object *jobjSendNewImage = json_object_new_object();


    if (JSONReturnGen == "1" || JSONReturnGen == "0") {

        if (JSONReturnGen == "1") {
            //sendJSON(jObj, "RAID");
        }

        json_object *jstringSendNewImage = json_object_new_string(JSONReturnGen.c_str()); /// "1" o "0"
        json_object_object_add(jobjSendNewImage, "NEWIMAGE", jstringSendNewImage);
        return json_object_to_json_string(jObj);

    } else {

        json_object *jobjError = json_object_new_object();
        json_object *jstringError = json_object_new_string("ERROR: NEWIMAGE");
        json_object_object_add(jobjError, "NEWIMAGE", jstringError);
        return json_object_to_json_string(jobjError);

    }
}

/**
 * Evia el texto que se escribio en la consola a la clase SQLController
 * @param console
 * @param consoleKey
 * @return string
 */
string sendConsole(string console, string consoleKey) {

    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();

    ///Toma el nombre de la nueva imagen
    json_object *jstringConsole = json_object_new_string(console.c_str());
    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,consoleKey.c_str(), jstringConsole);

    ///Se manda el texto de la consola en el cliente al MetadataDB
    sendJSON(jObj, "MetadataDB");

    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();


    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,consoleKey.c_str(), JSONReturnConsole);


    cout << "SENDING: " << json_object_to_json_string(jObj) << endl;


    return json_object_to_json_string(jObj);

}

/**
 * Obtiene la lista inicial de galerias e imagenes para iniciar el cliente
 * @param initial
 * @param initialKey
 * @return JSON
 */
string sendInitial(string initial, string initialKey) {

    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();

    ///Toma el nombre de la nueva imagen
    json_object *jstringConsole = json_object_new_string(initial.c_str());
    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,initialKey.c_str(), jstringConsole);

    ///Se manda el texto de la consola en el cliente al MetadataDB
    sendJSON(jObj, "MetadataDB");

    ///Toma el key de la nueva imagen
    json_object_object_add(jObj,initialKey.c_str(), JSONReturnConsole);

    return json_object_to_json_string(jObj);

}


////////////////////////////////////////////////////////////////////////////////////////////////

string sendTest() {

    ///KEY para probar la graficacion de cliente
    string keyForTest = "CONSOLE";


    ///Redefine el jObject para eliminar sus keys antiguos
    jObj = json_object_new_object();


    ///Instancia de jarray para pruebas General
    json_object *jarrayTestGeneral = json_object_new_array();


    ///Instancia de jarray para pruebas 0
    json_object *jarrayTest0 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 0
    json_object *jstringTestColumna0 = json_object_new_string("FILE_ID");
    json_object *jstringTest01 = json_object_new_string("viaje04.bmp");
    json_object *jstringTest02 = json_object_new_string("playa.bmp");
    json_object *jstringTest03 = json_object_new_string("gato.bmp");
    json_object *jstringTest04 = json_object_new_string("reloj.bmp");
    ///Para agregar al arrayTest0
    json_object_array_add(jarrayTest0, jstringTestColumna0);
    json_object_array_add(jarrayTest0, jstringTest01);
    json_object_array_add(jarrayTest0, jstringTest02);
    json_object_array_add(jarrayTest0, jstringTest03);
    json_object_array_add(jarrayTest0, jstringTest04);


    ///Instancia de jarray para pruebas 1
    json_object *jarrayTest1 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 1
    json_object *jstringTestColumna1 = json_object_new_string("NOMBRE");
    json_object *jstringTest11 = json_object_new_string("Imagen1");
    json_object *jstringTest12 = json_object_new_string("Imagen2");
    json_object *jstringTest13 = json_object_new_string("Imagen3");
    json_object *jstringTest14 = json_object_new_string("Imagen4");
    ///Para agregar al arrayTest1
    json_object_array_add(jarrayTest1, jstringTestColumna1);
    json_object_array_add(jarrayTest1, jstringTest11);
    json_object_array_add(jarrayTest1, jstringTest12);
    json_object_array_add(jarrayTest1, jstringTest13);
    json_object_array_add(jarrayTest1, jstringTest14);


    ///Instancia de jarray para pruebas 2
    json_object *jarrayTest2 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 2
    json_object *jstringTestColumna2 = json_object_new_string("AUTOR");
    json_object *jstringTest21 = json_object_new_string("Andrey");
    json_object *jstringTest22 = json_object_new_string("Edgar");
    json_object *jstringTest23 = json_object_new_string("José");
    json_object *jstringTest24 = json_object_new_string("Ruben");
    ///Para agregar al arrayTest2
    json_object_array_add(jarrayTest2, jstringTestColumna2);
    json_object_array_add(jarrayTest2, jstringTest21);
    json_object_array_add(jarrayTest2, jstringTest22);
    json_object_array_add(jarrayTest2, jstringTest23);
    json_object_array_add(jarrayTest2, jstringTest24);


    ///Instancia de jarray para pruebas 3
    json_object *jarrayTest3 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 3
    json_object *jstringTestColumna3 = json_object_new_string("CREACION");
    json_object *jstringTest31 = json_object_new_string("2004");
    json_object *jstringTest32 = json_object_new_string("2013");
    json_object *jstringTest33 = json_object_new_string("1994");
    json_object *jstringTest34 = json_object_new_string("2019");
    ///Para agregar al arrayTest3
    json_object_array_add(jarrayTest3, jstringTestColumna3);
    json_object_array_add(jarrayTest3, jstringTest31);
    json_object_array_add(jarrayTest3, jstringTest32);
    json_object_array_add(jarrayTest3, jstringTest33);
    json_object_array_add(jarrayTest3, jstringTest34);


    ///Instancia de jarray para pruebas 4
    json_object *jarrayTest4 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 4
    json_object *jstringTestColumna4 = json_object_new_string("TAMANO");
    json_object *jstringTest41 = json_object_new_string("1200x800");
    json_object *jstringTest42 = json_object_new_string("250x250");
    json_object *jstringTest43 = json_object_new_string("600x600");
    json_object *jstringTest44 = json_object_new_string("4200x3600");
    ///Para agregar al arrayTest4
    json_object_array_add(jarrayTest4, jstringTestColumna4);
    json_object_array_add(jarrayTest4, jstringTest41);
    json_object_array_add(jarrayTest4, jstringTest42);
    json_object_array_add(jarrayTest4, jstringTest43);
    json_object_array_add(jarrayTest4, jstringTest44);


    ///Instancia de jarray para pruebas 5
    json_object *jarrayTest5 = json_object_new_array();
    ///Crea el titulo y la informacion de la columna 5
    json_object *jstringTestColumna5 = json_object_new_string("DESCRIPCION");
    json_object *jstringTest51 = json_object_new_string("Edgar");
    json_object *jstringTest52 = json_object_new_string("Es");
    json_object *jstringTest53 = json_object_new_string("Muy");
    json_object *jstringTest54 = json_object_new_string("Cabezon");
    ///Para agregar al arrayTest5
    json_object_array_add(jarrayTest5, jstringTestColumna5);
    json_object_array_add(jarrayTest5, jstringTest51);
    json_object_array_add(jarrayTest5, jstringTest52);
    json_object_array_add(jarrayTest5, jstringTest53);
    json_object_array_add(jarrayTest5, jstringTest54);



    ///Para agregar los arrayTest's al arrayTestGeneral
    json_object_array_add(jarrayTestGeneral, jarrayTest0);
    json_object_array_add(jarrayTestGeneral, jarrayTest1);
    json_object_array_add(jarrayTestGeneral, jarrayTest2);
    json_object_array_add(jarrayTestGeneral, jarrayTest3);
    json_object_array_add(jarrayTestGeneral, jarrayTest4);
    json_object_array_add(jarrayTestGeneral, jarrayTest5);


    ///Se agrega al Obj para enviarse
    json_object_object_add(jObj, keyForTest.c_str(), jarrayTestGeneral);

    //cout << json_object_to_json_string(jObj) << endl;

    return json_object_to_json_string(jObj);

}


////////////////////////////////////////////////////////////////////////////////////////////////



/**
 * Corre el servidor.
 * @return int
 */
int runServer() {

    int fd, fd2;

    struct sockaddr_in server;
    struct sockaddr_in client;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("error en socket()\n");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    bzero(&(server.sin_zero), 8);

    if (bind(fd, (struct sockaddr *) &server,
             sizeof(struct sockaddr)) == -1) {
        printf("error en bind() \n");
        exit(-1);
    }

    if (listen(fd, BACKLOG) == -1) {
        printf("error en listen()\n");
        exit(-1);
    }

    printf("\nServidor 'My Invincible Library - Server' abierto.\n");

    while (true) {

        unsigned int address_size = sizeof(client);

        if ((fd2 = accept(fd, (struct sockaddr *) &client,
                          &address_size)) == -1) {
            printf("error en accept()\n");
            exit(-1);
        }

        printf("\nSe obtuvo una conexión de un cliente.\n");

        ssize_t r;

        char buff[MAXDATASIZE];

        for (;;) {
            r = read(fd2, buff, MAXDATASIZE);

            if (r == -1) {
                perror("read");
                return EXIT_FAILURE;
            }
            if (r == 0)
                break;
            printf("\nREAD: %s\n", buff);


            ///JSON Reads


            ///KEY: NEWGALLERY
            ///Obtiene el nombre de la nueva galeria para verificar si puede ser guardada.
            struct json_object *tempNewGallery;
            json_object *parsed_jsonNewGallery = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonNewGallery, "NEWGALLERY", &tempNewGallery);

            ///KEY: NEWIMAGE
            ///Obtiene el nombre de la nueva imagen para verificar si puede ser guardada.
            struct json_object *tempNewImage;
            json_object *parsed_jsonNewImage = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonNewImage, "NEWIMAGE", &tempNewImage);

            ///KEY: GALLERY
            ///Obtiene la galeria donde se cambiara o se retornara su contenido
            struct json_object *tempGallery;
            json_object *parsed_jsonGallery = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonGallery, "GALLERY", &tempGallery);

            ///KEY: BINARYDATA
            ///Obtiene un request para obtener el binaryData de la imagen por
            struct json_object *tempBinaryData;
            json_object *parsed_jsonBinaryData = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonBinaryData, "BINARYDATA", &tempBinaryData);

            ///KEY: CONSOLE
            ///Obtiene un request para enviar el texto de la consola e interpretarlo
            struct json_object *tempConsole;
            json_object *parsed_jsonConsole = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonConsole, "CONSOLE", &tempConsole);

            ///KEY: INITIAL
            ///Obtiene un request para obtener las galerias e imagenes en la base de datos
            struct json_object *tempInitial;
            json_object *parsed_jsonInitial = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonInitial, "INITIAL", &tempInitial);

            ///KEY: TEST
            ///Obtiene un request para
            struct json_object *tempTest;
            json_object *parsed_jsonTest = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonTest, "TEST", &tempTest);



            /*
            ///KEY: TEMPLATE
            ///Obtiene un request para
            struct json_object *tempZZ;
            json_object *parsed_jsonZZ = json_tokener_parse(buff);
            json_object_object_get_ex(parsed_jsonZZ, "ZZ", &tempZZ);
             */





            ///JSON Writes



            ///Obtendra un request para obtener la disponibilidad de la galeria.
            ///Verifica que reciba los KEYS: NEWGALLERY
            if (json_object_get_string(tempNewGallery) != nullptr ) {
                ///JSON saliente del servidor
                string newGallery = sendNewGallery(json_object_get_string(tempNewGallery), "NEWGALLERY");
                ///Envio al cliente
                send(fd2, newGallery.c_str(), MAXDATASIZE, 0);
            }


            ///Obtendra un request para obtener la disponibilidad de la nueva imagen.
            ///Verifica que reciba los KEYS: NEWIMAGE, GALLERY & BINARYDATA
            if (json_object_get_string(tempNewImage) != nullptr && json_object_get_string(tempGallery) != nullptr &&
                    json_object_get_string(tempBinaryData) != nullptr) {
                ///JSON saliente del servidor
                string newImage = sendNewImage(json_object_get_string(tempNewImage), "NEWIMAGE",
                        json_object_get_string(tempGallery), "GALLERY",
                        json_object_get_string(tempBinaryData), "BINARYDATA");
                ///Envio al cliente
                send(fd2, newImage.c_str(), MAXDATASIZE, 0);
            }

            ///Obtendra un request para obtener
            ///Verifica que reciba los KEYS: CONSOLE
            if (json_object_get_string(tempConsole) != nullptr ) {
                ///JSON saliente del servidor
                string sendConsoleBack = sendConsole(json_object_get_string(tempConsole), "CONSOLE");
                ///Envio al cliente
                send(fd2, sendConsoleBack.c_str(), MAXDATASIZE, 0);
            }

            ///Obtendra un request para obtener
            ///Verifica que reciba los KEYS: INITIAL
            if (json_object_get_string(tempInitial) != nullptr ) {
                ///JSON saliente del servidor
                string sendInitialData = sendInitial(json_object_get_string(tempInitial), "INITIAL");
                ///Envio al cliente
                send(fd2, sendInitialData.c_str(), MAXDATASIZE, 0);
            }

            ///Obtendra un request para obtener
            ///Verifica que reciba los KEYS: TEST
            if (json_object_get_string(tempTest) != nullptr ) {
                ///JSON saliente del servidor
                string test = sendTest();
                ///Envio al cliente
                send(fd2, test.c_str(), MAXDATASIZE, 0);
            }

            /*

            ///Obtendra un request para obtener
            ///Verifica que reciba los KEYS: TEMPLATE
            if (json_object_get_string(tempZZ) != nullptr ) {
                ///JSON saliente del servidor
                string variable = sendAType(json_object_get_string(tempZZ), "KEY");
                ///Envio al cliente
                send(fd2, variable.c_str(), MAXDATASIZE, 0);
            }

            */


        }

        ///Reestablece el buffer
        memset(buff, 0, MAXDATASIZE);

        cout <<
             "\n\n--------------------------------------------------------------------------------"
             "END--------------------------------------------------------------------------------\n"
             << endl;

    }

    close(fd2);

}


/**
 *
 * @return
 */
int main(){

    ///Instancia del controlador de la sintaxis de SQL.
    serverController = new ServerController();

    ///Corre el servidor
    runServer();

    return 0;
}

