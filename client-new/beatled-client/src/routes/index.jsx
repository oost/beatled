import ErrorPage from "../views/error-page";
import Index from "../views/index";
import Status, {
  loader as statusLoader,
  action as statusAction,
} from "../views/status";
import RootContainer from "../components/RootContainer";

const routes = [
  {
    path: "/",
    element: <RootContainer />,
    errorElement: <ErrorPage />,
    children: [
      { index: true, element: <Index /> },
      {
        path: "status",
        element: <Status />,
        loader: statusLoader,
        action: statusAction,
        errorElement: <div>Oops! There was an error.</div>,
      },
      {
        path: "program",
        errorElement: <div>Oops! There was an error.</div>,
      },
    ],
  },
];

export default routes;
