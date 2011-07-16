CFLAGS = -O2 -DNDEBUG
#CFLAGS = -pg -O5
#CFLAGS = -g
CPP = g++
#CPP = g++
OBJS = main.o maxent.o postag.o tokenize.o bidir.o

geniatagger: $(OBJS)
	$(CPP) -o geniatagger $(CFLAGS) $(OBJS)
clean:
	/bin/rm -r -f $(OBJS) geniatagger *.o *~ *.flc
.cpp.o:
	$(CPP) -c $(CFLAGS) $<