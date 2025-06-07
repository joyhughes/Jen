#!/bin/bash

# Jen Kaleidoscope App - One Command Startup
# Builds and runs the complete application with video recording

set -e

echo "🚀 Starting Jen Kaleidoscope Application..."
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Start the development container
echo "📦 Starting development environment..."
docker-compose up -d jen-dev

# Wait for container to be ready
echo "⏳ Waiting for container to be ready..."
sleep 5

# Check if container is running
if ! docker-compose ps jen-dev | grep -q "Up"; then
    echo "❌ Container failed to start. Check logs with: docker-compose logs jen-dev"
    exit 1
fi

echo "✅ Container is running!"
echo ""

# Start the React development server in background
echo "🌐 Starting React development server..."
docker exec -d jen-dev-environment bash -c "cd lux_react && npm start"

# Wait a moment for server to start
sleep 3

echo ""
echo "🎉 Jen Kaleidoscope Application is ready!"
echo ""
echo "📱 Open your browser and go to:"
echo "   👉 http://localhost:3000"
echo ""
echo "🎥 Features available:"
echo "   ✅ Live camera feed"
echo "   ✅ Real-time kaleidoscope effects"
echo "   ✅ H.264/MP4 video recording"
echo "   ✅ Hot-reload development"
echo ""
echo "🔧 Useful commands:"
echo "   • Stop app:           docker-compose down"
echo "   • View logs:          docker-compose logs jen-dev"
echo "   • Connect to shell:   docker exec -it jen-dev-environment bash"
echo "   • Rebuild:           docker-compose build --no-cache jen-dev"
echo ""
echo "🎯 Development workflow:"
echo "   • Edit React code → Auto reloads"
echo "   • Edit C++ code → Run 'make' in container"
echo "   • Test at http://localhost:3000"
echo ""

# Optional: Open browser automatically (uncomment if desired)
# if command -v open &> /dev/null; then
#     sleep 2
#     open http://localhost:3000
# fi

echo "Happy coding! 🎨" 