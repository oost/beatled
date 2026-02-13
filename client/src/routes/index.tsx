import ErrorPage from "../components/ErrorPage";
import Index from "../views/index";
import StatusPage, { loader as statusLoader, action as statusAction } from "../views/status";
import ProgramPage, { loader as programLoader, action as programAction } from "../views/program";
import LogPage, { loader as logLoader, action as logAction } from "../views/log";
import ConfigPage, { loader as configLoader, action as configAction } from "../views/config";

import RootContainer from "../components/RootContainer";
import { RootErrorBoundary } from "../components/RootErrorBoundaries";

const routes = [
  {
    path: "/",
    element: <RootContainer />,
    errorElement: <RootErrorBoundary />,
    children: [
      { index: true, element: <Index /> },
      {
        path: "status",
        element: <StatusPage />,
        loader: statusLoader,
        action: statusAction,
        errorElement: <ErrorPage />,
      },
      {
        path: "program",
        loader: programLoader,
        action: programAction,
        element: <ProgramPage />,
        errorElement: <ErrorPage />,
      },
      {
        path: "log",
        element: <LogPage />,
        loader: logLoader,
        action: logAction,
        errorElement: <ErrorPage />,
      },
      {
        path: "config",
        element: <ConfigPage />,
        loader: configLoader,
        action: configAction,
        errorElement: <ErrorPage />,
      },
    ],
  },
];

export default routes;
