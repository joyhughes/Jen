# Jen Development Guide

Complete guide for running and developing the Jen kaleidoscope application with live camera and video recording.

## 🚀 Quick Start (Just Run the App)

For users who just want to run the app quickly:

```bash
# 1. Clone and enter directory
git clone <repo-url>
cd Jen

# 2. One command to build and run everything
docker-compose up jen-dev

# 3. In a new terminal, start the React app
docker exec -it jen-dev-environment bash -c "cd lux_react && npm start"

# 4. Open browser: http://localhost:3000
```

**That's it!** The app will be running with full video recording capabilities.

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
cd Jen

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
- **📁 File Mounting**: Edit files locally, changes reflect in container
- **🎥 Live Camera**: Real-time camera processing with WebAssembly
- **🎬 Video Recording**: H.264/MP4 recording with FFmpeg
- **🔧 Build Tools**: Full C++ to WebAssembly compilation

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
# Check build status
make status

# Rebuild WebAssembly (if C++ code changed)
make clean && make

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

# Check FFmpeg libraries
ls -la /app/external/build/lib/libav*.a
ls -la /app/external/build/lib/libx264.a

# Verify Node.js version
node --version    # Should be v18.x.x
npm --version     # Should be v9.x.x+

# Test compilation environment
cd /app && make info
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
   - Edit React code locally (auto-reloads)
   - Edit C++ code locally
   - Test in browser: http://localhost:3000

4. **If C++ Changed**:
   ```bash
   # Inside container
   cd /app && make
   # Restart React server to pick up new WebAssembly
   ```

### File Structure

```
Jen/
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
├── Dockerfile            # Development environment setup
├── docker-compose.yml    # Container orchestration
└── Makefile             # C++ build system
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
# Inside container
cd /app && make clean && make
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