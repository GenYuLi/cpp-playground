#!/bin/bash

set -e

# Function to check for a command
has() {
  type "$1" > /dev/null 2>&1
}

# 1. Check for Nix
if has nix-shell; then
  echo "Nix detected. You should use 'nix-shell' or 'direnv allow' to set up your environment."
  echo "Run 'nix-shell' to enter the development environment."
  exit 0
fi

echo "Nix not found. Checking for system dependencies..."

MISSING_DEPS=false

# 2. Check for pkg-config
if ! has pkg-config; then
  echo "[-] pkg-config not found."
  MISSING_DEPS=true
else
  echo "[+] pkg-config found."
fi

# 3. Check for CMake
if ! has cmake; then
    echo "[-] cmake not found."
    MISSING_DEPS=true
else
    echo "[+] cmake found."
fi

# 4. Check for Boost via pkg-config
# Note: On some systems boost might not provide pkg-config files, 
# but usually libboost-all-dev on Debian does, or we check for headers/libs manually if this fails.
# Here we stick to user request: "check pkgconfig".
if has pkg-config; then
    if ! pkg-config --exists boost && ! pkg-config --exists boost_system; then
        echo "[-] Boost not found via pkg-config."
        MISSING_DEPS=true
    else
        echo "[+] Boost found."
    fi
else
    # logic already handled above, but if pkg-config missing, we assume we need to install boost too
    MISSING_DEPS=true
fi

if [ "$MISSING_DEPS" = false ]; then
  echo "All dependencies appear to be satisfied!"
  exit 0
fi

# 5. Fallback installation
echo "Some dependencies are missing."

if has apt-get; then
    echo "Detected apt-based system (Debian/Ubuntu)."
    echo "We can attempt to install: build-essential cmake pkg-config libboost-all-dev"
    read -p "Do you want to run sudo apt-get install? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo apt-get update
        sudo apt-get install -y build-essential cmake pkg-config libboost-all-dev
        echo "Installation complete."
    else
        echo "Skipping installation. Please install dependencies manually."
        exit 1
    fi
else
    echo "Could not detect 'apt-get'. detecting other package managers is not fully implemented yet."
    echo "Please manually install: cmake, pkg-config, boost"
    exit 1
fi
