
/**
 * Generates a deterministic color based on a string
 * @param {string} str - The string to generate a color from
 * @returns {string} - A CSS color string
 */
export function getPlaceholderColor(str) {
    if (!str) return '#444444';

    // Simple hash function
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
        hash = ((hash << 5) - hash) + str.charCodeAt(i);
        hash = hash & hash; // Convert to 32bit integer
    }

    // Generate HSL color with fixed saturation and lightness
    const hue = Math.abs(hash % 360);
    return `hsl(${hue}, 60%, 40%)`;
}

/**
 * Draws a placeholder thumbnail on a canvas
 * @param {HTMLCanvasElement} canvas - The canvas element to draw on
 * @param {string} name - The name to display
 * @param {number} size - The canvas size (assumes square)
 * @returns {boolean} - Success status
 */
export function drawPlaceholder(canvas, name, size) {
    if (!canvas) return false;

    const ctx = canvas.getContext('2d');
    if (!ctx) return false;

    // Fill with a color based on the name
    const color = getPlaceholderColor(name);
    ctx.fillStyle = color;
    ctx.fillRect(0, 0, size, size);

    // Add text
    ctx.fillStyle = '#FFFFFF';
    ctx.font = `${Math.max(8, size * 0.15)}px Arial`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    // Truncate long names
    let displayName = name.split('/').pop();
    if (displayName.length > 10) {
        displayName = displayName.substring(0, 8) + '...';
    }

    ctx.fillText(displayName, size / 2, size / 2);
    return true;
}

/**
 * Fetches and renders a thumbnail from the WebAssembly module
 * @param {HTMLCanvasElement} canvas - The canvas element to draw on
 * @param {string} imageName - The image name to fetch
 * @param {number} width - Canvas width
 * @param {number} height - Canvas height
 * @returns {Promise<object>} - Result with status and any error message
 */
export async function fetchAndDrawThumbnail(canvas, imageName, width, height) {
    if (!canvas || !imageName) {
        return { success: false, status: 'error', message: 'Invalid parameters' };
    }

    // Check if WebAssembly module is available
    if (!window.module) {
        return { success: false, status: 'error', message: 'Module not loaded' };
    }

    // Check if get_thumbnail function exists
    if (typeof window.module.get_thumbnail !== 'function') {
        return { success: false, status: 'error', message: 'No thumbnail API' };
    }

    try {
        // Get thumbnail from WebAssembly module
        const pixelDataVal = window.module.get_thumbnail(imageName, width, height);

        // Check if we got valid data
        if (!pixelDataVal) {
            return { success: false, status: 'error', message: 'No pixel data returned' };
        }

        const bufferLength = pixelDataVal.byteLength;

        // Validate buffer length
        if (!bufferLength || bufferLength !== width * height * 4) {
            return {
                success: false,
                status: 'error',
                message: `Invalid pixel data length: ${bufferLength}`,
                shouldRetry: true
            };
        }

        // Get clamped array view
        const pixelData = new Uint8ClampedArray(pixelDataVal.buffer, pixelDataVal.byteOffset, bufferLength);

        // Swizzle format if needed
        const rgbaPixelData = new Uint8ClampedArray(bufferLength);
        for (let i = 0; i < bufferLength; i += 4) {
            rgbaPixelData[i] = pixelData[i];         // R
            rgbaPixelData[i + 1] = pixelData[i + 1]; // G
            rgbaPixelData[i + 2] = pixelData[i + 2]; // B
            rgbaPixelData[i + 3] = 255;              // A (force full opacity)
        }

        // Create ImageData and get context
        const imageData = new ImageData(rgbaPixelData, width, height);
        const ctx = canvas.getContext('2d', { alpha: false });

        if (!ctx) {
            return { success: false, status: 'error', message: 'Could not get canvas context' };
        }

        // Create and render the bitmap
        const imageBitmap = await createImageBitmap(imageData);
        ctx.clearRect(0, 0, width, height);
        ctx.drawImage(imageBitmap, 0, 0, width, height);
        imageBitmap.close();

        return { success: true, status: 'loaded' };
    } catch (err) {
        return {
            success: false,
            status: 'error',
            message: err.message,
            shouldRetry: true
        };
    }
}