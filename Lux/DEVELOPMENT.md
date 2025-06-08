# Jen Development Guide

Complete guide for running and developing the Jen kaleidoscope application with live camera and video recording.

## 🚀 Quick Start (Just Run the App)

For users who just want to run the app quickly:

```bash
# 1. Clone and enter directory
git clone <repo-url>
cd Jen/Lux

# 2. One command to build and run everything
docker-compose up jen-dev

# 3. In a new terminal, start the React app
docker exec -it jen-dev-environment bash -c "cd lux_react && npm start"

# 4. Open browser: http://localhost:3000
```

**That's it!** The app will be running with full video recording capabilities and incremental C++ builds.

---

## 🔧 Development Workflow

### Prerequisites

- [Docker](https://www.docker.com/get-started) 
- [Docker Compose](https://docs.docker.com/compose/install/)
- Modern web browser (Chrome/Firefox recommended)

### 1. Initial Setup

```bash
# Clone repository
git clone <repo-url>
cd Jen/Lux

# Build development environment (first time: 15-20 minutes)
docker-compose build jen-dev

# Start development container
docker-compose up -d jen-dev
```

### 2. Development Commands

```bash
# Connect to development container
docker exec -it jen-dev-environment bash

# Inside container - start React development server
cd lux_react
npm start

# Access your app: http://localhost:3000
```

### 3. Development Features

- **🔄 Hot Reload**: React code changes reload automatically
- **⚡ Incremental C++ Build**: Only recompiles changed C++ files (5-30 seconds vs 2-5 minutes)
- **👀 File Watcher**: Automatically rebuilds C++ when files change
- **📁 File Mounting**: Edit files locally, changes reflect in container
- **🎥 Live Camera**: Real-time camera processing with WebAssembly
- **🎬 Video Recording**: H.264/MP4 recording with FFmpeg
- **🔧 Smart Dependencies**: Tracks header file changes for accurate rebuilds

---

## 📋 Command Reference

### Quick Commands

```bash
# Start everything (one command)
docker-compose up jen-dev

# Stop everything  
docker-compose down

# Rebuild if needed
docker-compose build --no-cache jen-dev

# Connect to running container
docker exec -it jen-dev-environment bash

# Start React app (inside container)
cd lux_react && npm start
```

### Development Commands (Inside Container)

```bash
# Incremental C++ builds (automatic file watching)
./dev-watch.sh              # Starts file watcher (runs automatically)

# Manual C++ build (only rebuilds changed files)
./build-cpp.sh              # 5-30 seconds vs 2-5 minutes

# Install/update React dependencies
cd lux_react
npm install
npm audit fix

# Start development servers
npm start                    # Main dev server (port 3000)
npm run start-dev           # Alternative (port 5173)

# Build for production
npm run build
```

### Debugging Commands

```bash
# Verify WebAssembly build
ls -la /app/lux_react/src/lux.js
du -h /app/lux_react/src/lux.js

# Check incremental build objects
ls -la /app/web_build/*.o
ls -la /app/web_build/*.d  # Dependency files

# Check FFmpeg libraries
ls -la /app/external/build/lib/libav*.a
ls -la /app/external/build/lib/libx264.a

# Check STB image library
ls -la /app/stb_image/stb_image*.h

# Verify Node.js version
node --version    # Should be v18.x.x
npm --version     # Should be v9.x.x+

# Test manual C++ build
./build-cpp.sh

# Monitor file watcher (if running)
ps aux | grep inotifywait
```

---

## 🎯 Development Workflow

### Daily Development

1. **Start Environment** (if not running):
   ```bash
   docker-compose up -d jen-dev
   ```

2. **Start React Dev Server**:
   ```bash
   docker exec -it jen-dev-environment bash
   cd lux_react && npm start
   ```

3. **Develop**:
   - Edit React code locally → **Auto-reloads instantly**
   - Edit C++ code locally → **Auto-rebuilds in 5-30 seconds**
   - Test in browser: http://localhost:3000

4. **C++ Development**:
   - File watcher runs automatically in background
   - Change any `.cpp` or `.hpp` file → triggers incremental rebuild
   - Only changed files recompile → much faster builds
   - Browser automatically picks up changes

### File Structure

```
Jen/Lux/
├── src/                    # C++ source code
│   ├── lux_web.cpp        # Main WebAssembly interface  
│   ├── video_recorder.cpp # H.264/MP4 recording
│   ├── effect.cpp         # Kaleidoscope effects
│   └── ...                # Other C++ modules
├── lux_react/             # React frontend
│   ├── src/
│   │   ├── lux.js        # Compiled WebAssembly (auto-generated)
│   │   └── ...           # React components
│   ├── package.json      # Dependencies and scripts
│   └── vite.config.js    # Vite configuration
├── web_build/             # Incremental build objects (.o files)
├── stb_image/             # STB image library (auto-downloaded)
├── Dockerfile            # Development environment setup
├── docker-compose.yml    # Container orchestration
├── build-cpp.sh          # Incremental C++ build script
├── dev-watch.sh          # File watcher for auto-rebuilds
└── start-jen.sh          # One-command startup script
```

---

## 🧪 Testing Your App

### Camera Features
1. **Open**: http://localhost:3000
2. **Allow**: Camera permissions when prompted
3. **Verify**: Live camera feed appears
4. **Test**: Real-time kaleidoscope effects

### Video Recording
1. **Start Recording**: Click record button
2. **Apply Effects**: Change kaleidoscope settings while recording
3. **Stop Recording**: Click stop after a few seconds
4. **Download**: Verify MP4 file downloads
5. **Playback**: Test video plays correctly with effects

### Performance Testing
- **Frame Rate**: Should maintain 30-60 FPS
- **Latency**: Real-time effects with minimal delay
- **Memory**: Monitor browser memory usage
- **Mobile**: Test on mobile browsers

---

## 🔧 Troubleshooting

### Common Issues

**Camera not working:**
```bash
# Check browser permissions
# Try different browser (Chrome recommended)
# Ensure accessing via http://localhost (not file://)
```

**WebAssembly loading issues:**
```bash
# Inside container - clean rebuild
rm -f /app/web_build/*.o
./build-cpp.sh
# Restart React server
```

**Node.js/Vite errors:**
```bash
# Inside container
cd lux_react
rm -rf node_modules package-lock.json
npm install
npm start
```

**Container build issues:**
```bash
# Clean rebuild
docker-compose down
docker system prune -f
docker-compose build --no-cache jen-dev
```

### Port Conflicts

If ports 3000 or 5173 are busy:

```bash
# Edit docker-compose.yml ports section:
ports:
  - "3001:3000"  # Change host port
  - "5174:5173"  # Change host port
```

### Performance Issues

```bash
# Check container resources
docker stats jen-dev-environment

# Allocate more memory to Docker if needed
# (Docker Desktop → Settings → Resources)
```

---

## 🚀 Production Build

```bash
# Inside container
cd lux_react
npm run build

# Serve production build
npm run preview
# Access: http://localhost:4173
```

---

## 📊 Performance Tips

### Development Performance
- **Incremental Builds**: C++ changes now take 5-30 seconds instead of 2-5 minutes
- **File Watcher**: Automatic rebuilds save manual command running
- **Smart Dependencies**: Only rebuilds when headers actually change
- **Parallel Development**: React and C++ can be developed simultaneously

### Runtime Performance
- **Camera Resolution**: Lower resolution = better performance
- **Effect Complexity**: Simple effects = smoother playback  
- **Browser Choice**: Chrome generally performs best
- **Memory**: Monitor browser memory, refresh if needed
- **Mobile**: Test on actual mobile devices for real performance

---

## 🤝 Contributing

1. **Fork** the repository
2. **Create** feature branch: `git checkout -b feature-name`
3. **Develop** using this Docker environment
4. **Test** thoroughly with camera and recording
5. **Submit** pull request

---

## 📝 Environment Variables

You can customize the development environment:

```bash
# In docker-compose.yml or .env file
NODE_ENV=development
CHOKIDAR_USEPOLLING=true     # For file watching
WATCHPACK_POLLING=true       # For hot reload
VITE_PORT=3000              # Dev server port
```

---

Your Jen kaleidoscope application should now be running smoothly with full development capabilities! 🎉 