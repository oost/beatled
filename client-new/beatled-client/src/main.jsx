import React from "react";
import ReactDOM from "react-dom/client";
import { createHashRouter, RouterProvider } from "react-router-dom";
import { IntlProvider } from "react-intl";

import routes from "./routes";
// Webpack CSS import
// import "./index.css";

import "bootstrap/dist/css/bootstrap.min.css";
import "./assets/scss/now-ui-dashboard.scss?v1.5.0";
import "./assets/css/demo.css";

// Translated messages in French with matching IDs to what you declared
const messagesInFrench = {
  myMessage: "Aujourd'hui, c'est le {ts, date, ::yyyyMMdd}",
};

const router = createHashRouter(routes);
ReactDOM.createRoot(document.getElementById("root")).render(
  <React.StrictMode>
    <IntlProvider messages={messagesInFrench} locale="en" defaultLocale="en">
      <RouterProvider router={router} />
    </IntlProvider>
  </React.StrictMode>
);
