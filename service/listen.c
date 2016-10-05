#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>


#define OPEL_INTERFACE "org.opel.mqtt"

static DBusHandlerResult dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data)
{    
	DBusMessageIter args;
	int data, data1;

	if(dbus_message_is_signal(message,"org.share.linux","on"))  // On Signal을 받은 경우 동작하는 핸들러 코드
	{       
		printf("Message cutomize received\n");  
		return DBUS_HANDLER_RESULT_HANDLED;  
	}

	if(dbus_message_is_signal(message,"org.share.linux","off")) // Off signal을 받은 경우 동작하는 핸들러 코드
	{
		dbus_message_iter_init(message, &args);

		do
		{
			dbus_message_iter_get_basic(&args, &data);
			printf("get data : %d\n", data);
		}while(dbus_message_iter_next(&args));

		printf("Message quit received\n");
		GMainLoop *loop = (GMainLoop*) user_data;
		g_main_loop_quit(loop);

		return DBUS_HANDLER_RESULT_HANDLED;

	}

	if(dbus_message_is_signal(message,"org.share.linux","publish"))
	{
		//Publish Logic
		printf("received well!\n");	






	}
	

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main()
{
	DBusConnection *connection;
	DBusError error;
	/* glib main loop */
	GMainLoop *loop;
	loop = g_main_loop_new(NULL,FALSE);   // main loop 설정
	dbus_error_init(&error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if ( dbus_error_is_set(&error) )
	{
		printf("Error connecting to the daemon bus: %s",error.message);
		dbus_error_free(&error);
		return 1;
	}
	dbus_bus_add_match (connection, "type='signal',interface='org.share.linux'", NULL);  //Dbus 위치 설정
	dbus_connection_add_filter (connection, dbus_filter, loop, NULL); //signal filter설정
	/* dbus-glib call */
	dbus_connection_setup_with_g_main(connection,NULL); 
	/* run glib main loop */
	g_main_loop_run(loop); // loop 시작
	return 0;
}








