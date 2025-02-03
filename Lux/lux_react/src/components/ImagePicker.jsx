import React, { useState, useRef } from 'react';
import { Box, Typography } from '@mui/material';
import { Upload } from 'lucide-react';
import JenMenu from './JenMenu';

export const ImagePicker = ({ json, width, onChange }) => {
    const [isDragging, setIsDragging] = useState(false);
    const fileInputRef = useRef(null);
    const [menuItems, setMenuItems] = useState(json.items || []);

    const handleFileUpload = async (file) => {
        try {
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
                    const newItems = [...menuItems];
                    if (!newItems.includes(imageName)) {
                        newItems.push(imageName);
                        setMenuItems(newItems);
                    }
                    window.module.update_source_name(imageName);

                }
            };
            reader.readAsArrayBuffer(file);
        } catch (error) {
            console.error('Failed to upload image:', error);
        }
    };

    const handleMenuChange = (newValue) => {
        if (window.module && newValue) {
            window.module.update_source_name(newValue);
            onChange(newValue);
        }
    };

    return (
        <Box sx={{ width }}>
            <JenMenu
                json={{...json, items: menuItems}}
                width={width}
                onChange={handleMenuChange}
            />

            <input
                ref={fileInputRef}
                type="file"
                accept="image/*"
                onChange={(e) => e.target.files?.[0] && handleFileUpload(e.target.files[0])}
                style={{ display: 'none' }}
            />

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
                <Upload size={24} />
                <Typography sx={{ mt: 1 }}>Add new image</Typography>
            </Box>
        </Box>
    );
};