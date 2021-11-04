import React from "react";
import Button from "@material-ui/core/Button";
import { Typography, Box } from "@material-ui/core";
import { Link as RouterLink } from "react-router-dom";
import { authenticationService } from "../../helpers/auth-service";

export default function LogoutPage() {
  React.useEffect(() => {
    authenticationService.logout();
  }, []);

  return (
    <Box mt={2}>
      <Typography variant="h5" gutterBottom>You have logged out.</Typography>
      <Button variant="contained" color="primary" component={RouterLink} to="/">Return Home</Button>
    </Box>
  );
}
