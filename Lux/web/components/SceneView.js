// SceneView.js
import React from 'react';
import Accordion from '@mui/material/Accordion';
import AccordionSummary from '@mui/material/AccordionSummary';
import AccordionDetails from '@mui/material/AccordionDetails';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import Typography from '@mui/material/Typography';
import {Box} from '@mui/material';

function SceneView() {
  // This array contains our top-level labels for the accordion
  const sections = ["Queue", "Effects", "Clusters", "Functions", "Elements", "Images"];

  return (
    <Box>
      <Typography variant="h5">Scene View</Typography>
      {sections.map((section) => (
        <Accordion key={section}>
          <AccordionSummary
            expandIcon={<ExpandMoreIcon />}
            aria-controls={`${section}-content`}
            id={`${section}-header`}
          >
            <Typography>{section}</Typography>
          </AccordionSummary>
          <AccordionDetails>
            <Box display="flex" flexDirection="column">
              <Typography>Label 1</Typography>
              <Typography>Label 2</Typography>
              <Typography>Label 3</Typography>
            </Box>
          </AccordionDetails>
        </Accordion>
      ))}
    </Box>
  );
}

export default SceneView;
