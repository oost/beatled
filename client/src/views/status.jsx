import { useLoaderData, useFetcher } from "react-router-dom";
import { useEffect } from "react";
import { useInterval } from "../hooks/interval";
import { getStatus, serviceControl } from "../lib/status";
import PanelHeader from "../components/PanelHeader";
import {
  Row,
  Col,
  Card,
  CardHeader,
  CardTitle,
  CardBody,
  CardFooter,
  Table,
  Input,
  FormGroup,
} from "reactstrap";
import { FormattedNumber } from "react-intl";

import BeatChart from "../components/status/BeatChart";
import { format } from "date-fns";
import { HiOutlineRefresh } from "react-icons/hi";

const tempoHistory = [];
const MAX_HISTORY = 30;

export async function loader({ request }) {
  console.log("Loading status");
  const status = await getStatus();
  // const tempo = 120 + 40 * 2 * (Math.random() - 0.5);
  const last = { x: new Date(), y: status.tempo || NaN, ...status };
  tempoHistory.push(last);
  if (tempoHistory.length > MAX_HISTORY) {
    tempoHistory.splice(0, tempoHistory.length - MAX_HISTORY);
  }
  return {
    last,
    history: tempoHistory,
  };
}

export async function action({ request, params }) {
  return true;
}

export default function StatusPage() {
  const fetcher = useFetcher();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit();
    }
  }, 2 * 1000);

  const toggleService = async (serviceId, status) => {
    await serviceControl(serviceId, status);

    return fetcher.submit();
  };

  useEffect(() => {
    if (fetcher.state === "idle" && !fetcher.data) {
      fetcher.submit();
    }
  }, [fetcher]);

  const data = fetcher.data?.last || {};
  const historyData = fetcher.data?.history || {};
  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Status</h2>
          </div>
        }
      />
      <div className="content">
        <Row>
          <Col xs={12} md={6}>
            <fetcher.Form method="post">
              <Card className="card-chart">
                <CardHeader>
                  {/* <h5 className="card-category">Status</h5> */}
                  <CardTitle tag="h4">Beatled Status</CardTitle>
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
                  <Table responsive>
                    <tbody>
                      <tr>
                        <th>Last update</th>
                        <td>{data.x && format(data.x, "h:mm:ss a")}</td>
                      </tr>

                      <tr>
                        <th>Tempo</th>
                        <td>
                          {data.tempo && (
                            <FormattedNumber
                              value={data.tempo}
                              minimumFractionDigits={1}
                              maximumFractionDigits={2}
                            />
                          )}
                        </td>
                      </tr>

                      {data.status &&
                        Object.entries(data.status).map(
                          ([controllerId, controllerStatus]) => {
                            return (
                              <tr key={controllerId}>
                                <th>{controllerId}</th>
                                <td>
                                  <FormGroup switch disabled>
                                    <Input
                                      type="switch"
                                      checked={controllerStatus}
                                      onChange={(e) =>
                                        toggleService(
                                          controllerId,
                                          e.target.checked
                                        )
                                      }
                                    />
                                  </FormGroup>
                                </td>
                              </tr>
                            );
                          }
                        )}
                    </tbody>
                  </Table>
                </CardBody>
                <CardFooter>
                  <div className="stats"></div>
                </CardFooter>
              </Card>
            </fetcher.Form>
          </Col>

          <Col xs={12} md={6}>
            <Card className="card-chart">
              <CardHeader>
                <CardTitle tag="h4">Beat History</CardTitle>
              </CardHeader>
              <CardBody>
                <BeatChart historyData={historyData} />
              </CardBody>
              <CardFooter>
                <div className="stats">
                  <i className="now-ui-icons arrows-1_refresh-69" /> Just
                  Updated
                </div>
              </CardFooter>
            </Card>
          </Col>
        </Row>
      </div>
    </>
  );
}
