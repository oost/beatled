import ErrorPage from "../views/error-page";
import Index from "../views/index";
import StatusPage, {
  loader as statusLoader,
  action as statusAction,
} from "../views/status";
import ProgramPage from "../views/program";
import LogPage, {
  loader as logLoader,
  action as logAction,
} from "../views/log";
import RootContainer from "../components/RootContainer";
import { RootErrorBoundary } from "../views/root-error-boundary";

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
    ],
  },
];

export default routes;
