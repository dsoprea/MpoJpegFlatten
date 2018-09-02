CXXFLAGS=-Wall -Wextra -Werror -O0 -g3 -std=c++11
LIBS=-lturbojpeg
BUILD_PATH=build
CC=g++

$(BUILD_PATH)/main: $(BUILD_PATH)/main.o $(BUILD_PATH)/image_data.o $(BUILD_PATH)/scanline_collector.o
	$(CC) $(CXXFLAGS) -o $(BUILD_PATH)/main \
		$(BUILD_PATH)/main.o \
		$(BUILD_PATH)/image_data.o \
		$(BUILD_PATH)/scanline_collector.o \
		$(LIBS)

$(BUILD_PATH)/%.o: %.cpp
	mkdir -p $(BUILD_PATH)
	$(CC) $(CXXFLAGS) -c $< -o $@

.PHONY: test

test: $(BUILD_PATH)/main
	$(BUILD_PATH)/main resource/IMAG0185.mpo $(BUILD_PATH)/IMAG0185.mpo.jpg
