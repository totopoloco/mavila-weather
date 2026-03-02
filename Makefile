# Makefile for weather C++ program
# Dependencies: libcurl, nlohmann-json

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -Wl,-Bdynamic -lcurl

# Version from git tags (sorted by version, falls back to 1.0.0 if no tags)
GIT_VERSION := $(shell git tag -l 'v*' --sort=-v:refname 2>/dev/null | head -n1 | sed 's/^v//')
VERSION := $(if $(GIT_VERSION),$(GIT_VERSION),1.0.0)
CXXFLAGS += -DVERSION_STRING=\"$(VERSION)\"

# Check for nlohmann-json location
# On Debian/Ubuntu: libnlohmann-json3-dev
# On Fedora/RHEL: nlohmann-json-devel
# On Arch: nlohmann-json

TARGET = weather
SOURCES = weather.cpp

.PHONY: all clean install deps

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

# Install dependencies (Debian/Ubuntu)
deps-debian:
	sudo apt-get update && sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev

# Install dependencies (Fedora/RHEL)
deps-fedora:
	sudo dnf install -y libcurl-devel nlohmann-json-devel

# Install dependencies (Arch)
deps-arch:
	sudo pacman -S --noconfirm curl nlohmann-json

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

install-completion:
	sudo cp weather-completion.bash /etc/bash_completion.d/weather

help:
	@echo "Available targets:"
	@echo "  all                - Build the weather program (default)"
	@echo "  clean              - Remove built files"
	@echo "  deps-debian        - Install dependencies on Debian/Ubuntu"
	@echo "  deps-fedora        - Install dependencies on Fedora/RHEL"
	@echo "  deps-arch          - Install dependencies on Arch Linux"
	@echo "  install            - Install to /usr/local/bin"
	@echo "  install-completion - Install bash completion to /etc/bash_completion.d"
	@echo "  help               - Show this help"
