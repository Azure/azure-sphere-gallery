import React from "react";
import { Route, Redirect } from "react-router-dom";

import { authenticationService } from "../helpers/auth-service";

export function PrivateRoute(props) {
  const currentUser = authenticationService.currentUserValue;

  if (!currentUser) {
    return (
      <Redirect to={{ pathname: "/login", state: { from: props.location } }} />
    );
  }
  return <Route {...props} />;
}
