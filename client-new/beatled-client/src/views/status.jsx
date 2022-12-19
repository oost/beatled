import { useLoaderData, Form, useFetcher } from "react-router-dom";
import { useInterval } from "../hooks/interval";
import { getStatus } from "../lib/status";
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
} from "reactstrap";
import { FormattedNumber } from "react-intl";

import BeatChart from "../components/status/BeatChart";
import { format } from "date-fns";

// import BeatChart from "../components/status/BeatChart";

const props = [
  { label: "Status", key: "status", format: (v) => v },
  {
    label: "Tempo",
    key: "tempo",
    format: (v) => (
      <FormattedNumber
        value={v}
        minimumFractionDigits={1}
        maximumFractionDigits={2}
      />
    ),
  },
  {
    label: "Last update",
    key: "x",
    format: (v) => format(v, "h:mm:ss a"),
  },
  {
    label: "Beat Detector",
    key: "beat_detector",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "Broadcaster",
    key: "broadcaster",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "UDP Server",
    key: "udp_server",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "HTTP Server",
    key: "http_server",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "Message",
    key: "message",
    format: (v) => (v ? "On" : "Off"),
  },
];

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

export default function Status() {
  const fetcher = useFetcher();
  const statusData = useLoaderData();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit();
    }
  }, 2 * 1000);

  const data = fetcher.data ? fetcher.data.last : statusData.last;
  const historyData = fetcher.data ? fetcher.data.history : statusData.history;
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
          <Col xs={12} md={4}>
            <Card className="card-chart">
              <CardHeader>
                {/* <h5 className="card-category">Status</h5> */}
                <CardTitle tag="h4">Beatled Status</CardTitle>
                <fetcher.Form method="post">
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
                      <i className="now-ui-icons arrows-1_refresh-69" />
                    </button>
                  </div>
                </fetcher.Form>
              </CardHeader>
              <CardBody>
                <Table responsive>
                  <tbody>
                    {props.map((prop) => {
                      return (
                        <tr key={prop.key}>
                          <th>{prop.label}</th>
                          <td>{prop.format(data[prop.key])}</td>
                        </tr>
                      );
                    })}
                  </tbody>
                </Table>
              </CardBody>
              <CardFooter>
                <div className="stats"></div>
              </CardFooter>
            </Card>
          </Col>

          <Col xs={12} md={4}>
            <BeatChart historyData={historyData} />
          </Col>
        </Row>
      </div>
    </>
  );
}
