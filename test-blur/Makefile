CC = gcc
CFLAGS += -Wall  $(shell pkg-config --cflags gtk+-2.0 glib-2.0)
LIBS = $(shell pkg-config --libs gtk+-2.0 glib-2.0)


SRCS = main.c img.c img_utils.c xmalloc.c
OBJS = $(subst .c,.o,$(SRCS))
EXE = image

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	@$(CC) $^ $(CFLAGS) $(LIBS) -o $@ $(LIBS)
	@echo "Compilation is complited: $@"

%.o:%.c
	@echo "Building $< --> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(subst .c,.d,$(SRCS))

%.d:%.c
	@$(CC) -M $(CPPFLAGS) $< > $@.$$$$ 2>/dev/null;		\
	sed 's,\($*\).o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;	\
	$(RM) $@.$$$$

clean:
	$(RM) *.o *.d *.c~ *.h~ $(EXE)
