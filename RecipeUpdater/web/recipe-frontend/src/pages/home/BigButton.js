import React from "react";
import {
  makeStyles, Button, Grid
} from "@material-ui/core";
import { Link as RouterLink } from 'react-router-dom';

const useStyles = makeStyles((theme) => ({
  button: {
    padding: theme.spacing(2),
    marginRight: theme.spacing(1),
    marginBottom: theme.spacing(1),
    width: '10em',
    height: '10em',
    background: theme.palette.background.paper
  }
}));

export default function BigButton(props) {
  const { to, icon, text } = props;
  const classes = useStyles();
  return (
    <Button
      color="primary"
      variant="outlined"
      component={RouterLink}
      to={to}
      className={classes.button}
    >
      <Grid
        container
        direction="column"
        justifyContent="center"
        alignItems="center"
      >
        <Grid item>
          {icon}
        </Grid>
        <Grid item>
          {text}
        </Grid>
      </Grid>
    </Button>
  );
}
