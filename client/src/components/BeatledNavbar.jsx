/*!

=========================================================
* Now UI Dashboard React - v1.5.1
=========================================================

* Product Page: https://www.creative-tim.com/product/now-ui-dashboard-react
* Copyright 2022 Creative Tim (https://www.creative-tim.com)
* Licensed under MIT (https://github.com/creativetimofficial/now-ui-dashboard-react/blob/main/LICENSE.md)

* Coded by Creative Tim

=========================================================

* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

*/
import React, { useState } from "react";
import { NavLink } from "react-router-dom";
import {
  Collapse,
  Navbar,
  NavbarToggler,
  NavbarBrand,
  Nav,
  NavItem,
} from "reactstrap";
import logoImg from "../assets/img/logo.svg";

// import routes from "routes.js";

function BeatledNavbar(props) {
  const [isOpen, setIsOpen] = useState(false);

  const toggle = () => setIsOpen(!isOpen);

  return (
    <Navbar fixed="top" expand={"md"}>
      <NavbarBrand>
        <NavLink to={"/"}>
          <img src={logoImg} alt="react-logo" />
        </NavLink>
      </NavbarBrand>
      <NavbarToggler onClick={toggle} />
      <Collapse isOpen={isOpen} navbar>
        <Nav className="me-auto" navbar>
          {props.routes.map((route) => (
            <NavItem key={route.path}>
              <NavLink to={route.path} className="nav-link">
                <span>
                  {route.icon} {route.name}
                </span>
              </NavLink>
            </NavItem>
          ))}
        </Nav>
        {/* <NavbarText>Simple Text</NavbarText> */}
      </Collapse>
    </Navbar>
  );
}

export default BeatledNavbar;
