import React, { useState } from "react";
import { Box, Button, Typography, TextField } from "@mui/material";
import { useScene } from "../SceneContext.jsx";

const SceneEditor = ({ onClose }) => {
  const { scenes, setScenes } = useScene();
  const [jsonValue, setJsonValue] = useState(JSON.stringify(scenes, null, 2));
  const [error, setError] = useState("");

  const handleSave = () => {
    try {
      const parsed = JSON.parse(jsonValue);
      setScenes(parsed);
      setError("");
      if (onClose) onClose();
    } catch (e) {
      setError("Invalid JSON: " + e.message);
    }
  };

  return (
    <Box
      sx={{
        p: 2,
        bgcolor: "background.paper",
        borderRadius: 2,
        boxShadow: 3,
        maxWidth: 600,
        mx: "auto",
        my: 4,
      }}
    >
      <Typography variant="h6" mb={2}>
        Scene Editor
      </Typography>
      <TextField
        multiline
        minRows={12}
        maxRows={24}
        fullWidth
        value={jsonValue}
        onChange={(e) => setJsonValue(e.target.value)}
        variant="outlined"
        sx={{ fontFamily: "monospace", mb: 2 }}
      />
      {error && (
        <Typography color="error" mb={2}>
          {error}
        </Typography>
      )}
      <Box sx={{ display: "flex", gap: 2 }}>
        <Button variant="contained" color="primary" onClick={handleSave}>
          Save
        </Button>
        <Button variant="outlined" onClick={onClose}>
          Cancel
        </Button>
      </Box>
    </Box>
  );
};

export default SceneEditor;
