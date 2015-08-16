CC = gcc
CFLAGS += -Wall  $(shell pkg-config --cflags gtk+-2.0 glib-2.0)
LIBS = $(shell pkg-config --libs gtk+-2.0 glib-2.0)

ARCH := $(shell uname -m)
ifeq ($(ARCH), x86_64)
  LIBS += -L/usr/lib/x86_64-linux-gnu/ -lOpenCL
else
  LIBS += -L/usr/lib/i386-linux-gnu/ -lOpenCL
endif

SRCS = main.c img.c clerr.c xmalloc.c
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
