OBJECT_FILES :=
OBJECT_FILES += core.o
OBJECT_FILES += counterd.o

TEST_OBJECTS :=
TEST_OBJECTS := core.o

BINARIES :=
BINARIES += counterd

TESTS :=
TESTS += t/01-variance

LDLIBS :=
LDLIBS := -lm
LDLIBS += -lctap

all: $(BINARIES)
clean:
	find . -name '*.o' | xargs rm -f
	rm -f $(BINARIES)
check: test
test: $(TESTS)
	prove -e '' -lv $(TESTS)

counterd: $(OBJECT_FILES)
t/01-variance: t/01-variance.c $(TEST_OBJECTS)
