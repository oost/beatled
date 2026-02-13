import React from "react";
import ReactDOM from "react-dom/client";
import { createBrowserRouter, RouterProvider } from "react-router-dom";
import { IntlProvider } from "react-intl";
import { initializeConsole } from "./lib/console";
import routes from "./routes";

import { registerSW } from "virtual:pwa-register";

registerSW({ immediate: true });

import "./index.css";

initializeConsole();
const router = createBrowserRouter(routes);

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <IntlProvider locale="en" defaultLocale="en">
      <RouterProvider router={router} />
    </IntlProvider>
  </React.StrictMode>,
);
