#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>    //dbus 헤더파일 



static void publish(DBusConnection *connection)
{
	DBusMessage *message;
	message = dbus_message_new_signal("/org/share/linux", "org.share.linux", "publish");

	/* Send the signal */
	dbus_connection_send (connection, message, NULL);
	dbus_message_unref(message);
	

}




int main (int argc, char **argv)
{

	DBusConnection *connection;
	DBusError error;
	dbus_error_init (&error);
	connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	if (!connection)
	{
		printf ("Failed to connect to the D-BUS daemon: %s", error.message);
		dbus_error_free (&error);
		return 1;
	}
	
	publish(connection);




	return 0;
}
