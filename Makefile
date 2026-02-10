BUILD_DIR := build

.PHONY: all configure build test clean rebuild run release lint lint-all fix fix-all format

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

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

lint: configure
	git diff --cached --name-only --diff-filter=d -- '*.cpp' '*.h' | xargs -r clang-tidy -p $(BUILD_DIR)

lint-all: configure
	find src/QFO2Tool test -name '*.cpp' -o -name '*.h' | xargs clang-tidy -p $(BUILD_DIR)

fix: configure
	git diff --cached --name-only --diff-filter=d -- 'src/QFO2Tool/*.cpp' 'src/QFO2Tool/*.h' | xargs -r clang-tidy -p $(BUILD_DIR) --fix

fix-all: configure
	find src/QFO2Tool test -name '*.cpp' -o -name '*.h' | xargs clang-tidy -p $(BUILD_DIR) --fix

format:
	find src/QFO2Tool test -name '*.cpp' -o -name '*.h' | xargs clang-format -i
