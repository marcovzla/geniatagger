CFLAGS = -O2 -DNDEBUG
#CFLAGS = -pg -O5
#CFLAGS = -g
OBJS = main.o maxent.o tokenize.o postag.o modeldata.o

a.out: $(OBJS)
	g++ -o geniatagger $(CFLAGS) $(OBJS)
clean:
	/bin/rm -r -f $(OBJS) a.out geniatagger *.o *~
.cpp.o:
	g++ -c $(CFLAGS) $<