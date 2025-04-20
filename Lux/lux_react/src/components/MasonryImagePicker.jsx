import React, { useState, useRef, useEffect } from 'react';
import { Box, Typography, Button, CircularProgress, Paper } from '@mui/material';
import { Upload, ImagePlus, Check } from 'lucide-react';
import Masonry from 'react-masonry-css';
import './MasonryImagePicker.css';

export const MasonryImagePicker = ({ json, width, onChange }) => {
    const [images, setImages] = useState([]);
    const [selectedImage, setSelectedImage] = useState('');
    const [isDragging, setIsDragging] = useState(false);
    const [isLoading, setIsLoading] = useState(true);
    const fileInputRef = useRef(null);

    // Responsive breakpoints for Masonry
    const breakpointColumns = {
        default: 3,
        900: 2,
        500: 2
    };

    // Load images from the menu items on component mount
    useEffect(() => {
        if (json?.items) {
            const imageList = json.items.map(item => ({
                id: item,
                name: item,
                isSelected: json.choice !== undefined ?
                    (json.choice === json.items.indexOf(item)) : false
            }));
            setImages(imageList);

            // Set initially selected image
            if (json.choice !== undefined && json.items[json.choice]) {
                setSelectedImage(json.items[json.choice]);
            }

            setIsLoading(false);
        }
    }, [json]);

    const handleFileUpload = async (file) => {
        try {
            setIsLoading(true);
            const reader = new FileReader();
            reader.onload = async (e) => {
                const uint8Array = new Uint8Array(e.target.result);
                const imagePath = `/lux_files/${file.name}`;
                const imageName = file.name.split('.')[0];

                if (window.module) {
                    console.log('Writing file:', imagePath);
                    window.module.FS.writeFile(imagePath, uint8Array);
                    window.module.add_image_to_scene(imageName, imagePath);
                    window.module.add_to_menu('source_image_menu', imageName);

                    // Update images list
                    const newImage = {
                        id: imageName,
                        name: imageName,
                        isSelected: false
                    };

                    setImages(prevImages => {
                        // Check if image already exists to avoid duplicates
                        if (!prevImages.some(img => img.id === imageName)) {
                            return [...prevImages, newImage];
                        }
                        return prevImages;
                    });

                    // Auto-select the newly added image
                    handleImageSelect(imageName);
                }
                setIsLoading(false);
            };
            reader.readAsArrayBuffer(file);
        } catch (error) {
            console.error('Failed to upload image:', error);
            setIsLoading(false);
        }
    };

    const handleImageSelect = (imageId) => {
        setSelectedImage(imageId);

        // Update the selected image in the C++ backend
        if (window.module) {
            window.module.update_source_name(imageId);
            onChange(imageId);

            // Update the selected state for all images
            setImages(prevImages =>
                prevImages.map(img => ({
                    ...img,
                    isSelected: img.id === imageId
                }))
            );
        }
    };

    // Generates placeholder preview image for testing
    const getPlaceholderImage = (imageName) => {
        // This function will generate a repeatable color based on the image name
        const getColorFromName = (name) => {
            let hash = 0;
            for (let i = 0; i < name.length; i++) {
                hash = name.charCodeAt(i) + ((hash << 5) - hash);
            }
            const c = (hash & 0x00FFFFFF)
                .toString(16)
                .toUpperCase()
                .padStart(6, '0');
            return `#${c}`;
        };

        const color = getColorFromName(imageName);
        return `https://via.placeholder.com/150/${color.substring(1)}/FFFFFF?text=${imageName}`;
    };

    if (isLoading) {
        return (
            <Box
                sx={{
                    display: 'flex',
                    justifyContent: 'center',
                    alignItems: 'center',
                    height: 200,
                    width: width || '100%'
                }}
            >
                <CircularProgress />
            </Box>
        );
    }

    return (
        <Box sx={{ width: width || '100%' }}>
            <Typography variant="subtitle1" gutterBottom>
                Source Images
            </Typography>

            {/* Masonry grid for image thumbnails */}
            <Masonry
                breakpointCols={breakpointColumns}
                className="masonry-grid"
                columnClassName="masonry-grid-column"
            >
                {images.map((image) => (
                    <Paper
                        key={image.id}
                        className={`image-item ${image.isSelected ? 'selected' : ''}`}
                        onClick={() => handleImageSelect(image.id)}
                        elevation={image.isSelected ? 4 : 1}
                        sx={{
                            position: 'relative',
                            overflow: 'hidden',
                            borderRadius: 1,
                            mb: 1,
                            cursor: 'pointer',
                            transition: 'all 0.2s ease-in-out',
                            '&:hover': {
                                transform: 'translateY(-2px)',
                                boxShadow: 3
                            }
                        }}
                    >
                        {/* This would be replaced with actual image thumbnails */}
                        <Box
                            component="img"
                            src={getPlaceholderImage(image.name)}
                            alt={image.name}
                            sx={{
                                width: '100%',
                                height: 'auto',
                                display: 'block'
                            }}
                        />

                        {/* Image name overlay */}
                        <Box
                            sx={{
                                position: 'absolute',
                                bottom: 0,
                                left: 0,
                                right: 0,
                                backgroundColor: 'rgba(0,0,0,0.6)',
                                color: 'white',
                                padding: '4px 8px',
                                fontSize: '0.8rem',
                                display: 'flex',
                                justifyContent: 'space-between',
                                alignItems: 'center'
                            }}
                        >
                            <Typography variant="caption" noWrap>
                                {image.name}
                            </Typography>

                            {image.isSelected && (
                                <Check size={16} color="white" />
                            )}
                        </Box>
                    </Paper>
                ))}
            </Masonry>

            {/* Upload new image section */}
            <Box
                sx={{
                    mt: 2,
                    p: 2,
                    border: 2,
                    borderStyle: 'dashed',
                    borderColor: isDragging ? 'primary.main' : 'grey.300',
                    borderRadius: 1,
                    textAlign: 'center',
                    bgcolor: isDragging ? 'action.hover' : 'transparent',
                    transition: 'all 0.2s',
                    cursor: 'pointer'
                }}
                onDragOver={(e) => { e.preventDefault(); setIsDragging(true); }}
                onDragLeave={() => setIsDragging(false)}
                onDrop={(e) => {
                    e.preventDefault();
                    setIsDragging(false);
                    const file = e.dataTransfer.files[0];
                    if (file?.type.startsWith('image/')) handleFileUpload(file);
                }}
                onClick={() => fileInputRef.current?.click()}
            >
                <ImagePlus size={24} />
                <Typography sx={{ mt: 1 }}>Add new image</Typography>
                <input
                    ref={fileInputRef}
                    type="file"
                    accept="image/*"
                    onChange={(e) => e.target.files?.[0] && handleFileUpload(e.target.files[0])}
                    style={{ display: 'none' }}
                />
            </Box>
        </Box>
    );
};

export default MasonryImagePicker;