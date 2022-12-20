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
import React from "react";
import { Link, useLocation } from "react-router-dom";
import { Navbar, NavbarBrand } from "reactstrap";

// import routes from "routes.js";

function BeatledNavbar(props) {
  const location = useLocation();
  const sidebarToggle = React.useRef();

  const getBrand = () => {
    var name;
    // routes.map((prop, key) => {
    //   if (prop.collapse) {
    //     prop.views.map((prop, key) => {
    //       if (prop.path === props.location.pathname) {
    //         name = prop.name;
    //       }
    //       return null;
    //     });
    //   } else {
    //     if (prop.redirect) {
    //       if (prop.path === props.location.pathname) {
    //         name = prop.name;
    //       }
    //     } else {
    //       if (prop.path === props.location.pathname) {
    //         name = prop.name;
    //       }
    //     }
    //   }
    //   return null;
    // });
    return name;
  };
  const openSidebar = () => {
    document.documentElement.classList.toggle("nav-open");
    sidebarToggle.current.classList.toggle("toggled");
  };

  React.useEffect(() => {
    if (
      window.innerWidth < 993 &&
      document.documentElement.className.indexOf("nav-open") !== -1
    ) {
      document.documentElement.classList.toggle("nav-open");
      sidebarToggle.current.classList.toggle("toggled");
    }
  }, [location]);
  return (
    // add or remove classes depending if we are on full-screen-maps page or not
    <Navbar
      color={
        location.pathname.indexOf("full-screen-maps") !== -1
          ? "white"
          : "transparent"
      }
      expand="lg"
      className={
        location.pathname.indexOf("full-screen-maps") !== -1
          ? "navbar-absolute fixed-top"
          : "navbar-absolute fixed-top " +
            ("transparent" === "transparent" ? "navbar-transparent " : "")
      }
    >
      <div className="navbar-wrapper">
        <div className="navbar-toggle">
          <button
            type="button"
            ref={sidebarToggle}
            className="navbar-toggler"
            onClick={() => openSidebar()}
          >
            <span className="navbar-toggler-bar bar1" />
            <span className="navbar-toggler-bar bar2" />
            <span className="navbar-toggler-bar bar3" />
          </button>
        </div>
        <NavbarBrand href="/">{getBrand()}</NavbarBrand>
      </div>
    </Navbar>
  );
}

export default BeatledNavbar;
