import React, { useState } from 'react';
import {
    Dialog,
    DialogTitle,
    DialogContent,
    DialogActions,
    Button,
    Typography,
    Slider,
    Select,
    MenuItem,
    FormControl,
    InputLabel,
    Grid,
    Box,
    useMediaQuery,
    useTheme
} from '@mui/material';
import { Settings } from 'lucide-react';

const VideoRecordingSettings = ({ open, onClose, onApply, defaultSettings }) => {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));

    // Default settings
    const [settings, setSettings] = useState(defaultSettings || {
        fps: 30,
        bitrate: 2000000, // 2 Mbps
        codec: "libx264",
        format: "mp4",
        preset: "medium"
    });

    const handleChange = (prop) => (event) => {
        setSettings({
            ...settings,
            [prop]: event.target.value
        });
    };

    const handleSliderChange = (prop) => (event, newValue) => {
        setSettings({
            ...settings,
            [prop]: newValue
        });
    };

    const handleApply = () => {
        onApply(settings);
        onClose();
    };

    // Formatter for the bitrate slider
    const formatBitrate = (value) => {
        if (value >= 1000000) {
            return `${value / 1000000} Mbps`;
        } else {
            return `${value / 1000} Kbps`;
        }
    };

    return (
        <Dialog
            open={open}
            onClose={onClose}
            fullWidth
            maxWidth="sm"
            PaperProps={{
                sx: {
                    borderRadius: 2,
                    boxShadow: theme.shadows[10]
                }
            }}
        >
            <DialogTitle sx={{
                display: 'flex',
                alignItems: 'center',
                gap: 1,
                pb: 1
            }}>
                <Settings size={24} />
                <Typography variant="h6">Video Recording Settings</Typography>
            </DialogTitle>

            <DialogContent dividers>
                <Grid container spacing={3}>
                    {/* FPS Slider */}
                    <Grid item xs={12}>
                        <Typography gutterBottom>Frame Rate (FPS)</Typography>
                        <Slider
                            value={settings.fps}
                            onChange={handleSliderChange('fps')}
                            min={15}
                            max={60}
                            step={1}
                            marks={[
                                { value: 15, label: '15' },
                                { value: 30, label: '30' },
                                { value: 60, label: '60' }
                            ]}
                            valueLabelDisplay="auto"
                        />
                    </Grid>

                    {/* Bitrate Slider */}
                    <Grid item xs={12}>
                        <Typography gutterBottom>Bitrate</Typography>
                        <Slider
                            value={settings.bitrate}
                            onChange={handleSliderChange('bitrate')}
                            min={500000}
                            max={5000000}
                            step={100000}
                            marks={[
                                { value: 500000, label: '0.5 Mbps' },
                                { value: 2000000, label: '2 Mbps' },
                                { value: 5000000, label: '5 Mbps' }
                            ]}
                            valueLabelDisplay="auto"
                            valueLabelFormat={formatBitrate}
                        />
                    </Grid>

                    {/* Codec Selection */}
                    <Grid item xs={12} sm={6}>
                        <FormControl fullWidth variant="outlined">
                            <InputLabel>Codec</InputLabel>
                            <Select
                                value={settings.codec}
                                onChange={handleChange('codec')}
                                label="Codec"
                            >
                                <MenuItem value="libx264">H.264 (Recommended)</MenuItem>
                                <MenuItem value="libvpx">VP8</MenuItem>
                                <MenuItem value="libvpx-vp9">VP9</MenuItem>
                            </Select>
                        </FormControl>
                    </Grid>

                    {/* Format Selection */}
                    <Grid item xs={12} sm={6}>
                        <FormControl fullWidth variant="outlined">
                            <InputLabel>Format</InputLabel>
                            <Select
                                value={settings.format}
                                onChange={handleChange('format')}
                                label="Format"
                            >
                                <MenuItem value="mp4">MP4 (Recommended)</MenuItem>
                                <MenuItem value="webm">WebM</MenuItem>
                                <MenuItem value="mov">MOV</MenuItem>
                            </Select>
                        </FormControl>
                    </Grid>

                    {/* Encoder Preset */}
                    <Grid item xs={12}>
                        <FormControl fullWidth variant="outlined">
                            <InputLabel>Encoding Preset</InputLabel>
                            <Select
                                value={settings.preset}
                                onChange={handleChange('preset')}
                                label="Encoding Preset"
                            >
                                <MenuItem value="ultrafast">Ultrafast (Lowest Quality)</MenuItem>
                                <MenuItem value="superfast">Superfast</MenuItem>
                                <MenuItem value="veryfast">Very Fast</MenuItem>
                                <MenuItem value="faster">Faster</MenuItem>
                                <MenuItem value="fast">Fast</MenuItem>
                                <MenuItem value="medium">Medium (Balanced)</MenuItem>
                                <MenuItem value="slow">Slow (Higher Quality)</MenuItem>
                                <MenuItem value="slower">Slower</MenuItem>
                            </Select>
                        </FormControl>
                    </Grid>

                    {/* Info Box */}
                    <Grid item xs={12}>
                        <Box sx={{
                            backgroundColor: theme.palette.background.default,
                            borderRadius: 1,
                            p: 2,
                            mt: 1
                        }}>
                            <Typography variant="body2" color="text.secondary">
                                <strong>Tips:</strong>
                                <ul>
                                    <li>Higher FPS creates smoother video but requires more processing power</li>
                                    <li>Higher bitrate improves quality but increases file size</li>
                                    <li>The "Medium" preset balances quality and encoding speed</li>
                                    <li>Mobile devices should use faster presets and lower bitrates</li>
                                </ul>
                            </Typography>
                        </Box>
                    </Grid>
                </Grid>
            </DialogContent>

            <DialogActions sx={{ px: 3, py: 2 }}>
                <Button onClick={onClose} variant="outlined">
                    Cancel
                </Button>
                <Button onClick={handleApply} variant="contained" color="primary">
                    Apply Settings
                </Button>
            </DialogActions>
        </Dialog>
    );
};

export default VideoRecordingSettings;