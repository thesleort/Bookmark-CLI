CXX      ?= g++
BINARY_DIR  ?= $(HOME)/.local/bmk
BIN         ?= $(HOME)/.local/bin

.PHONY: all build clean install sudo-install test

all: build

build:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$$(nproc)
	@echo "Build complete: build/bmkbin"

clean:
	@rm -rf build
	@echo "Cleaned build directory"

install: build
	@mkdir -p $(BINARY_DIR)
	@mkdir -p $(BIN)
	@cp build/bmkbin $(BINARY_DIR)/bmkbin
	@cp scripts/bmk-wrapper.sh $(BIN)/bmk
	@chmod +x $(BINARY_DIR)/bmkbin $(BIN)/bmk
	@# Install completion scripts
	@cp scripts/bmk-completion.bash $(BINARY_DIR)/completion.bash
	@cp scripts/bmk-completion.zsh $(BINARY_DIR)/completion.zsh
	@# Ensure the wrapper is sourced in shell config so functions persist
	@if [ -f $(HOME)/.bashrc ] && ! grep -q 'source ~/.local/bin/bmk' $(HOME)/.bashrc; then \
		echo 'source ~/.local/bin/bmk' >> $(HOME)/.bashrc; \
		echo "  Added 'source ~/.local/bin/bmk' to $(HOME)/.bashrc (restart your shell)"; \
	fi
	@if [ -f $(HOME)/.bashrc ] && ! grep -q 'source ~/.local/bmk/completion.bash' $(HOME)/.bashrc; then \
		echo 'source ~/.local/bmk/completion.bash' >> $(HOME)/.bashrc; \
		echo "  Added 'source ~/.local/bmk/completion.bash' to $(HOME)/.bashrc (restart your shell)"; \
	fi
	@if [ -f $(HOME)/.zshrc ] && ! grep -q 'source ~/.local/bin/bmk' $(HOME)/.zshrc; then \
		echo 'source ~/.local/bin/bmk' >> $(HOME)/.zshrc; \
		echo "  Added 'source ~/.local/bin/bmk' to $(HOME)/.zshrc (restart your shell)"; \
	fi
	@if [ -f $(HOME)/.zshrc ] && ! grep -q 'source ~/.local/bmk/completion.zsh' $(HOME)/.zshrc; then \
		echo 'source ~/.local/bmk/completion.zsh' >> $(HOME)/.zshrc; \
		echo "  Added 'source ~/.local/bmk/completion.zsh' to $(HOME)/.zshrc (restart your shell)"; \
	fi
	@echo "Installed:"
	@echo "  $(BIN)/bmk            (wrapper, already in PATH)"
	@echo "  $(BINARY_DIR)/bmkbin  (binary)"
	@echo "  $(BINARY_DIR)/completion.bash  (bash completion)"
	@echo "  $(BINARY_DIR)/completion.zsh   (zsh completion)"

sudo-install: build
	@mkdir -p /usr/local/bmk
	@cp build/bmkbin /usr/local/bmk/bmkbin
	@cp scripts/bmk-wrapper.sh /usr/local/bin/bmk
	@chmod +x /usr/local/bmk/bmkbin /usr/local/bin/bmk
	@echo "Installed (as root):"
	@echo "  /usr/local/bin/bmk         (wrapper, already in PATH)"
	@echo "  /usr/local/bmk/bmkbin     (binary)"

test: build
	@echo "Running basic tests..."
	@mkdir -p /tmp/bmk-test-$$
	@cd /tmp/bmk-test-$$ && $(CURDIR)/build/bmkbin mkenv && \
	$(CURDIR)/build/bmkbin add testbook && \
	$(CURDIR)/build/bmkbin ls && \
	$(CURDIR)/build/bmkbin go testbook && \
	$(CURDIR)/build/bmkbin rm testbook && \
	$(CURDIR)/build/bmkbin ls && \
	@echo "All basic tests passed!" && \
	rm -rf /tmp/bmk-test-$$

uninstall:
	@rm -rf $(BMK_DIR)
	@echo "Uninstalled $(BMK_DIR)"
