CFLAGS = -I. -I../server -I$(LIBEVENT)/include -g
LIBS = -L. -L../server -L$(LIBEVENT)/lib -levent -ldl -lm -lc 
AR = ar
AR_FLAGS = rc
RANLIB = ranlib

libpubsubclient.a: pubsubclient.o
	/bin/rm -f $@
	$(AR) $(AR_FLAGS) $@ $<
	$(RANLIB) $@

pubsub-reader: pubsub-reader.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) -lsimplehttp -lpubsubclient -lpthread -lcurl

all: libpubsubclient.a pubsub-reader

install:
	/usr/bin/install pubsub-reader /usr/bin
	/usr/bin/install pubsub2simplequeue.sh /usr/bin
	/usr/bin/install libpubsubclient.a $(LIBEVENT)/lib/
	/usr/bin/install -D pubsubclient.h $(LIBEVENT)/include/simplehttp/pubsubclient.h

clean:
	/bin/rm -f libpubsubclient.a pubsubclient.o pubsub-reader pubsub-reader.o
