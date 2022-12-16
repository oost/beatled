import Root, {
  loader as rootLoader,
  action as rootAction,
} from "../views/root";
import ErrorPage from "../views/error-page";
import Index from "../views/index";
import Contact, { loader as contactLoader } from "../views/contact";
import EditContact, { action as editAction } from "../views/edit";
import { action as destroyAction } from "../views/destroy";
import Status, {
  loader as statusLoader,
  action as statusAction,
} from "../views/status";
import Admin from "../components/Admin";

const routes = [
  {
    path: "/",
    element: <Root />,
    errorElement: <ErrorPage />,
    loader: rootLoader,
    action: rootAction,
    children: [
      { index: true, element: <Index /> },
      {
        path: "contacts/:contactId",
        element: <Contact />,
        loader: contactLoader,
      },
      {
        path: "contacts/:contactId/edit",
        element: <EditContact />,
        loader: contactLoader,
        action: editAction,
      },
      {
        path: "contacts/:contactId/destroy",
        action: destroyAction,
        errorElement: <div>Oops! There was an error.</div>,
      },
    ],
  },
  {
    path: "/admin",
    element: <Admin />,
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
