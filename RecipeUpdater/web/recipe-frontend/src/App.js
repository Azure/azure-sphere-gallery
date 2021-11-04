import React from "react";
import {
  BrowserRouter as Router,
  Switch,
  Route
} from "react-router-dom";
import { PrivateRoute } from "./components/PrivateRoute";
import UploadPage from './pages/upload';
import LoginPage from "./pages/login";
import LogoutPage from "./pages/logout";
import HomePage from './pages/home';
import DevicesPage from "./pages/devices";
import Layout from "./layout";

export default function App() {
  return (
    <Router>
      <Layout>

        {/* A <Switch> looks through its children <Route>s and
            renders the first one that matches the current URL. */}
        <Switch>
          <PrivateRoute path="/upload">
            <UploadPage />
          </PrivateRoute>
          <PrivateRoute path="/devices">
            <DevicesPage />
          </PrivateRoute>
          <Route path="/login">
            <LoginPage />
          </Route>
          <Route path="/logout">
            <LogoutPage />
          </Route>
          <Route path="/">
            <HomePage />
          </Route>
        </Switch>
      </Layout>
    </Router>
  );
}
