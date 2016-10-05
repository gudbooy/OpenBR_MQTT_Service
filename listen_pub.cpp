/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <pthread.h>
#include <unistd.h>
// Dbus
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "service.h"
#include <iostream>
#include <string>
// MQTT
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include <mosquitto.h>
#include "client_shared.h"

#define STATUS_CONNECTING 0
#define STATUS_CONNACK_RECVD 1
#define STATUS_WAITING 2

/* Global variables for use in callbacks. See sub_client.c for an example of
 * using a struct to hold variables for use in callbacks. */
const char* anonymous = "anonymous";

static char *topic = NULL;
static char *message = NULL;
static long msglen = 0;
static int qos = 0;
static int retain = 0;
static int mode = MSGMODE_NONE;
static int status = STATUS_CONNECTING;
static int mid_sent = 0;
static int last_mid = -1;
static int last_mid_sent = -1;
static bool connected = true;
static char *username = NULL;
static char *password = NULL;
static bool disconnect_sent = false;
static bool quiet = false;
pthread_t thread;

void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
//	printf("connect callback!\n");
	int rc = MOSQ_ERR_SUCCESS;

	if(!result){
		switch(mode){
			case MSGMODE_CMD:
			case MSGMODE_FILE:
			case MSGMODE_STDIN_FILE:
				rc = mosquitto_publish(mosq, &mid_sent, topic, msglen, message, qos, retain);
				break;
			case MSGMODE_NULL:
				rc = mosquitto_publish(mosq, &mid_sent, topic, 0, NULL, qos, retain);
				break;
			case MSGMODE_STDIN_LINE:
				status = STATUS_CONNACK_RECVD;
				break;
		}
		if(rc){
			if(!quiet){
				switch(rc){
					case MOSQ_ERR_INVAL:
						fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
						break;
					case MOSQ_ERR_NOMEM:
						fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
						break;
					case MOSQ_ERR_NO_CONN:
						fprintf(stderr, "Error: Client not connected when trying to publish.\n");
						break;
					case MOSQ_ERR_PROTOCOL:
						fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
						break;
					case MOSQ_ERR_PAYLOAD_SIZE:
						fprintf(stderr, "Error: Message payload is too large.\n");
						break;
				}
			}
			mosquitto_disconnect(mosq);
		}
	}else{
		if(result && !quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	connected = false;
	disconnect_sent = false;
//	printf("disconnect callback!\n");
}

void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	last_mid_sent = mid;
	if(mode == MSGMODE_STDIN_LINE){
		if(mid == last_mid){
			mosquitto_disconnect(mosq);
			disconnect_sent = true;
		}
	}else if(disconnect_sent == false){
		mosquitto_disconnect(mosq);
		disconnect_sent = true;
//		connected = false;
//		printf("disconnect!\n");
	}
}

void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
//	printf("%s\n", str);
}

int load_stdin(void)
{
	long pos = 0, rlen;
	char buf[1024];
	char *aux_message = NULL;

	mode = MSGMODE_STDIN_FILE;

	while(!feof(stdin)){
		rlen = fread(buf, 1, 1024, stdin);
		aux_message = (char*)realloc(message, pos+rlen);
		if(!aux_message){
			if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
			free(message);
			return 1;
		} else
		{
			message = aux_message;
		}
		memcpy(&(message[pos]), buf, rlen);
		pos += rlen;
	}
	msglen = pos;

	if(!msglen){
		if(!quiet) fprintf(stderr, "Error: Zero length input.\n");
		return 1;
	}

	return 0;
}

int load_file(const char *filename)
{
	long pos, rlen;
	FILE *fptr = NULL;

	fptr = fopen(filename, "rb");
	if(!fptr){
		if(!quiet) fprintf(stderr, "Error: Unable to open file \"%s\".\n", filename);
		return 1;
	}
	mode = MSGMODE_FILE;
	fseek(fptr, 0, SEEK_END);
	msglen = ftell(fptr);
	if(msglen > 268435455){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is too large (>268,435,455 bytes).\n", filename);
		return 1;
	}else if(msglen == 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is empty.\n", filename);
		return 1;
	}else if(msglen < 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Unable to determine size of file \"%s\".\n", filename);
		return 1;
	}
	fseek(fptr, 0, SEEK_SET);
	message = (char*)malloc(msglen);
	if(!message){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	pos = 0;
	while(pos < msglen){
		rlen = fread(&(message[pos]), sizeof(char), msglen-pos, fptr);
		pos += rlen;
	}
	fclose(fptr);
	return 0;
}

void print_usage(void)
{
	int major, minor, revision;

	mosquitto_lib_version(&major, &minor, &revision);
	printf("mosquitto_pub is a simple mqtt client that will publish a message on a single topic and exit.\n");
//	printf("mosquitto_pub version %s running on libmosquitto %d.%d.%d.\n\n", VERSION, major, minor, revision);
	printf("Usage: mosquitto_pub [-h host] [-k keepalive] [-p port] [-q qos] [-r] {-f file | -l | -n | -m message} -t topic\n");
#ifdef WITH_SRV
	printf("                     [-A bind_address] [-S]\n");
#else
	printf("                     [-A bind_address]\n");
#endif
	printf("                     [-i id] [-I id_prefix]\n");
	printf("                     [-d] [--quiet]\n");
	printf("                     [-M max_inflight]\n");
	printf("                     [-u username [-P password]]\n");
	printf("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
#ifdef WITH_TLS
	printf("                     [{--cafile file | --capath dir} [--cert file] [--key file]\n");
	printf("                      [--ciphers ciphers] [--insecure]]\n");
#ifdef WITH_TLS_PSK
	printf("                     [--psk hex-key --psk-identity identity [--ciphers ciphers]]\n");
#endif
#endif
#ifdef WITH_SOCKS
	printf("                     [--proxy socks-url]\n");
#endif
	printf("       mosquitto_pub --help\n\n");
	printf(" -A : bind the outgoing socket to this host/ip address. Use to control which interface\n");
	printf("      the client communicates over.\n");
	printf(" -d : enable debug messages.\n");
	printf(" -f : send the contents of a file as the message.\n");
	printf(" -h : mqtt host to connect to. Defaults to localhost.\n");
	printf(" -i : id to use for this client. Defaults to mosquitto_pub_ appended with the process id.\n");
	printf(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
	printf("      broker is using the clientid_prefixes option.\n");
	printf(" -k : keep alive in seconds for this client. Defaults to 60.\n");
	printf(" -l : read messages from stdin, sending a separate message for each line.\n");
	printf(" -m : message payload to send.\n");
	printf(" -M : the maximum inflight messages for QoS 1/2..\n");
	printf(" -n : send a null (zero length) message.\n");
	printf(" -p : network port to connect to. Defaults to 1883.\n");
	printf(" -P : provide a password (requires MQTT 3.1 broker)\n");
	printf(" -q : quality of service level to use for all messages. Defaults to 0.\n");
	printf(" -r : message should be retained.\n");
	printf(" -s : read message from stdin, sending the entire input as a message.\n");
#ifdef WITH_SRV
	printf(" -S : use SRV lookups to determine which host to connect to.\n");
#endif
	printf(" -t : mqtt topic to publish to.\n");
	printf(" -u : provide a username (requires MQTT 3.1 broker)\n");
	printf(" -V : specify the version of the MQTT protocol to use when connecting.\n");
	printf("      Can be mqttv31 or mqttv311. Defaults to mqttv31.\n");
	printf(" --help : display this message.\n");
	printf(" --quiet : don't print error messages.\n");
	printf(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
	printf("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
	printf("                  length message will be sent.\n");
	printf(" --will-qos : QoS level for the client Will.\n");
	printf(" --will-retain : if given, make the client Will retained.\n");
	printf(" --will-topic : the topic on which to publish the client Will.\n");
#ifdef WITH_TLS
	printf(" --cafile : path to a file containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --capath : path to a directory containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --cert : client certificate for authentication, if required by server.\n");
	printf(" --key : client private key for authentication, if required by server.\n");
	printf(" --ciphers : openssl compatible list of TLS ciphers to support.\n");
	printf(" --tls-version : TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1.\n");
	printf("                 Defaults to tlsv1.2 if available.\n");
	printf(" --insecure : do not check that the server certificate hostname matches the remote\n");
	printf("              hostname. Using this option means that you cannot be sure that the\n");
	printf("              remote host is the server you wish to connect to and so is insecure.\n");
	printf("              Do not use this option in a production environment.\n");
#  ifdef WITH_TLS_PSK
	printf(" --psk : pre-shared-key in hexadecimal (no leading 0x) to enable TLS-PSK mode.\n");
	printf(" --psk-identity : client identity string for TLS-PSK mode.\n");
#  endif
#endif
#ifdef WITH_SOCKS
	printf(" --proxy : SOCKS5 proxy URL of the form:\n");
	printf("           socks5h://[username[:password]@]hostname[:port]\n");
	printf("           Only \"none\" and \"username\" authentication is supported.\n");
#endif
	printf("\nSee http://mosquitto.org/ for more information.\n\n");
}

//int main(int argc, char *argv[])
void *publish(void* arg)
{
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;
	int rc;
	int rc2;
	char *buf;
	int buf_len = 1024;
	int buf_len_actual;
	int read_len;
	int pos;
	int argc = 3;
	char** argv;
	

	argv = (char**) malloc(2*sizeof(char*));
	argv[0] = (char*)arg;

  connected = true;
	buf = (char*)malloc(buf_len);
	if(!buf){
		fprintf(stderr, "Error: Out of memory.\n");
		//return 1;
		exit(1);
	}

	memset(&cfg, 0, sizeof(struct mosq_config));
	rc = client_config_load(&cfg, CLIENT_PUB, argc, argv);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			/* --help */
			print_usage();
		}else{
			fprintf(stderr, "\nUse 'mosquitto_pub --help' to see usage.\n");
		}
		//return 1;
		exit(1);
	}

	free(argv);

	topic = cfg.topic;

	/********************* modifying part ***********/
	// message to send
	message = cfg.message;
	msglen = cfg.msglen;




	qos = cfg.qos;
	retain = cfg.retain;
	mode = cfg.pub_mode;
	username = cfg.username;
	password = cfg.password;
	quiet = cfg.quiet;

	if(cfg.pub_mode == MSGMODE_STDIN_FILE){
		if(load_stdin()){
			fprintf(stderr, "Error loading input from stdin.\n");
		//	return 1;
			exit(1);
		}
	}else if(cfg.file_input){
		if(load_file(cfg.file_input)){
			fprintf(stderr, "Error loading input file \"%s\".\n", cfg.file_input);
			//return 1;
			exit(1);
		}
	}

	if(!topic || mode == MSGMODE_NONE){
		fprintf(stderr, "Error: Both topic and message must be supplied.\n");
		print_usage();
		//return 1;
		exit(1);
	}


	mosquitto_lib_init();

	if(client_id_generate(&cfg, "mosqpub")){
		//return 1;
		exit(1);
	}

	mosq = mosquitto_new(cfg.id, true, NULL);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!quiet) fprintf(stderr, "Error: Invalid id.\n");
				break;
		}
		mosquitto_lib_cleanup();
	//	return 1;
		exit(1);
	}
	if(cfg.debug){
		mosquitto_log_callback_set(mosq, my_log_callback);
	}
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);

	if(client_opts_set(mosq, &cfg)){
	//	return 1;
		exit(1);
	}
	rc = client_connect(mosq, &cfg);
	if(rc) //return rc; 
	exit(1);

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_start(mosq);
	}

	do{
		if(mode == MSGMODE_STDIN_LINE){
			if(status == STATUS_CONNACK_RECVD){
				pos = 0;
				read_len = buf_len;
				while(fgets(&buf[pos], read_len, stdin)){
					buf_len_actual = strlen(buf);
					if(buf[buf_len_actual-1] == '\n'){
						buf[buf_len_actual-1] = '\0';
						rc2 = mosquitto_publish(mosq, &mid_sent, topic, buf_len_actual-1, buf, qos, retain);
						if(rc2){
							if(!quiet) fprintf(stderr, "Error: Publish returned %d, disconnecting.\n", rc2);
							mosquitto_disconnect(mosq);
						}
						break;
					}else{
						buf_len += 1024;
						pos += 1023;
						read_len = 1024;
						buf = (char*)realloc(buf, buf_len);
						if(!buf){
							fprintf(stderr, "Error: Out of memory.\n");
							//return 1;
							exit(1);
						}
					}
				}
				if(feof(stdin)){
					last_mid = mid_sent;
					status = STATUS_WAITING;
				}
			}else if(status == STATUS_WAITING){
				if(last_mid_sent == last_mid && disconnect_sent == false){
					mosquitto_disconnect(mosq);
					disconnect_sent = true;
				}
#ifdef WIN32
				Sleep(100);
#else
				usleep(100000);
#endif
			}
			rc = MOSQ_ERR_SUCCESS;
		}else{
			rc = mosquitto_loop(mosq, -1, 1);
	//		printf("aa\n");
		}
	}while(rc == MOSQ_ERR_SUCCESS && connected);
	
//	printf("bb\n");
//	connected = true;

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_stop(mosq, false);
	}

	if(message && mode == MSGMODE_FILE){
		free(message);
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
	}
//	return rc;
//	exit(1);

	
	pthread_join(thread, NULL);
	return NULL;
}



static DBusHandlerResult dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data)
{
	DBusMessageIter args;
	
	char* imgPath, data;
	const	char* temp_argv = NULL;
	std::string tmp;
	const char* forTest = "/home/mini/OPEL/service/00002_20160618051248.jpg";
	char *fileID;
	
	fileID = (char*) malloc(sizeof(char) * 25);
	fileID[24] = '\0';
	ImageProcessing* faceRecognition = new ImageProcessing();
	faceRecognition->initialize("FaceRecognition");
	faceRecognition->parseJsonData();

	if(dbus_message_is_signal(message, "org.share.linux", "publish"))
	{
		dbus_message_iter_init(message, &args);
		do{
		dbus_message_iter_get_basic(&args, &imgPath);
		}while(dbus_message_iter_next(&args));
		QString imgData(imgPath);
		strncpy(fileID, imgPath+46, 24);
		fprintf(stderr, "fileID : %s\n", fileID); 
		std::string appId(fileID);
		QString testImg(forTest);
//		imgData = forTest;
		// Publish logic
		std::cout << "Image Path : " << imgData.toStdString() << std::endl;
			
		targetDataSet* matchedItem = faceRecognition->getMatchingItem(imgData);
		if(matchedItem == NULL)
		{
			std::ostringstream age;
			age << faceRecognition->ageEstimation(imgData);
			std::cout << "Its anonymous !!!" << std::endl;	
			tmp += "{\"Name\":\"";
			tmp += "anonymous";
			tmp += "\",\"age\":\"";
			tmp += age.str();
			tmp += "\",\"gender\":\"";
			tmp += faceRecognition->genderEstimation(imgData).toStdString();
			tmp += "\",\"info\":\"";	
			tmp += "anonymous";
			tmp += "\",\"img\":\"";
			tmp += appId;
			tmp += "\"}";
			temp_argv = tmp.c_str();	
			std::cout << "matchedItem String : " << temp_argv << std::endl;
			/*anonymous*/
		}
		else
		{
			std::cout << "Name : " << matchedItem->name.toStdString() << std::endl;
			tmp += "{\"Name\":\"";
			tmp += matchedItem->name.toStdString();
			tmp += "\",\"age\":\"";
			tmp += matchedItem->age.toStdString();
			tmp += "\",\"gender\":\"";
			tmp += matchedItem->gender.toStdString();
			tmp += "\",\"info\":\"";
			tmp += matchedItem->personalInfo.toStdString();
			tmp += "\",\"img\":\"";
			tmp += appId;
			tmp += "\"}";
			temp_argv = tmp.c_str();	
			std::cout << "matchedItem String : " << temp_argv << std::endl;
			/*make data set*/
		}
		
//		printf("received well!\n");

		delete faceRecognition;
		pthread_create(&thread, NULL, &publish, (void*)temp_argv);			
		pthread_join(thread, NULL);
		
		
		
		
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

}

int main()
{
	DBusConnection *connection;
	DBusError error;

	/* glib main loop */
	GMainLoop *loop;
	loop = g_main_loop_new(NULL, FALSE);
	dbus_error_init(&error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if( dbus_error_is_set(&error))
	{
		printf("Error connecting to the daemon bus: %s", error.message);
		dbus_error_free(&error);
		return 1;
	}
	dbus_bus_add_match( connection, "type='signal',interface='org.share.linux'", NULL);
	dbus_connection_add_filter(connection, dbus_filter, loop, NULL);

	/* dbus-glib call */
	dbus_connection_setup_with_g_main(connection, NULL);

	/* run glib main loop */
	g_main_loop_run(loop);

	return 0;

}
	
