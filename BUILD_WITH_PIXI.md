# Building Mumble with Pixi

This document describes how to build the Mumble voice chat client using [Pixi](https://pixi.sh), a modern package manager for conda ecosystems.

## Prerequisites

1. Install Pixi following the instructions at https://pixi.sh
2. Ensure you have git installed

## Quick Start

```bash
# Initialize git submodules (required for 3rd party dependencies)
pixi run setup

# Configure and build in one step
pixi run build-all
```

The built Mumble client will be located in `build/src/mumble/mumble`.

## Detailed Build Process

### 1. Initialize Dependencies

```bash
# Clone and initialize all git submodules
pixi run setup
```

### 2. Install Build Dependencies

```bash
# Install all conda dependencies
pixi install
```

This installs:
- Build tools (cmake, ninja, compilers)
- Qt6 framework
- Audio libraries (ALSA, PortAudio, Opus, Speex, etc.)
- System libraries (OpenGL, X11, etc.)
- Development libraries (Boost, Protobuf, spdlog, etc.)

### 3. Configure Build

```bash
# Configure for release build
pixi run configure

# Or configure for debug build
pixi run configure-debug
```

### 4. Build

```bash
# Build with all available cores
pixi run build-parallel

# Or build with single thread
pixi run build
```

## Available Commands

| Command | Description |
|---------|-------------|
| `pixi run setup` | Initialize git submodules |
| `pixi run configure` | Configure CMake for release build |
| `pixi run configure-debug` | Configure CMake for debug build |
| `pixi run build` | Build the project |
| `pixi run build-parallel` | Build with parallel compilation |
| `pixi run build-client` | Configure and build in one step |
| `pixi run build-all` | Configure and build with parallel compilation |
| `pixi run test` | Run tests |
| `pixi run run-tests` | Build and run tests |
| `pixi run clean` | Clean build artifacts |
| `pixi run reset` | Remove build directory completely |

## Build Configuration

This pixi configuration builds Mumble with the following settings:

### Enabled Features
- Mumble client (GUI application)
- Qt6 interface
- ALSA and PortAudio support
- Opus and Speex audio codecs
- OpenGL overlay support
- Multi-language support

### Disabled Features (for compatibility)
- Murmur server components
- 32-bit overlay cross-compilation
- Crash reporting
- Plugin system
- WebRTC audio processing
- Speech dispatcher integration
- Zeroconf/mDNS service discovery

### Platform Support
Currently configured for Linux x86_64 only. Additional platforms can be added by:
1. Adding platform to `platforms` array in `pixi.toml`
2. Installing platform-specific dependencies
3. Testing the build configuration

## Troubleshooting

### Build Fails with Missing Dependencies
If the build fails due to missing system dependencies, you can try:
```bash
pixi run configure-full
```

This enables all features but may require additional system packages.

### OpenGL/X11 Issues
On some systems, you may need additional development packages:
```bash
# On Ubuntu/Debian
sudo apt install mesa-common-dev libxi-dev

# On Fedora/CentOS
sudo dnf install mesa-libGL-devel libXi-devel
```

### Compilation Errors
If you encounter C++ compilation errors, try:
1. Clean rebuild: `pixi run reset && pixi run build-all`
2. Check that git submodules are properly initialized: `pixi run setup`
3. Verify all dependencies are installed: `pixi install`

## Customization

To customize the build:

1. **Enable more features**: Edit the configure commands in `pixi.toml` to remove `-D<feature>=OFF` flags
2. **Add dependencies**: Add new packages to the `[dependencies]` section
3. **Change build type**: Modify `CMAKE_BUILD_TYPE` from `Release` to `Debug`

## Performance Notes

- First build will be slower due to downloading and compiling 3rd party libraries
- Subsequent builds are incremental and much faster
- Use `build-parallel` for fastest compilation on multi-core systems
- RNNoise and other ML components are downloaded during first configuration

## Output

The successful build produces:
- `build/src/mumble/mumble` - Main Mumble client executable
- `build/src/mumble/*.qm` - Translation files
- Various libraries in the build tree

## Contributing

When submitting patches that modify dependencies:
1. Update `pixi.toml` with any new required packages
2. Test the build on a clean environment
3. Document any new system requirements