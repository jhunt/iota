OBJECT_FILES :=
OBJECT_FILES += core.o
OBJECT_FILES += counterd.o

BINARIES :=
BINARIES += counterd

all: $(BINARIES)
clean:
	rm -f $(BINARIES) $(OBJECT_FILES)

counterd: $(OBJECT_FILES)
