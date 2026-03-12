# Building the Collaborative Editor Mod

## Prerequisites

1. **Geode SDK**: Install the Geode modding framework for Geometry Dash
2. **Visual Studio 2022**: Required for Windows compilation
3. **Node.js**: For the server component
4. **websocketpp library**: Should be automatically handled by Geode

## Setup Instructions

### 1. Geode SDK Setup

1. Download and install Geode from https://geode-sdk.org/
2. Set up your development environment following the official Geode documentation
3. Ensure you have the Geode CMake toolchain configured

### 2. Build the Mod

```bash
# Create build directory
mkdir build
cd build

# Configure with Geode (adjust paths as needed)
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/geode/cmake/GeodeToolchain.cmake

# Build the mod
cmake --build . --config Release
```

### 3. Install the Mod

1. Copy the generated `.geode` file to your Geode mods folder
2. Launch Geometry Dash to test

### 4. Setup the Server

```bash
cd server
npm install
npm start
```

## Troubleshooting

### Common Issues

1. **Geode not found**: Ensure the Geode SDK is properly installed and the CMake toolchain path is correct
2. **Missing websocketpp**: Install via vcpkg or ensure Geode provides it
3. **Compilation errors**: Check that all dependencies are properly linked

### Build Variants

- **Debug**: Use `--config Debug` for development
- **Release**: Use `--config Release` for distribution

## Dependencies

The mod requires the following libraries:
- websocketpp (WebSocket client)
- nlohmann/json (JSON parsing - usually provided by Geode)
- Geode SDK v5.3.0

## Testing

1. Start the server: `cd server && npm start`
2. Launch Geometry Dash with the mod installed
3. Open the level editor
4. Click the "Collab" button to open the collaboration panel
5. Test hosting and joining sessions

## Architecture Notes

The mod uses a client-server architecture:
- **Client**: C++ mod running in Geometry Dash
- **Server**: Node.js WebSocket relay server
- **Protocol**: JSON messages over WebSocket

For detailed architecture information, see the main README.md file.
