import React from "react";
import { useHistory } from 'react-router-dom';
import TextField from "@material-ui/core/TextField";
import Button from "@material-ui/core/Button";
import {
  Box, Grid, Paper, Typography
} from "@material-ui/core";
import Alert from '@material-ui/lab/Alert';
import { authenticationService } from "../../helpers/auth-service";

export default function LoginPage() {
  const [username, setUsername] = React.useState("");
  const [password, setPassword] = React.useState("");
  const [error, setError] = React.useState(null);
  const [loading, setLoading] = React.useState(false);

  const history = useHistory();

  function handleLogin(e) {
    e.preventDefault();
    setLoading(true);

    authenticationService.login(username, password)
      .then(() => {
        history.push("/");
        setLoading(false);
      })
      .catch((err) => {
        setError(<Box my={2}><Alert severity="error">{err}</Alert></Box>);
        setLoading(false);
      });

    return false;
  }

  return (
    <Grid
      justifyContent="center"
      alignItems="center"
      container
    >
      <Grid item lg={4} xs={12} sm={9}>
        <Box mt={1} p={2} component={Paper} elevation={0}>
          <form onSubmit={handleLogin}>
            <Grid
              direction="column"
              container
              alignItems="stretch"
            >
              <Grid item>
                <Typography variant="h5" gutterBottom>Login</Typography>
              </Grid>
              <Grid item>{error}</Grid>
              <Grid item>
                <TextField
                  fullWidth
                  label="Username"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                  required
                />
              </Grid>
              <Grid item>
                <TextField
                  fullWidth
                  type="password"
                  label="Password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  required
                />
              </Grid>
              <Grid item>
                <Box mt={2}>
                  <Button fullWidth variant="contained" type="submit" color="primary" disabled={loading}>Login</Button>
                </Box>
              </Grid>
            </Grid>
          </form>
        </Box>
      </Grid>
    </Grid>
  );
}
