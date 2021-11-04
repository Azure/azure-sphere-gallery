import React, { useEffect, useState } from "react";
import { makeStyles } from "@material-ui/core/styles";
import AppBar from "@material-ui/core/AppBar";
import Toolbar from "@material-ui/core/Toolbar";
import Typography from "@material-ui/core/Typography";
import Button from "@material-ui/core/Button";
import IconButton from "@material-ui/core/IconButton";
import CloudCircleIcon from "@material-ui/icons/CloudCircle";
import { Container, Link } from "@material-ui/core";
import { Link as RouterLink } from 'react-router-dom';
import { authenticationService } from "../helpers/auth-service";

const useStyles = makeStyles((theme) => ({
  root: {
    flexGrow: 1
  },
  appBar: {
    background: theme.palette.background.paper,
    color: theme.palette.text.primary
  },
  menuButton: {
    marginRight: theme.spacing(2)
  },
  title: {
    flexGrow: 1
  }
}));

export default function ButtonAppBar() {
  const classes = useStyles();
  const [user, setUser] = useState();
  let menu = [];

  useEffect(() => {
    const subscription = authenticationService.currentUser.subscribe((currentUser) => {
      setUser(currentUser);
    });
    return () => {
      subscription.unsubscribe();
    };
  }, []);

  if (user) {
    menu = [
      <Button key="upload" color="inherit" component={RouterLink} to="/upload">Upload</Button>,
      <Button key="logout" color="inherit" component={RouterLink} to="/logout">Logout</Button>
    ];
  } else {
    menu = [<Button key="login" color="inherit" component={RouterLink} to="/login">Login</Button>];
  }

  return (
    <div className={classes.root}>
      <AppBar position="static" className={classes.appBar} elevation={0}>
        <Container>
          <Toolbar disableGutters>
            <IconButton
              edge="start"
              className={classes.menuButton}
              color="inherit"
              aria-label="menu"
              component={RouterLink}
              to="/"
            >
              <CloudCircleIcon />
            </IconButton>
            <Typography variant="h6" className={classes.title}>
              <Link color="inherit" underline="none" component={RouterLink} to="/">
                Recipe Pusher
              </Link>
            </Typography>
            {menu}
          </Toolbar>
        </Container>
      </AppBar>
    </div>
  );
}
