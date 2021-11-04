import React from "react";
import {
  Box, Typography, Paper
} from "@material-ui/core";
import DeviceTwinPicker from "../../components/DeviceTwinPicker";

export default function DevicesPage() {
  return (
    <Box component={Paper} elevation={0} mt={2} px={3} pt={2} pb={4}>
      <Typography variant="h5">All Devices</Typography>
      <DeviceTwinPicker />
    </Box>
  );
}
