#  Makefile                                                      -*-makefile-*-
#  ----------------------------------------------------------------------------

CMAKE_CC  = $(CC)
CMAKE_CXX = $(CXX)
BUILD     = bin

.PHONY: default build test distclean clean

default: test

stdexec:
	git clone https://github.com/NVIDIA/stdexec

test: build
	./$(BUILD)/coro

build:  stdexec
	@mkdir -p $(BUILD)
	cd $(BUILD); cmake .. # -DCMAKE_C_COMPILER=$(CMAKE_CC) -DCMAKE_CC_COMPILER=$(CMAKE_CXX)
	cmake --build $(BUILD)

clean:
	$(RM) mkerr olderr *~

distclean: clean
	$(RM) -r $(BUILD)
