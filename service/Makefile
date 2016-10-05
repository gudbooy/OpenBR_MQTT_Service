include ../config.mk

.PHONY: all install uninstall reallyclean clean

all : mosquitto_pub mosquitto_sub listen_pub sub_send

FLAG1=`pkg-config --libs --cflags dbus-1 glib-2.0 dbus-glib-1` 


listen_pub : listen_pub.o client_shared.o
	${CROSS_COMPILE}${CC} $(FLAG1)$^ -o $@ $(FLAG1) -lpthread ${CLIENT_LDFLAGS}
sub_send : sub_send.o client_shared.o
	${CROSS_COMPILE}${CC} $(FLAG1)$^ -g -o $@ $(FLAG1) -lpthread ${CLIENT_LDFLAGS}
listen_pub.o :  listen_pub.c ../lib/libmosquitto.so.${SOVERSION}
	${CROSS_COMPILE}${CC} $(FLAG1)-c $< -o  $@ ${CLIENT_CFLAGS} $(FLAG1)
sub_send.o :sub_send.c ../lib/libmosquitto.so.${SOVERSION}
	${CROSS_COMPILE}${CC} $(FLAG1)-c $< -g -o  $@ ${CLIENT_CFLAGS} $(FLAG1)






mosquitto_pub : pub_client.o client_shared.o
	${CROSS_COMPILE}${CC} $^ -o $@  ${CLIENT_LDFLAGS}

mosquitto_sub : sub_client.o client_shared.o
	${CROSS_COMPILE}${CC} $^ -o $@ ${CLIENT_LDFLAGS}

pub_client.o : pub_client.c ../lib/libmosquitto.so.${SOVERSION}
	${CROSS_COMPILE}${CC} -c $< -o $@ ${CLIENT_CFLAGS}

sub_client.o : sub_client.c ../lib/libmosquitto.so.${SOVERSION}
	${CROSS_COMPILE}${CC} -c $< -o $@ ${CLIENT_CFLAGS}

client_shared.o : client_shared.c client_shared.h
	${CROSS_COMPILE}${CC} -c $< -o $@ ${CLIENT_CFLAGS}

../lib/libmosquitto.so.${SOVERSION} :
	$(MAKE) -C ../lib

install : all
	$(INSTALL) -d ${DESTDIR}$(prefix)/bin
	$(INSTALL) -s --strip-program=${CROSS_COMPILE}${STRIP} mosquitto_pub ${DESTDIR}${prefix}/bin/mosquitto_pub
	$(INSTALL) -s --strip-program=${CROSS_COMPILE}${STRIP} mosquitto_sub ${DESTDIR}${prefix}/bin/mosquitto_sub

uninstall :
	-rm -f ${DESTDIR}${prefix}/bin/mosquitto_pub
	-rm -f ${DESTDIR}${prefix}/bin/mosquitto_sub

reallyclean : clean

clean : 
	-rm -f *.o mosquitto_pub mosquitto_sub
