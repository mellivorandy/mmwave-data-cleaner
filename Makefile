CXX 	 ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

TARGET ?= data_cleaner

SRC_DIR   := src
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)
	@echo
	@echo "Build done."

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

debug: CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra
debug: clean all

release: CXXFLAGS := -std=c++17 -O3 -Wall -Wextra
release: clean all

run: all
	./$(TARGET)

clean:
	rm -rf $(TARGET) $(BUILD_DIR)

rebuild: clean all

# Remove all generated CSV files except the input file
clean-output:
	@find data/ -type f -name '*.csv' \
		! -name 'left-to-right.csv' \
		! -name 'right-to-left.csv' \
		! -name 'up-to-down.csv' \
		! -name 'down-to-up.csv' \
		! -name 'push.csv' \
		! -name 'pull.csv' \
		-delete

.PHONY: all clean debug release run rebuild clean-output
