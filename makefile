CFLAGS = -Wall -Wextra -pedantic -std=c99
LDLIBS = -lncurses

EXEC = cto-do

all: $(EXEC)

$(EXEC): ctodo.o
	$(CC) ctodo.o -o $(EXEC) $(LDLIBS)

clean:
	@$(RM) *.o
	@$(RM) $(EXEC)
