import React from "react";
import { Container } from "@material-ui/core";
import Header from "./Header";

export default function Layout(props) {
  return (
    <>
      <Header />
      <Container {...props} />
    </>
  );
}
