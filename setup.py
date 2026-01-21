"""
Setup script for orderbook_py Python bindings.

This allows installing the C++ extension module via pip.

Usage:
    # Development install (after building with CMake)
    pip install -e .

    # Build and install from source
    pip install .
"""

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import os
from pathlib import Path


class CMakeExtension(Extension):
    """Extension that will be built by CMake."""
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Custom build extension that runs CMake."""

    def build_extension(self, ext):
        if isinstance(ext, CMakeExtension):
            # Get build directory
            extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

            # CMake build configuration
            cfg = 'Debug' if self.debug else 'Release'

            cmake_args = [
                f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}',
                f'-DPYTHON_EXECUTABLE={sys.executable}',
                f'-DCMAKE_BUILD_TYPE={cfg}',
            ]

            build_args = ['--config', cfg]

            # Build directory
            build_temp = Path(self.build_temp)
            build_temp.mkdir(parents=True, exist_ok=True)

            # Run CMake
            os.chdir(str(build_temp))
            self.spawn(['cmake', ext.sourcedir] + cmake_args)

            # Build the target
            self.spawn(['cmake', '--build', '.', '--target', 'orderbook_py'] + build_args)

            os.chdir(str(Path.cwd()))
        else:
            super().build_extension(ext)


# Read version from C++ module or default
version = '1.0.0'

# Read README for long description
readme_path = Path(__file__).parent / 'python/README.md'
long_description = readme_path.read_text() if readme_path.exists() else ''

setup(
    name='orderbook-py',
    version=version,
    author='witherslin',
    author_email='hank90555@gmail.com',
    description='High-performance C++20 orderbook with Python bindings',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/GenYuLi/cpp-playground',

    # Extension modules
    ext_modules=[CMakeExtension('orderbook_py', sourcedir='.')],
    cmdclass={'build_ext': CMakeBuild},

    # Python requirements
    python_requires='>=3.9',
    install_requires=[
        # No runtime dependencies - pure C++ extension
    ],
    extras_require={
        'dev': [
            'pytest>=7.0',
            'pytest-benchmark>=4.0',
        ],
    },

    # Package metadata
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Intended Audience :: Financial and Insurance Industry',
        'Topic :: Office/Business :: Financial',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
        'Programming Language :: C++',
    ],

    # Package discovery
    packages=[],  # No pure Python packages, only C++ extension
    zip_safe=False,
)
