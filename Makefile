BUILD_DIR := build

.PHONY: all configure build test clean rebuild run release

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build $(BUILD_DIR) -j$$(nproc)

test: configure
	cmake --build $(BUILD_DIR) --target tests -j$$(nproc)
	cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	cmake --build $(BUILD_DIR) --target clean

rebuild: clean build

run: build
	./$(BUILD_DIR)/FO2MapEdit

release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -j$$(nproc)
