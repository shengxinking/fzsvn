#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb -I/opt/mariadb/include
LDFLAGS = -L/opt/mariadb/lib -lmysqlclient -lsqlite3

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = database_test logtest

TEST = 

.PHONY : all test clean depclean

# for all target
all : $(TARGET)

database_test : mysql_api.o sqlite_api.o database_test.o

logtest: logtest.o

# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for depclean target
depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps
	rm -rf *.data *.sql


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d



