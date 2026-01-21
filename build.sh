#!/usr/bin/env bash
# Build script for cpp-playground with Python bindings
# Uses the local .venv Python to avoid environment conflicts

set -e

# Use venv Python if available, otherwise fall back to system python
if [ -f ".venv/bin/python" ]; then
    PYTHON_EXE="$(pwd)/.venv/bin/python"
else
    echo "Warning: .venv not found, using system python"
    PYTHON_EXE=$(which python3)
fi

echo "Using Python: $PYTHON_EXE"
echo "Python version: $($PYTHON_EXE --version)"

# Clean environment variables that cause ABI mismatch
unset PYTHONPATH
unset _PYTHON_SYSCONFIGDATA_NAME

# Clean and create build directory
rm -rf build
mkdir build
cd build

# Configure with venv Python
cmake -DPython3_EXECUTABLE="$PYTHON_EXE" -DPython3_FIND_VIRTUALENV=ONLY ..

# Build orderbook_py target
make orderbook_py -j$(nproc)

cd ..

# Copy .so to venv site-packages
SO_FILE=$(ls build/orderbook_py.*.so 2>/dev/null | head -1)
if [ -n "$SO_FILE" ] && [ -d ".venv/lib" ]; then
    SITE_PACKAGES=$(find .venv/lib -name "site-packages" -type d | head -1)
    cp "$SO_FILE" "$SITE_PACKAGES/"
    echo ""
    echo "Installed: $SO_FILE -> $SITE_PACKAGES/"
fi

echo ""
echo "Build successful!"
echo "Test with: python -c 'import orderbook_py; print(orderbook_py.OrderBook())'"
