OBJECT_FILES :=
OBJECT_FILES += core.o

TEST_OBJECTS :=
TEST_OBJECTS := core.o

BINARIES :=
BINARIES += counterd
BINARIES += sampled

TESTS :=
TESTS += t/01-variance

LDLIBS :=
LDLIBS := -lm
LDLIBS += -lctap
LDLIBS += -lvigor

all: $(BINARIES)
clean:
	find . -name '*.o' | xargs rm -f
	rm -f $(BINARIES)
check: test
test: $(TESTS)
	prove -e '' -lv $(TESTS)

counterd: $(OBJECT_FILES) counterd.o
sampled:  $(OBJECT_FILES) sampled.o
t/01-variance: t/01-variance.c $(TEST_OBJECTS)
