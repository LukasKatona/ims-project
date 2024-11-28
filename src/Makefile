CXX = g++
CXXFLAGS = -g -O2
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = ims-proj

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lsimlib -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe *.dat *.out
	rm -rf tests

run: clean all
	./$(TARGET)

plot: run
	python3 outputParser.py
