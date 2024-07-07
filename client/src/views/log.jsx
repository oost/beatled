import PanelHeader from "../components/PanelHeader";
import { getLogs } from "../lib/log";
import { useLoaderData, useFetcher } from "react-router-dom";
import { useEffect } from "react";
import { useInterval } from "../hooks/interval";

import {
  Row,
  Col,
  Card,
  CardHeader,
  CardTitle,
  CardBody,
  CardFooter,
} from "reactstrap";
import { HiOutlineRefresh } from "react-icons/hi";
import { getConsoleLogs } from "../lib/console";

export async function loader({ request }) {
  console.log("Loading status");

  return { logs: await getLogs(), consoleLogs: getConsoleLogs() };
}

export async function action({ request, params }) {
  return true;
}

export default function LogPage() {
  const fetcher = useFetcher();
  const statusData = useLoaderData();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit();
    }
  }, 10 * 1000);

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit();
    }
  }, [fetcher]);

  // const logs = fetcher.data ? fetcher.data.logs : statusData.logs;
  const logs = fetcher.data?.logs || [];
  const consoleLogs = fetcher.data?.consoleLogs || [];

  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Logs</h2>
          </div>
        }
      />
      <div className="content">
        <fetcher.Form method="post">
          <Row>
            <Col xs={12} md={12}>
              <Card className="card-chart">
                <CardHeader>
                  <CardTitle tag="h4">Beatled Logs</CardTitle>
                  <div
                    onClick={(event) => {
                      fetcher.submit();
                    }}
                    className="dropdown right"
                  >
                    <button
                      type="button"
                      aria-haspopup="true"
                      aria-expanded="true"
                      className="btn-round btn-outline-default btn-icon btn btn-default"
                    >
                      <HiOutlineRefresh />
                    </button>
                  </div>
                </CardHeader>
                <CardBody>
                  {logs?.error ? (
                    <p>{logs.status}</p>
                  ) : (
                    <pre>{logs.map((logLine) => logLine)}</pre>
                  )}
                </CardBody>
                <CardFooter>
                  <div className="stats"></div>
                </CardFooter>
              </Card>
            </Col>
            <Col xs={12} md={12}>
              <Card className="card-chart">
                <CardHeader>
                  <CardTitle tag="h4">Beatled Logs</CardTitle>
                  <div
                    onClick={(event) => {
                      fetcher.submit();
                    }}
                    className="dropdown right"
                  >
                    <button
                      type="button"
                      aria-haspopup="true"
                      aria-expanded="true"
                      className="btn-round btn-outline-default btn-icon btn btn-default"
                    >
                      <HiOutlineRefresh />
                    </button>
                  </div>
                </CardHeader>
                <CardBody>
                  <pre>{consoleLogs.map((logLine) => logLine)}</pre>
                </CardBody>
                <CardFooter>
                  <div className="stats"></div>
                </CardFooter>
              </Card>
            </Col>
          </Row>
        </fetcher.Form>
      </div>
    </>
  );
}
